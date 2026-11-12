#include "modellformat_03_intern.hpp"

#include "segelphysik/defaults.hpp"
#include "segelphysik/modell_fingerprints.hpp"

#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonschema/jsonschema.hpp>
#include <picosha2.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <map>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <cstdlib>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace segelphysik::detail {
namespace {

using Json = nlohmann::json;

[[noreturn]] void modelfehler(const std::string& ort, const std::string& text)
{
    throw std::runtime_error("CORE-MOD-0002 [" + ort + "]: " + text);
}

std::string lesen(const std::filesystem::path& datei)
{
    std::ifstream input(datei, std::ios::binary);
    if (!input) modelfehler(datei.filename().u8string(), "Datei nicht lesbar");
    const std::string result{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    if (input.bad()) modelfehler(datei.filename().u8string(), "Lesefehler");
    return result;
}

Json json_aus_bytes(const std::string& bytes, const std::string& name)
{
    try {
        std::vector<std::set<std::string>> schluessel;
        return Json::parse(bytes, [&](int, Json::parse_event_t event, Json& value) {
            if (event == Json::parse_event_t::object_start) schluessel.emplace_back();
            else if (event == Json::parse_event_t::object_end) schluessel.pop_back();
            else if (event == Json::parse_event_t::key &&
                !schluessel.back().insert(value.get<std::string>()).second)
                throw std::runtime_error("CORE-MOD-0001 [" + name +
                    "]: Doppelter Objektschluessel: " + value.get<std::string>());
            return true;
        });
    } catch (const Json::exception& error) {
        throw std::runtime_error("CORE-MOD-0001 [" + name +
            "]: Ungueltiges JSON: " + error.what());
    }
}

Json json_lesen(const std::filesystem::path& datei)
{
    return json_aus_bytes(lesen(datei), datei.filename().u8string());
}

class Sha256 {
public:
    void update(const unsigned char* bytes, std::size_t count)
    {
        hasher_.process(bytes, bytes + count);
    }

    void update(const std::string& value)
    {
        update(reinterpret_cast<const unsigned char*>(value.data()), value.size());
    }

    std::string finish()
    {
        hasher_.finish();
        return picosha2::get_hash_hex_string(hasher_);
    }

private:
    picosha2::hash256_one_by_one hasher_;
};

std::string bereich_name(ModellFingerprintBereich bereich)
{
    switch (bereich) {
    case ModellFingerprintBereich::visualisation: return "visualisation";
    case ModellFingerprintBereich::mass: return "mass";
    case ModellFingerprintBereich::hydrostatics: return "hydrostatics";
    case ModellFingerprintBereich::aerodynamics: return "aerodynamics";
    case ModellFingerprintBereich::modelData: return "modelData";
    }
    throw std::runtime_error("CORE-MOD-0002: Unbekannter Fingerprint-Bereich");
}

std::filesystem::path bereich_pfad(
    const std::filesystem::path& modellordner, ModellFingerprintBereich bereich)
{
    switch (bereich) {
    case ModellFingerprintBereich::visualisation: return modellordner / "modells/visualisation";
    case ModellFingerprintBereich::mass: return modellordner / "modells/mass";
    case ModellFingerprintBereich::hydrostatics: return modellordner / "modells/hydrostatics";
    case ModellFingerprintBereich::aerodynamics: return modellordner / "modells/aerodynamics";
    case ModellFingerprintBereich::modelData: return modellordner / "modells_data.json";
    }
    throw std::runtime_error("CORE-MOD-0002: Unbekannter Fingerprint-Bereich");
}

void laenge(Sha256& hash, std::uint64_t wert)
{
    std::array<unsigned char, 8> bytes{};
    for (unsigned i = 0; i < bytes.size(); ++i) {
        bytes[i] = static_cast<unsigned char>(wert & 0xffU);
        wert >>= 8U;
    }
    hash.update(bytes.data(), bytes.size());
}

std::string modified_at()
{
    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm local{};
#ifdef _WIN32
    gmtime_s(&local, &now);
#else
    gmtime_r(&now, &local);
#endif
    std::ostringstream result;
    result << std::put_time(&local, "%Y-%m-%d");
    return result.str();
}

class SchemaPruefer {
public:
    explicit SchemaPruefer(const Json& root)
        : schema_(jsoncons::jsonschema::make_json_schema(
            jsoncons::json::parse(root.dump()),
            jsoncons::jsonschema::evaluation_options{}.require_format_validation(true)))
    {
        if (root.value("$schema", std::string{}) !=
            jsoncons::jsonschema::schema_version::draft202012())
            modelfehler("$schema", "JSON Schema Draft 2020-12 erforderlich");
    }

    std::vector<std::string> pruefe(const Json& instance)
    {
        std::vector<std::string> result;
        schema_.validate(jsoncons::json::parse(instance.dump()),
            [&](const jsoncons::jsonschema::validation_message& message) {
                result.push_back(message.instance_location().string() + ": " + message.message());
                return jsoncons::jsonschema::walk_result::advance;
            });
        return result;
    }

private:
    jsoncons::jsonschema::json_schema<jsoncons::json> schema_;
};

std::filesystem::path sichere_ressource(
    const std::filesystem::path& root, const std::string& relative)
{
    const std::filesystem::path input = std::filesystem::u8path(relative);
    if (input.empty() || input.is_absolute() || input.has_root_name() || input.has_root_directory())
        modelfehler("resource", "Ressourcenpfad muss relativ sein: " + relative);
    for (const auto& component : input)
        if (component == "." || component == "..")
            modelfehler("resource", "Ressourcenpfad verlaesst den Modellordner: " + relative);
    auto current = root;
    std::error_code error;
    for (const auto& component : input) {
        current /= component;
        const auto status = std::filesystem::symlink_status(current, error);
        if (error || std::filesystem::is_symlink(status))
            modelfehler("resource", "Ressource fehlt oder ist ein Symlink: " + relative);
    }
    if (!std::filesystem::is_regular_file(current, error) || error)
        modelfehler("resource", "Ressource fehlt oder ist keine regulaere Datei: " + relative);
    return current;
}

void pruefe_ressourcen(const Json& data, const std::filesystem::path& model)
{
    if (!data.contains("groups") || !data["groups"].is_object()) return;
    for (auto group = data["groups"].begin(); group != data["groups"].end(); ++group) {
        if (!group.value().is_object() || !group.value().contains("elements") ||
            !group.value()["elements"].is_array()) continue;
        for (const auto& element : group.value()["elements"]) {
            if (!element.is_object()) continue;
            const auto add = [&](const Json& object, const char* key) {
                if (object.is_object() && object.contains(key) && object[key].is_string())
                    sichere_ressource(model, object[key].get<std::string>());
            };
            if (element.contains("display")) add(element["display"], "geometry");
            if (!element.contains("physics") || !element["physics"].is_object()) continue;
            const auto& physics = element["physics"];
            if (physics.contains("mass")) add(physics["mass"], "geometry");
            if (physics.contains("hydrostatic")) add(physics["hydrostatic"], "geometry");
        }
    }
}

std::vector<std::filesystem::path> dateien_im_bereich(
    const std::filesystem::path& modellordner, ModellFingerprintBereich bereich)
{
    const auto path = bereich_pfad(modellordner, bereich);
    std::error_code error;
    std::vector<std::filesystem::path> result;
    auto current = modellordner;
    for (const auto& component : path.lexically_relative(modellordner)) {
        current /= component;
        const auto status = std::filesystem::symlink_status(current, error);
        if (error == std::errc::no_such_file_or_directory) {
            error.clear();
            break;
        }
        if (error || std::filesystem::is_symlink(status))
            modelfehler(current.filename().u8string(),
                "Fingerprint-Pfad nicht lesbar oder enthaelt einen Symlink");
    }
    if (bereich == ModellFingerprintBereich::modelData) {
        if (std::filesystem::is_symlink(std::filesystem::symlink_status(path, error)) || error)
            modelfehler(path.filename().u8string(), "Modelldatei ist nicht regulaer");
        if (!std::filesystem::is_regular_file(path, error) || error)
            modelfehler(path.filename().u8string(), "Modelldatei fehlt");
        result.push_back(path);
    } else if (std::filesystem::exists(path, error)) {
        if (std::filesystem::is_symlink(std::filesystem::symlink_status(path, error)) || error)
            modelfehler(path.filename().u8string(), "Bereichsordner ist ein Symlink");
        if (!std::filesystem::is_directory(path, error) || error)
            modelfehler(path.filename().u8string(), "Fingerprint-Bereich ist kein Ordner");
        std::filesystem::recursive_directory_iterator it(path, error), end;
        if (error) modelfehler(path.filename().u8string(), "Fingerprint-Bereich nicht lesbar");
        for (;
             it != end; it.increment(error)) {
            if (error) modelfehler(path.filename().u8string(), "Fingerprint-Bereich nicht lesbar");
            const auto status = it->symlink_status(error);
            if (error) modelfehler(it->path().filename().u8string(), "Dateistatus nicht lesbar");
            if (std::filesystem::is_symlink(status))
                modelfehler(it->path().filename().u8string(), "Symlinks sind unzulaessig");
            if (std::filesystem::is_regular_file(status)) result.push_back(it->path());
            else if (!std::filesystem::is_directory(status))
                modelfehler(it->path().filename().u8string(), "Nicht regulaere Datei");
        }
        if (error) modelfehler(path.filename().u8string(), "Fingerprint-Bereich nicht vollstaendig lesbar");
    } else if (error) {
        modelfehler(path.filename().u8string(), "Fingerprint-Bereich nicht lesbar");
    }
    std::sort(result.begin(), result.end(), [&](const auto& a, const auto& b) {
        return std::filesystem::relative(a, modellordner).generic_u8string() <
            std::filesystem::relative(b, modellordner).generic_u8string();
    });
    return result;
}

std::string fingerprint(
    const std::filesystem::path& modellordner, ModellFingerprintBereich bereich)
{
    Sha256 digest;
    const auto files = dateien_im_bereich(modellordner, bereich);
    for (const auto& file : files) {
        const auto relative = std::filesystem::relative(file, modellordner).generic_u8string();
        std::error_code error;
        const auto size = std::filesystem::file_size(file, error);
        if (error) modelfehler(file.filename().u8string(), "Dateigroesse nicht lesbar");
        const auto modified = std::filesystem::last_write_time(file, error);
        if (error) modelfehler(file.filename().u8string(), "Dateizeit nicht lesbar");
        laenge(digest, static_cast<std::uint64_t>(relative.size()));
        digest.update(relative);
        laenge(digest, static_cast<std::uint64_t>(size));
        std::ifstream input(file, std::ios::binary);
        if (!input) modelfehler(file.filename().u8string(), "Datei nicht lesbar");
        std::array<unsigned char, 65536> buffer{};
        std::uint64_t read_count = 0;
        while (input) {
            input.read(reinterpret_cast<char*>(buffer.data()),
                static_cast<std::streamsize>(buffer.size()));
            const auto count = input.gcount();
            if (count > 0) {
                digest.update(buffer.data(), static_cast<std::size_t>(count));
                read_count += static_cast<std::uint64_t>(count);
            }
        }
        if (input.bad() || read_count != size)
            modelfehler(file.filename().u8string(), "Datei waehrend Fingerprint-Lesen geaendert");
        if (std::filesystem::last_write_time(file) != modified ||
            std::filesystem::file_size(file) != size)
            modelfehler(file.filename().u8string(), "Datei waehrend Fingerprint-Lesen geaendert");
    }
    if (dateien_im_bereich(modellordner, bereich) != files)
        modelfehler("fingerprints", "Dateibestand waehrend Fingerprint-Lesen geaendert");
    return digest.finish();
}

std::filesystem::path schema_pfad(const std::filesystem::path& schemaordner, const char* name)
{
    return schemaordner / name;
}

void pruefe_identitaet(const Json& data, const Json& validation,
                       const std::filesystem::path& model)
{
    if (!data.contains("identity") || !data["identity"].is_object() ||
        !validation.contains("identity") || !validation["identity"].is_object())
        modelfehler("identity", "Identitaetsobjekt fehlt");
    if (data["identity"] != validation["identity"])
        modelfehler("identity", "Identitaet in den Modelldateien stimmt nicht ueberein");
    const auto id = data["identity"].value("id", std::string{});
    if (id.empty() || id != model.filename().u8string())
        modelfehler("identity.id", "Modell-ID stimmt nicht mit dem Ordnernamen ueberein");
    if (!data.contains("schema") || !validation.contains("schema") ||
        data["schema"] != validation["schema"])
        modelfehler("schema", "Schema-Version oder Freigabestatus stimmt nicht ueberein");
}

std::vector<std::string> bereiche{
    "visualisation", "mass", "hydrostatics", "modelData"};

ModellFingerprintBereich parse_bereich(const std::string& name)
{
    if (name == "visualisation") return ModellFingerprintBereich::visualisation;
    if (name == "mass") return ModellFingerprintBereich::mass;
    if (name == "hydrostatics") return ModellFingerprintBereich::hydrostatics;
    if (name == "aerodynamics") return ModellFingerprintBereich::aerodynamics;
    if (name == "modelData") return ModellFingerprintBereich::modelData;
    throw std::runtime_error("CORE-MOD-0002: Unbekannter Fingerprint-Bereich: " + name);
}

class Schreibsperre {
public:
    explicit Schreibsperre(const std::filesystem::path& path) : path_(path)
    {
        std::error_code error;
        if (!std::filesystem::create_directory(path_, error))
            throw std::runtime_error("CORE-FPR-0002: Validierungsdatei wird bereits bearbeitet "
                "oder Schreibsperre nicht verfuegbar");
    }
    ~Schreibsperre()
    {
        if (active_) {
            std::error_code ignored;
            std::filesystem::remove(path_, ignored);
        }
    }
private:
    std::filesystem::path path_;
    bool active_ = true;
};

std::optional<std::string> umgebung(const char* name)
{
    std::string value;
#ifdef _MSC_VER
    std::size_t length = 0;
    if (getenv_s(&length, nullptr, 0, name) != 0 || length <= 1) return std::nullopt;
    value.resize(length);
    if (getenv_s(&length, value.data(), value.size(), name) != 0) return std::nullopt;
    value.resize(length - 1);
#else
    const auto* environment_value = std::getenv(name);
    if (!environment_value || !*environment_value) return std::nullopt;
    value = environment_value;
#endif
    if (value.empty()) return std::nullopt;
    return value;
}

std::filesystem::path absoluter_pfad(const std::string& value,
                                    const std::filesystem::path& basis,
                                    const std::string& feld)
{
    auto path = std::filesystem::u8path(value);
    if (path.empty()) modelfehler(feld, "Pfad darf nicht leer sein");
    if (path.is_relative()) path = basis / path;
    std::error_code error;
    path = std::filesystem::absolute(path, error);
    if (error) modelfehler(feld, "Pfad kann nicht aufgeloest werden");
    return path.lexically_normal();
}

std::filesystem::path defaults_pfad()
{
    if (const auto path = umgebung("SEGELPHYSIK_TEST_DEFAULTS_FILE"))
        return absoluter_pfad(*path, std::filesystem::current_path(), "defaults-Datei");
    if (const auto path = umgebung("SEGELPHYSIK_DEFAULTS_FILE"))
        return absoluter_pfad(*path, std::filesystem::current_path(), "defaults-Datei");
    const auto appdata = umgebung("APPDATA");
    if (!appdata) modelfehler("defaults.json", "APPDATA ist nicht gesetzt");
    return absoluter_pfad(*appdata + "\\SegelPhysik\\defaults.json",
        std::filesystem::current_path(), "defaults-Datei");
}

std::optional<Json> defaults_json_optional()
{
    const auto path = defaults_pfad();
    std::error_code error;
    if (!std::filesystem::exists(path, error)) {
        if (error) modelfehler("defaults.json", "Konfiguration nicht lesbar");
        return std::nullopt;
    }
    Json config;
    try {
        config = json_lesen(path);
    } catch (const std::exception& exception) {
        modelfehler("defaults.json", std::string("Ungueltige Konfiguration: ") + exception.what());
    }
    if (!config.is_object()) modelfehler("defaults.json", "Konfiguration muss ein Objekt sein");
    for (const auto* field : {"modelRoot", "schemaRoot"})
        if (!config.contains(field) || !config[field].is_string() ||
            config[field].get_ref<const std::string&>().empty())
            modelfehler(std::string("defaults.") + field, "Nicht leerer Pfad erforderlich");
    if (!config.contains("defaults") || !config["defaults"].is_object())
        modelfehler("defaults.defaults", "defaults muss ein Objekt sein");
    return config;
}

std::filesystem::path config_pfad(const Json& config, const char* field,
                                  const std::filesystem::path& config_file)
{
    if (!config.contains(field) || !config[field].is_string())
        modelfehler(std::string("defaults.") + field, "Absoluter oder relativer Pfad erforderlich");
    return absoluter_pfad(config[field].get<std::string>(), config_file.parent_path(),
        std::string("defaults.") + field);
}

void atomar_ersetzen(const std::filesystem::path& path, const std::string& contents)
{
    const auto temporary = path.parent_path() / ".modells_validation.lock" / "replacement.json";
    {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output) throw std::runtime_error("CORE-FPR-0003: Temporaere Validierungsdatei "
            "kann nicht erstellt werden");
        output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
        output.flush();
        if (!output) {
            std::error_code ignored;
            std::filesystem::remove(temporary, ignored);
            throw std::runtime_error("CORE-FPR-0003: Validierungsdatei kann nicht geschrieben werden");
        }
    }
#ifdef _WIN32
    if (!MoveFileExW(temporary.c_str(), path.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        throw std::runtime_error("CORE-FPR-0003: Validierungsdatei kann nicht atomar ersetzt werden");
    }
#else
    std::error_code error;
    std::filesystem::rename(temporary, path, error);
    if (error) {
        std::filesystem::remove(temporary);
        throw std::runtime_error("CORE-FPR-0003: Validierungsdatei kann nicht atomar ersetzt werden: "
            + error.message());
    }
#endif
}

} // namespace

bool ist_modellformat03_datei(const std::filesystem::path& datei)
{
    return datei.filename() == "modells_data.json";
}

Modellformat03Pruefung pruefe_modellformat03(
    const std::filesystem::path& modellordner, const std::filesystem::path& schemaordner,
    bool fingerprints)
{
    Modellformat03Pruefung result;
    const auto data_path = modellordner / "modells_data.json";
    const auto validation_path = modellordner / "modells_validation.json";
    const auto data_schema_path = schema_pfad(schemaordner, "schema_data.json");
    const auto validation_schema_path = schema_pfad(schemaordner, "schema_validation.json");
    Sha256 revision;
    const auto dokument = [&](const std::filesystem::path& path) {
        const auto bytes = lesen(path);
        const auto name = path.filename().u8string();
        laenge(revision, name.size());
        revision.update(name);
        laenge(revision, bytes.size());
        revision.update(bytes);
        return json_aus_bytes(bytes, name);
    };
    try {
        std::error_code model_status_error;
        const auto model_status =
            std::filesystem::symlink_status(modellordner, model_status_error);
        if (model_status_error || !std::filesystem::is_directory(model_status) ||
            std::filesystem::is_symlink(model_status))
            modelfehler("Modellordner", "Regulaerer Modellordner ohne Symlink erforderlich");
        std::error_code legacy_status_error;
        if (std::filesystem::exists(modellordner / "modells_information.json", legacy_status_error))
            modelfehler("modells_information.json",
                "Legacy- und Modellformat-0.3-Dateien duerfen nicht gleichzeitig vorliegen");
        result.daten = dokument(data_path);
        result.validierung = dokument(validation_path);
        const auto data_schema = dokument(data_schema_path);
        const auto validation_schema = dokument(validation_schema_path);
        SchemaPruefer data_validator(data_schema), validation_validator(validation_schema);
        const auto data_errors = data_validator.pruefe(result.daten);
        const auto validation_errors =
            validation_validator.pruefe(result.validierung);
        for (const auto& schema_error : data_errors)
            result.fehler.push_back("CORE-MOD-0002 [modells_data.json]: " + schema_error);
        for (const auto& schema_error : validation_errors)
            result.fehler.push_back("CORE-MOD-0002 [modells_validation.json]: " + schema_error);
        if (result.fehler.empty()) {
            pruefe_identitaet(result.daten, result.validierung, modellordner);
            pruefe_ressourcen(result.daten, modellordner);
            result.schemas_gueltig = true;
        }
        if (fingerprints && result.schemas_gueltig) {
            const auto& stored = result.validierung["model"]["fingerprints"];
            for (const auto& name : bereiche) {
                try {
                    const auto expected = stored[name].value("hash", std::string{});
                    const auto actual = fingerprint(modellordner, parse_bereich(name));
                    result.berechnete_hashes.emplace(name, actual);
                    if (expected != actual) result.veraltete_bereiche.push_back(name);
                } catch (const std::exception& exception) {
                    result.fehler.push_back(exception.what());
                    result.fehler_bereiche.push_back(name);
                }
            }
        }
    } catch (const std::exception& exception) {
        const std::string message = exception.what();
        result.fehler.push_back(message.rfind("CORE-", 0) == 0 ? message :
            "CORE-MOD-0002 [Modellformat 0.3]: " + message);
    }
    for (const auto& [name, hash] : result.berechnete_hashes) {
        revision.update(name);
        revision.update(hash);
    }
    for (const auto& error : result.fehler) revision.update(error);
    result.fassung = revision.finish();
    return result;
}

Json normalisiere_modellformat03(const Json& daten, const std::filesystem::path& modellordner)
{
    if (!daten.is_object() || !daten.contains("identity") || !daten["identity"].is_object() ||
        !daten.contains("groups") || !daten["groups"].is_object())
        modelfehler("modells_data.json", "Modellformat 0.3-Struktur unvollstaendig");
    Json result{
        {"schema_version", "0.1-entwurf"},
        {"id", daten["identity"].value("id", std::string{})},
        {"name", daten["identity"].value("name", std::string{})},
        {"gruppen", Json::object()}
    };
    const auto& groups = daten["groups"];
    for (auto group = groups.begin(); group != groups.end(); ++group) {
        if (!group.value().is_object() || !group.value().contains("elements") ||
            !group.value()["elements"].is_array())
            modelfehler("groups." + group.key(), "elements-Array fehlt");
        Json converted = Json::array();
        for (const auto& source : group.value()["elements"]) {
            if (!source.is_object() || !source.contains("id") || !source["id"].is_string())
                modelfehler("groups." + group.key(), "Element-ID fehlt");
            Json element{{"id", source["id"]}};
            element["darstellung_verborgen"] = source.at("display").at("hidden");
            if (source.contains("display") && source["display"].is_object() &&
                !source["display"].value("hidden", false)) {
                const auto& display = source["display"];
                Json item = Json::object();
                if (display.contains("geometry")) item["datei"] = display["geometry"];
                if (display.contains("color")) item["farbe"] = display["color"];
                if (display.contains("surface")) {
                    static const std::map<std::string, std::string> surfaces{
                        {"matte", "matt"}, {"glossy", "glaenzend"}, {"metal", "metall"},
                        {"chrome", "chrom"}, {"paint", "lack"}, {"glass", "glas"},
                        {"glowing", "leuchtend"}, {"iridescent", "schillernd"},
                        {"wireframe", "drahtgitter"}, {"faceted", "facettiert"},
                        {"emissive", "leuchtend"}, {"motherOfPearl", "perlmutt"},
                        {"invisible", "unsichtbar"}, {"fabric", "stoff"}, {"wood", "holz"},
                        {"brushedMetal", "metall_gebuerstet"}};
                    const auto it = surfaces.find(display["surface"].get<std::string>());
                    if (it == surfaces.end())
                        modelfehler("display.surface", "Unbekannte Oberflaeche");
                    item["oberflaeche"] = it->second;
                }
                if (!item.empty()) element["Darstellung"] = std::move(item);
            }
            const Json* physics = source.contains("physics") && source["physics"].is_object()
                ? &source["physics"] : nullptr;
            if (physics) {
                Json hydro = Json::object();
                if (physics->contains("mass") && (*physics)["mass"].is_object()) {
                    const auto& mass = (*physics)["mass"];
                    const auto type = mass.value("type", std::string{});
                    Json converted_mass = Json::object();
                    if (type == "pointMass") {
                        converted_mass["art"] = "punktmasse";
                        converted_mass["masse_kg"] = mass.value("mass_kg", 0.0);
                        converted_mass["position_m"] = mass.value("position_m", Json::array());
                    } else if (type == "solid") {
                        if (mass.contains("material"))
                            modelfehler("physics.mass.material",
                                "Materialreferenzen sind ohne validierten Materialkatalog nicht verwendbar");
                        converted_mass["art"] = "vollkoerper";
                        converted_mass["dichte_kg_pro_m3"] = mass.value("density_kg_per_m3", 0.0);
                        if (mass.contains("geometry")) converted_mass["datei"] = mass["geometry"];
                    } else if (type == "shell") {
                        if (mass.contains("material"))
                            modelfehler("physics.mass.material",
                                "Materialreferenzen sind ohne validierten Materialkatalog nicht verwendbar");
                        converted_mass["art"] = "schale";
                        converted_mass["dichte_kg_pro_m3"] = mass.value("density_kg_per_m3", 0.0);
                        converted_mass["wanddicke_m"] = mass.value("thickness_m", 0.0);
                        if (mass.contains("geometry")) converted_mass["datei"] = mass["geometry"];
                    } else {
                        modelfehler("physics.mass.type", "Nicht unterstuetzte Massenart: " + type);
                    }
                    hydro["masse"] = std::move(converted_mass);
                }
                if (physics->contains("hydrostatic") && (*physics)["hydrostatic"].is_object() &&
                    (*physics)["hydrostatic"].contains("geometry"))
                    hydro["volumen"] = {{"datei", (*physics)["hydrostatic"]["geometry"]}};
                if (!hydro.empty()) element["Hydrostatik"] = std::move(hydro);

                if (physics->contains("hydrodynamic") &&
                    (*physics)["hydrodynamic"].is_object() &&
                    (*physics)["hydrodynamic"].contains("role")) {
                    const auto role = (*physics)["hydrodynamic"]["role"].get<std::string>();
                    if (role == "body" || role == "hull")
                        element["Hydrodynamik"] = {{"rolle", "rumpf"}};
                    else if (role == "keel")
                        element["Hydrodynamik"] = {{"rolle", "kiel"}};
                    else if (role == "rudder")
                        element["Hydrodynamik"] = {{"rolle", "ruder"}};
                }
                if (physics->contains("aerodynamic") && (*physics)["aerodynamic"].is_object()) {
                    const auto& aero = (*physics)["aerodynamic"];
                    if (aero.contains("definition"))
                        element["Segel"] = {{"quelle_datei", aero["definition"]}};
                    if (aero.contains("lookup"))
                        element["Segel"]["lookup_datei"] = aero["lookup"];
                }
            }
            if (source.contains("placement") && source["placement"].is_object()) {
                const auto& placement = source["placement"];
                element["placement"] = placement;
                element["setting"] = {
                    {"translation_m", placement.value("translation_m", Json::array({0, 0, 0}))},
                    {"rotation_xyzw", placement.value("rotation_xyzw", Json::array({0, 0, 0, 1}))},
                    {"konfiguriert", true}
                };
                if (placement.contains("originalTack")) {
                    static const std::map<std::string, std::string> tack{
                        {"unknown", "unbekannt"}, {"port", "backbord"},
                        {"starboard", "steuerbord"}};
                    const auto value = placement.at("originalTack").get<std::string>();
                    const auto found = tack.find(value);
                    if (found == tack.end())
                        modelfehler("placement.originalTack", "Unbekannte Buglage");
                    element["setting"]["original_buglage"] = found->second;
                }
            }
            converted.push_back(std::move(element));
        }
        if (group.value().contains("movement") && group.value()["movement"].is_object()) {
            const auto& movement = group.value()["movement"];
            const auto type = movement.value("type", std::string{});
            Json converted_movement = Json::object();
            if (type == "rotate") {
                converted_movement = {
                    {"art", "drehen"},
                    {"achse_modell", movement.value("axisModel", Json::array())},
                    {"drehpunkt_modell_m", movement.value("pivotPointModelM", Json::array())}
                };
            } else if (type == "translate") {
                converted_movement = {
                    {"art", "heben"},
                    {"richtung_modell", movement.value("directionModel", Json::array())}
                };
            } else {
                modelfehler("groups." + group.key() + ".movement.type",
                    "Nicht unterstuetzte Bewegungsart: " + type);
            }
            std::size_t controller = converted.size();
            for (std::size_t i = 0; i < group.value()["elements"].size(); ++i) {
                const auto& source = group.value()["elements"][i];
                if (!source.is_object() || !source.contains("physics") ||
                    !source["physics"].is_object() ||
                    !source["physics"].contains("hydrostatic")) continue;
                const auto& hydro = source["physics"]["hydrostatic"];
                if (hydro.is_object() && hydro.contains("geometry")) {
                    controller = i;
                    break;
                }
            }
            if (controller == converted.size()) {
                if (converted.empty())
                    modelfehler("groups." + group.key() + ".movement",
                        "Bewegungsgruppe benoetigt mindestens ein Element");
                controller = 0;
            }
            Json moving = Json::array();
            for (std::size_t i = 0; i < group.value()["elements"].size(); ++i)
                if (i != controller && group.value()["elements"][i].contains("id"))
                    moving.push_back(group.value()["elements"][i]["id"]);
            if (!moving.empty()) converted_movement["mitbewegte_elemente"] = std::move(moving);
            converted[controller]["Bewegung"] = std::move(converted_movement);
        }
        result["gruppen"][group.key()] = {{"elemente", std::move(converted)}};
    }
    (void)modellordner;
    return result;
}

std::string modellformat03_fassung(const Modellformat03Pruefung& pruefung)
{
    return pruefung.fassung;
}

} // namespace segelphysik::detail

