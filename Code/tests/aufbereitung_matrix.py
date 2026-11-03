"""Serial Release benchmark matrix; no instrumentation or third-party packages."""
import argparse
import hashlib
import json
import pathlib
import statistics
import subprocess


def followup(args):
    """Keep the original matrix intact; recheck the final touched paths."""
    original = [json.loads(line) for line in
                (args.output / "release-matrix.jsonl").read_text(encoding="utf-8").splitlines()]
    path = args.output / "followup-release.jsonl"
    if path.exists():
        raise SystemExit("Follow-up evidence already exists.")
    records = []
    specifications = [("420", 500, "fixed", "foreign_on", True)]
    specifications += [(model, steps, setting, "hydro_drive", False)
                       for model in ("420", "folkeboot")
                       for steps in (50, 500) for setting in ("fixed", "events")]
    with path.open("w", encoding="utf-8") as out:
        for model, steps, setting, case, both in specifications:
            versions = [("C-0.10", "aufbereitung_benchmark.exe")]
            if both:
                versions.insert(0, ("C-0.9", "aufbereitung_referenz.exe"))
            for version, runner in versions:
                result = subprocess.run(
                    [str(args.build / runner), str(args.models / model / "modell.json"),
                     case, str(steps), setting], capture_output=True,
                    encoding="utf-8", check=True)
                for line in result.stdout.splitlines():
                    record = json.loads(line)
                    record.update(version=version, model=model)
                    records.append(record)
                    out.write(json.dumps(record, ensure_ascii=False) + "\n")
                out.flush()
                print("FOLLOWUP", version, model, steps, setting, case, flush=True)
    summary = []
    for model, steps, setting, case, both in specifications:
        def matches(r):
            return (r["model"], r["steps"], r["settings"], r["scenario"]) == (
                model, steps, setting, case)
        old = [r for r in (records if both else original)
               if matches(r) and r["version"] == "C-0.9"]
        new = [r for r in records if matches(r) and r["version"] == "C-0.10"]
        assert len(old) == len(new) == 5
        parity = all(a["status"] == b["status"] == "ok" and
                     a["trajectory_fnv1a64"] == b["trajectory_fnv1a64"]
                     for a, b in zip(old, new))
        phases = {}
        for phase in ("load_ms", "setup_ms", "steps_ms", "json_ms", "force_ms", "total_ms"):
            phases[phase] = {}
            for version, runs in (("C-0.9", old), ("C-0.10", new)):
                values = [r[phase] for r in runs]
                phases[phase][version] = {
                    "n": len(values), "raw": values, "median": statistics.median(values),
                    "min": min(values), "max": max(values)}
        summary.append({
            "model": model, "steps": steps, "settings": setting, "scenario": case,
            "reference": "same_followup_slot" if both else "original_main_matrix",
            "exact_trajectory_parity": parity, "phases_ms": phases,
            "total_speedup": statistics.median(r["total_ms"] for r in old) /
            statistics.median(r["total_ms"] for r in new),
            "nonoverlapping_improvement":
                max(r["total_ms"] for r in new) < min(r["total_ms"] for r in old)})
    (args.output / "followup-summary.json").write_text(json.dumps({
        "design": "One warmup + five repeats; final build; original evidence preserved",
        "raw_sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
        "original_raw_sha256": hashlib.sha256(
            (args.output / "release-matrix.jsonl").read_bytes()).hexdigest(),
        "summary": summary}, indent=2), encoding="utf-8")
    assert all(r["exact_trajectory_parity"] for r in summary)
    print("FOLLOWUP PARITY True", flush=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build", type=pathlib.Path, required=True)
    parser.add_argument("--models", type=pathlib.Path, required=True)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    parser.add_argument("--pilot", action="store_true")
    parser.add_argument("--followup", action="store_true")
    args = parser.parse_args()
    if args.followup:
        followup(args)
        return
    args.output.mkdir(parents=True, exist_ok=True)
    raw = args.output / "release-matrix.jsonl"
    if raw.exists():
        raise SystemExit("Output already exists; preserve evidence.")
    records = []
    cases = ("free", "drive", "hydro", "hydro_drive", "foreign_off", "foreign_on")
    if args.pilot:
        cases = ("hydro_drive",)
    with raw.open("w", encoding="utf-8") as out:
        for model in ("420", "folkeboot"):
            for steps in ((50,) if args.pilot else (50, 500)):
                for setting in (("fixed",) if args.pilot else ("fixed", "events")):
                    for case in cases:
                        for version, runner in (("C-0.9", "aufbereitung_referenz.exe"),
                                                ("C-0.10", "aufbereitung_benchmark.exe")):
                            command = [str(args.build / runner),
                                       str(args.models / model / "modell.json"),
                                       case, str(steps), setting]
                            result = subprocess.run(command, capture_output=True,
                                                    encoding="utf-8", check=True)
                            for line in result.stdout.splitlines():
                                record = json.loads(line)
                                record.update(version=version, model=model)
                                records.append(record)
                                out.write(json.dumps(record, ensure_ascii=False) + "\n")
                            out.flush()
                            print(version, model, steps, setting, case,
                                  records[-1]["status"], flush=True)
    groups = {}
    for r in records:
        key = (r["model"], r["steps"], r["settings"], r["scenario"])
        groups.setdefault(key, {}).setdefault(r["version"], []).append(r)
    summary = []
    for key, versions in groups.items():
        row = dict(zip(("model", "steps", "settings", "scenario"), key))
        old, new = versions["C-0.9"], versions["C-0.10"]
        row["reference_status"] = sorted({r["status"] for r in old})
        row["optimized_status"] = sorted({r["status"] for r in new})
        row["exact_trajectory_parity"] = all(
            a.get("trajectory_fnv1a64") == b.get("trajectory_fnv1a64") and
            a["status"] == b["status"] and
            a.get("diagnosis") == b.get("diagnosis")
            for a, b in zip(old, new))
        row["phases_ms"] = {}
        for phase in ("load_ms", "setup_ms", "steps_ms", "json_ms", "force_ms", "total_ms"):
            row["phases_ms"][phase] = {}
            for name, runs in versions.items():
                values = [r[phase] for r in runs if phase in r]
                if values:
                    row["phases_ms"][phase][name] = {
                        "n": len(values), "raw": values,
                        "median": statistics.median(values),
                        "min": min(values), "max": max(values)}
        if all(r["status"] == "ok" for r in old + new):
            before = statistics.median(r["total_ms"] for r in old)
            after = statistics.median(r["total_ms"] for r in new)
            row["total_speedup"] = before / after
            row["nonoverlapping_improvement"] = (
                max(r["total_ms"] for r in new) < min(r["total_ms"] for r in old))
        else:
            row["diagnoses"] = sorted({r.get("diagnosis", "") for r in old + new})
        summary.append(row)
    manifest = {
        "design": {"warmup": 1, "measured_runs": 5, "dt_s": .01,
                   "horizons_s": [.5] if args.pilot else [.5, 5.],
                   "serial": True, "instrumentation": False,
                   "states": "Initial state plus every integrated end state",
                   "events": "50 percent at midpoint; identical value next step",
                   "force_ms": "One separate final-state force request, after JSON",
                   "json_ms": "Every full JSON state, including force work if required",
                   "parity": "FNV-1a of every full JSON state, exact; final JSON retained"},
        "raw_sha256": hashlib.sha256(raw.read_bytes()).hexdigest(),
        "summary": summary}
    (args.output / "release-summary.json").write_text(
        json.dumps(manifest, indent=2, ensure_ascii=False), encoding="utf-8")
    print("PARITY", all(r["exact_trajectory_parity"] for r in summary), flush=True)


if __name__ == "__main__":
    main()
