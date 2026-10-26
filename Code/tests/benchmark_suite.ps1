param(
    [Parameter(Mandatory=$true)][string]$BinaryDirectory,
    [Parameter(Mandatory=$true)][string]$Report
)
$ErrorActionPreference = 'Stop'
$culture = [Globalization.CultureInfo]::InvariantCulture
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$binary = (Resolve-Path $BinaryDirectory).Path
$reportPath = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Report)
if (Test-Path -LiteralPath $reportPath) { throw 'Report already exists; refusing overwrite.' }
$models = Join-Path $root '..\Modelle'
$utf8 = New-Object Text.UTF8Encoding($false)
$temp = Join-Path ([IO.Path]::GetTempPath()) ('segel-core-bench-' + [guid]::NewGuid())
[void][IO.Directory]::CreateDirectory($temp)
function Invoke-Core([string]$Executable, [string[]]$Arguments) {
    $lines = & (Join-Path $binary $Executable) @Arguments
    if ($LASTEXITCODE -ne 0) { throw "Native command failed: $Executable ($LASTEXITCODE)" }
    return ($lines -join "`n")
}
function Get-Summary($Durations) {
    $sorted = @($Durations | Sort-Object)
    return [ordered]@{
        median_s = $sorted[[int][Math]::Floor($sorted.Count / 2)]
        min_s = $sorted[0]
        max_s = $sorted[-1]
        repetitions = $sorted.Count
    }
}
function Get-Duration([string]$Response) {
    if ($Response -notmatch '(?m)^Gesamtlaufzeit_s:\s*(\S+)') { throw 'Missing total duration.' }
    return [double]::Parse($Matches[1], $culture)
}
function Read-States([string]$Path, [bool]$Jsonl) {
    $content = [IO.File]::ReadAllText($Path, $utf8)
    if ($Jsonl) {
        return @($content -split "`n" | Where-Object { $_.Trim() } | ForEach-Object { $_ | ConvertFrom-Json })
    }
    foreach ($state in ($content | ConvertFrom-Json)) { $state }
}
try {
    $source = Join-Path $models 'quader\meshes\hydrostatik\rumpf.obj'
    $lines = [IO.File]::ReadAllLines($source, $utf8)
    $xs = @($lines | Where-Object { $_ -match '^v ' } | ForEach-Object {
        [double]::Parse(($_ -split '\s+')[1], $culture)
    })
    $range = $xs | Measure-Object -Minimum -Maximum
    $width = $range.Maximum - $range.Minimum
    $elements = @()
    for ($i = 0; $i -lt 8; $i++) {
        $name = "box-$i.obj"
        $shifted = foreach ($line in $lines) {
            if ($line -match '^v ') {
                $parts = $line -split '\s+'
                $x = [double]::Parse($parts[1], $culture) + ($i - 3.5) * ($width + 1)
                'v ' + $x.ToString('R', $culture) + ' ' + $parts[2] + ' ' + $parts[3]
            } else { $line }
        }
        [IO.File]::WriteAllLines((Join-Path $temp $name), [string[]]$shifted, $utf8)
        $elements += @{
            id = "Rumpf-$i"
            Hydrostatik = @{
                masse = @{art='vollkoerper'; dichte_kg_pro_m3=950; datei=$name}
                volumen = @{datei=$name}
            }
        }
    }
    $composite = Join-Path $temp 'modell.json'
    $model = @{schema_version='0.1-entwurf'; id='acht_quader'; gruppen=@{rumpf=@{elemente=$elements}}}
    [IO.File]::WriteAllText($composite, ($model | ConvertTo-Json -Depth 12), $utf8)
    $cases = @()
    foreach ($entry in @(
        @{name='small'; path=(Join-Path $models 'quader\modell.json')},
        @{name='composite'; path=$composite},
        @{name='large'; path=(Join-Path $models 'kugel\modell.json')}
    )) {
        $case = (Invoke-Core 'core_benchmark.exe' @($entry.path)) | ConvertFrom-Json
        $runs = [ordered]@{}
        foreach ($mode in @('statisch','dynamisch')) {
            $durations = @()
            for ($repeat=0; $repeat -lt 3; $repeat++) {
                $output = Join-Path $temp "$($entry.name)-$mode-$repeat.jsonl"
                $arguments = @("--$mode", $entry.path, $output)
                if ($mode -eq 'dynamisch') { $arguments += @('0.2','0.01') }
                $durations += Get-Duration (Invoke-Core 'segelphysik_cli.exe' $arguments)
                $states = @(Read-States $output $true)
                foreach ($state in $states) {
                    if ($state.orientierung_xyzw.Count -ne 4 -or $state.modell_fassung -ne $case.modell_fassung) {
                        throw 'Invalid recorded reference state.'
                    }
                }
                $expected = 'konvergiert'
                if ($mode -eq 'dynamisch') { $expected = 'abgeschlossen' }
                if ($states[-1].status -ne $expected) { throw 'Unexpected final status.' }
            }
            $summary = Get-Summary $durations
            if ($mode -eq 'dynamisch') { $summary['simulated_seconds_per_wall_second'] = 0.2 / $summary.median_s }
            $runs[$mode] = $summary
        }
        $case | Add-Member -NotePropertyName case -NotePropertyValue $entry.name
        $case | Add-Member -NotePropertyName full_runs -NotePropertyValue $runs
        $cases += $case
    }
    $formats = [ordered]@{}
    foreach ($extension in @('json','jsonl')) {
        $durations = @()
        for ($repeat=0; $repeat -lt 7; $repeat++) {
            $output = Join-Path $temp "format-$repeat.$extension"
            $arguments = @('--dynamisch', (Join-Path $models 'quader\modell.json'), $output, '1', '0.001')
            $durations += Get-Duration (Invoke-Core 'segelphysik_cli.exe' $arguments)
            $states = @(Read-States $output ($extension -eq 'jsonl'))
            if ($states.Count -ne 1001 -or $states[-1].status -ne 'abgeschlossen' -or $states[-1].zeit_s -ne 1) {
                throw 'Unexpected recording count, endpoint or status.'
            }
        }
        $formats[$extension] = Get-Summary $durations
    }
    $result = [ordered]@{
        platform = [Environment]::OSVersion.VersionString
        cpu = $env:PROCESSOR_IDENTIFIER
        build = 'MSVC Release x64, /fp:precise, single-threaded'
        performance_acceptance = 'Baseline only; no approved universal speed budget'
        cases = $cases
        format_full_run = $formats
    }
    [IO.File]::WriteAllText($reportPath, (($result | ConvertTo-Json -Depth 16) + "`n"), $utf8)
    Write-Output "Verified benchmark report: $reportPath"
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force
}