namespace segelphysik {

std::filesystem::path laufzeit_defaults_datei()
{
    return detail::defaults_pfad();
}

std::filesystem::path modellordner_standard(const std::filesystem::path& expliziter_ordner)
{
    if (!expliziter_ordner.empty())
        return detail::absoluter_pfad(expliziter_ordner.u8string(),
            std::filesystem::current_path(), "modelRoot");
    if (const auto path = detail::umgebung("SEGELPHYSIK_TEST_MODEL_ROOT"))
        return detail::absoluter_pfad(*path, std::filesystem::current_path(),
            "SEGELPHYSIK_TEST_MODEL_ROOT");
    if (const auto path = detail::umgebung("SEGELPHYSIK_MODEL_ROOT"))
        return detail::absoluter_pfad(*path, std::filesystem::current_path(),
            "SEGELPHYSIK_MODEL_ROOT");
    const auto config_file = detail::defaults_pfad();
    const auto config = detail::defaults_json_optional();
    if (!config) detail::modelfehler("defaults.json", "Konfigurationsdatei fehlt");
    return detail::config_pfad(*config, "modelRoot", config_file);
}

std::filesystem::path schemaordner_fuer_modellordner(
    const std::filesystem::path&, const std::filesystem::path& expliziter_schemaordner)
{
    if (!expliziter_schemaordner.empty())
        return detail::absoluter_pfad(expliziter_schemaordner.u8string(),
            std::filesystem::current_path(), "schemaRoot");
    if (const auto path = detail::umgebung("SEGELPHYSIK_TEST_SCHEMA_ROOT"))
        return detail::absoluter_pfad(*path, std::filesystem::current_path(),
            "SEGELPHYSIK_TEST_SCHEMA_ROOT");
    if (const auto path = detail::umgebung("SEGELPHYSIK_SCHEMA_ROOT"))
        return detail::absoluter_pfad(*path, std::filesystem::current_path(),
            "SEGELPHYSIK_SCHEMA_ROOT");
    const auto config_file = detail::defaults_pfad();
    const auto config = detail::defaults_json_optional();
    if (!config) detail::modelfehler("defaults.json", "Konfigurationsdatei fehlt");
    return detail::config_pfad(*config, "schemaRoot", config_file);
}

ModellFingerprintErgebnis aktualisiere_modell_fingerprints(
    const std::filesystem::path& modellordner,
    const std::vector<ModellFingerprintBereich>& requested)
{
    if (requested.empty())
        throw std::runtime_error("CORE-MOD-0002: Mindestens ein Fingerprint-Bereich erforderlich");
    std::set<std::string> unique;
    for (const auto area : requested) {
        const auto name = detail::bereich_name(area);
        if (!unique.insert(name).second)
            throw std::runtime_error("CORE-MOD-0002: Fingerprint-Bereich doppelt angegeben: " + name);
    }
    const auto schemaordner = schemaordner_fuer_modellordner(modellordner);
    detail::Schreibsperre lock(modellordner / ".modells_validation.lock");
    const auto original = detail::lesen(modellordner / "modells_validation.json");
    const auto original_data = detail::lesen(modellordner / "modells_data.json");
    const auto original_data_schema = detail::lesen(schemaordner / "schema_data.json");
    const auto original_validation_schema = detail::lesen(schemaordner / "schema_validation.json");
    auto check = detail::pruefe_modellformat03(modellordner, schemaordner, false);
    if (!check.schemas_gueltig)
        throw std::runtime_error(check.fehler.empty()
            ? "CORE-MOD-0002: Schema-Pruefung fehlgeschlagen" : check.fehler.front());
    if (nlohmann::json::parse(original) != check.validierung ||
        nlohmann::json::parse(original_data) != check.daten)
        throw std::runtime_error("CORE-FPR-0002: Parallele Aenderung der Modelldateien erkannt");
    const auto& validation = check.validierung;
    auto temporary = validation;
    const auto modified = detail::modified_at();
    ModellFingerprintErgebnis result;
    for (const auto area : requested) {
        const auto name = detail::bereich_name(area);
        const auto hash = detail::fingerprint(modellordner, area);
        auto& entry = temporary["model"]["fingerprints"][name];
        const auto previous = entry.value("hash", std::string{});
        if (previous != hash) {
            entry["hash"] = hash;
            entry["modifiedAt"] = modified;
        }
        result.werte.push_back({area, hash, entry.value("modifiedAt", modified)});
    }
    const auto schema = detail::json_lesen(schemaordner / "schema_validation.json");
    detail::SchemaPruefer validator(schema);
    const auto output_errors = validator.pruefe(temporary);
    if (!output_errors.empty())
        throw std::runtime_error("CORE-MOD-0002 [modells_validation.json]: "
            + output_errors.front());
    for (const auto& value : result.werte)
        if (detail::fingerprint(modellordner, value.bereich) != value.hash)
            throw std::runtime_error("CORE-FPR-0002: Parallele Aenderung der Ressourcen erkannt");
    if (detail::lesen(modellordner / "modells_validation.json") != original ||
        detail::lesen(modellordner / "modells_data.json") != original_data ||
        detail::lesen(schemaordner / "schema_data.json") != original_data_schema ||
        detail::lesen(schemaordner / "schema_validation.json") != original_validation_schema)
        throw std::runtime_error("CORE-FPR-0002: Parallele Aenderung der Modell- oder Schemadateien erkannt");
    if (temporary == validation) return result;
    auto path = modellordner / "modells_validation.json";
    detail::atomar_ersetzen(path, temporary.dump(2) + "\n");
    result.validierungsdatei_aktualisiert = true;
    return result;
}

} // namespace segelphysik
