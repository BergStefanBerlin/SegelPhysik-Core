#include "segelphysik/modell_fingerprints.hpp"
#include "segelphysik/defaults.hpp"
#include "segelphysik/katalog.hpp"
#include "segelphysik/inspektion.hpp"
#include "segelphysik/core.hpp"
#include "../src/modellformat_03_intern.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void pruefung(bool condition, const std::string& text)
{
    if (!condition) throw std::runtime_error(text);
}

std::string lesen(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

void schreiben(const std::filesystem::path& path, const std::string& value)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(value.data(), static_cast<std::streamsize>(value.size()));
    pruefung(output.good(), "Testdatei konnte nicht geschrieben werden");
}

void pruefe_420_bewegungsvorschau(const segelphysik::ModelInspection& inspection)
{
    const auto mainsail = segelphysik::getMovementPreview(inspection, "Mainsail");
    const auto jib = segelphysik::getMovementPreview(inspection, "Jib");
    pruefung(mainsail && jib, "420-Segelgruppen liefern unabhaengige Bewegungsvorschauen");
    pruefung(!segelphysik::getMovementPreview(inspection, "Boom"),
        "Mitbewegter Grossbaum wird kein eigener Bewegungscontroller");
    pruefung(mainsail->kind == "drehen" && mainsail->axis_point_m
            && std::abs((*mainsail->axis_point_m)[0] - 0.85070675) < 1e-8
            && std::abs(mainsail->axis_direction[2] - 1.0) < 1e-8,
        "Grosssegelgruppe behaelt Mastachse und Drehpunkt");
    pruefung(jib->kind == "drehen" && jib->axis_point_m
            && std::abs((*jib->axis_point_m)[0] - 2.00570675) < 1e-8
            && std::abs((*jib->axis_point_m)[2] - 0.616428) < 1e-8
            && std::abs(jib->axis_direction[0] - (-0.2776051021)) < 1e-8
            && std::abs(jib->axis_direction[2] - 0.9606952729) < 1e-8,
        "Fockgruppe behaelt Vorstagachse und Drehpunkt");

    const auto verify_group = [](const segelphysik::InspectionMovementPreview& preview,
                                  const std::set<std::string>& expected,
                                  const std::string& label) {
        const auto minimum = std::find_if(preview.poses.begin(), preview.poses.end(),
            [](const auto& pose) { return pose.label == "minimum"; });
        const auto maximum = std::find_if(preview.poses.begin(), preview.poses.end(),
            [](const auto& pose) { return pose.label == "maximum"; });
        pruefung(minimum != preview.poses.end() && maximum != preview.poses.end(),
            label + ": Minimum-/Maximum-Vorschau fehlt");
        const auto meshes_by_element = [](const auto& pose) {
            std::map<std::string, const segelphysik::InspectionMesh*> result;
            for (const auto& mesh : pose.meshes) result.emplace(mesh.element_id, &mesh);
            return result;
        };
        const auto minimum_meshes = meshes_by_element(*minimum);
        const auto maximum_meshes = meshes_by_element(*maximum);
        std::set<std::string> actual;
        for (const auto& [id, mesh] : minimum_meshes) {
            actual.insert(id);
            pruefung(!mesh->points_m.empty(), label + ": Vorschau-Mesh ist leer: " + id);
        }
        pruefung(actual == expected && maximum_meshes.size() == expected.size(),
            label + ": Vorschau enthaelt nicht exakt die Gruppenmitglieder");
        for (const auto& id : expected) {
            const auto before = minimum_meshes.at(id)->points_m;
            const auto after = maximum_meshes.at(id)->points_m;
            pruefung(before.size() == after.size(),
                label + ": Geometriepunktzahl aendert sich fuer " + id);
            const bool moved = std::equal(before.begin(), before.end(), after.begin(),
                [](const auto& a, const auto& b) {
                    return std::abs(a[0] - b[0]) < 1e-8
                        && std::abs(a[1] - b[1]) < 1e-8
                        && std::abs(a[2] - b[2]) < 1e-8;
                });
            pruefung(!moved, label + ": Bewegung erreicht Gruppenmitglied " + id + " nicht");
        }
    };
    verify_group(*mainsail, {"Boom", "Mainsail"}, "Grosssegelgruppe");
    verify_group(*jib, {"Jib"}, "Fockgruppe");
}

class Umgebung {
public:
    explicit Umgebung(const char* name) : name_(name)
    {
#ifdef _MSC_VER
        char* value = nullptr;
        std::size_t size = 0;
        pruefung(_dupenv_s(&value, &size, name) == 0, "Testumgebung nicht lesbar");
        if (value) previous_ = value;
        std::free(value);
#else
        if (const auto* value = std::getenv(name)) previous_ = value;
#endif
    }
    ~Umgebung()
    {
        setzen(previous_.value_or(""));
    }
    void setzen(const std::string& value) const
    {
#ifdef _WIN32
        pruefung(_putenv_s(name_.c_str(), value.c_str()) == 0, "Testumgebung nicht setzbar");
#else
        pruefung((value.empty() ? unsetenv(name_.c_str()) :
            setenv(name_.c_str(), value.c_str(), 1)) == 0, "Testumgebung nicht setzbar");
#endif
    }
private:
    std::string name_;
    std::optional<std::string> previous_;
};

template<class Aktion>
void muss_fehlschlagen(Aktion aktion)
{
    bool rejected = false;
    try { aktion(); }
    catch (const std::runtime_error& error) {
        const std::string message = error.what();
        const auto id = message.substr(0, message.find_first_of(" [:"));
        const auto catalog = nlohmann::json::parse(segelphysik::fehlerdokumentation_json());
        for (const auto& entry : catalog.at("fehler"))
            if (entry.at("id") == id) rejected = true;
    }
    pruefung(rejected, "Fehlende Core-Diagnose fuer ungueltige Eingabe");
}

void pruefe_defaults(const std::filesystem::path& root)
{
    using nlohmann::json;
    Umgebung file("SEGELPHYSIK_TEST_DEFAULTS_FILE"), ordinary_file("SEGELPHYSIK_DEFAULTS_FILE");
    Umgebung model("SEGELPHYSIK_MODEL_ROOT"), test_model("SEGELPHYSIK_TEST_MODEL_ROOT");
    Umgebung schema("SEGELPHYSIK_SCHEMA_ROOT"), test_schema("SEGELPHYSIK_TEST_SCHEMA_ROOT");
    model.setzen(""); test_model.setzen(""); schema.setzen(""); test_schema.setzen("");
    const auto config_file = root / "defaults.json";
    file.setzen(config_file.u8string());
    muss_fehlschlagen([] { (void)segelphysik::modellordner_standard(); });
    muss_fehlschlagen([&] { (void)segelphysik::schemaordner_fuer_modellordner(root); });
    schreiben(config_file, "{");
    muss_fehlschlagen([] { (void)segelphysik::modellordner_standard(); });
    json config{{"modelRoot", "models"}, {"schemaRoot", "schemas"},
        {"defaults", {{"additionalSetting", true}}}};
    schreiben(config_file, config.dump());
    pruefung(segelphysik::modellordner_standard() == root / "models", "Relativer Modellpfad");
    pruefung(segelphysik::schemaordner_fuer_modellordner(root) == root / "schemas",
        "Relativer Schemapfad");
    config["modelRoot"] = (root / "changed").u8string();
    schreiben(config_file, config.dump());
    pruefung(segelphysik::modellordner_standard() == root / "changed", "Defaults nicht neu gelesen");
    model.setzen((root / "environment").u8string());
    schema.setzen((root / "schema-environment").u8string());
    pruefung(segelphysik::modellordner_standard() == root / "environment", "Umgebungsprioritaet");
    test_model.setzen((root / "test").u8string());
    test_schema.setzen((root / "schema-test").u8string());
    pruefung(segelphysik::modellordner_standard() == root / "test", "Testprioritaet");
    pruefung(segelphysik::schemaordner_fuer_modellordner(root) == root / "schema-test",
        "Schema-Testprioritaet");
    pruefung(segelphysik::modellordner_standard(root / "explicit") == root / "explicit",
        "Explizite Modellprioritaet");
    pruefung(segelphysik::schemaordner_fuer_modellordner(root, root / "explicit-schema")
        == root / "explicit-schema", "Explizite Schemaprioritaet");
    model.setzen(""); test_model.setzen(""); schema.setzen(""); test_schema.setzen("");
    config.erase("defaults");
    schreiben(config_file, config.dump());
    muss_fehlschlagen([] { (void)segelphysik::modellordner_standard(); });
    file.setzen("");
    ordinary_file.setzen(config_file.u8string());
    muss_fehlschlagen([] { (void)segelphysik::modellordner_standard(); });
}

void pruefe_ungueltige_modelle(const std::filesystem::path& model,
                              const std::filesystem::path& schemas)
{
    const auto path = model / "modells_data.json";
    const auto original = lesen(path);
    const auto validation_path = model / "modells_validation.json";
    const auto validation = lesen(validation_path);
    const auto modified = std::filesystem::last_write_time(validation_path);
    const auto reject = [&] {
        const auto check = segelphysik::detail::pruefe_modellformat03(model, schemas, false);
        pruefung(!check.schemas_gueltig && !check.fehler.empty(),
            "Ungueltige Modelldatei wurde freigegeben");
        pruefung(check.fehler.front().rfind("CORE-MOD-", 0) == 0,
            "Formatfehler ohne Core-Diagnose");
        pruefung(lesen(validation_path) == validation &&
            std::filesystem::last_write_time(validation_path) == modified,
            "Fehlerpruefung hat Validierungsdatei geaendert");
    };
    auto data = nlohmann::json::parse(original);
    data["identity"]["name"] = "Different identity";
    schreiben(path, data.dump()); reject();
    data = nlohmann::json::parse(original);
    data["schema"]["schema_version"] = "0.4";
    schreiben(path, data.dump()); reject();
    data = nlohmann::json::parse(original);
    data["groups"]["body"]["elements"][0]["display"]["geometry"] =
        "modells/visualisation/missing.obj";
    schreiben(path, data.dump()); reject();
    data["groups"]["body"]["elements"][0]["display"]["geometry"] =
        "../outside.obj";
    schreiben(path, data.dump()); reject();
    schreiben(path, "{"); reject();
    schreiben(path, "{\"schema\": {}, \"schema\": {}}"); reject();
    std::filesystem::remove(path); reject();
    schreiben(path, original);
    schreiben(model / "modells_information.json", "{}"); reject();
    std::filesystem::remove(model / "modells_information.json");
    auto validation_data = nlohmann::json::parse(validation);
    validation_data["identity"]["id"] = "different";
    schreiben(validation_path, validation_data.dump());
    const auto mismatch = segelphysik::detail::pruefe_modellformat03(model, schemas, false);
    pruefung(!mismatch.schemas_gueltig, "Dateiuebergreifende Identitaet nicht geprueft");
    schreiben(validation_path, validation);
}

void pruefe_schema_fehlermatrix(const std::filesystem::path& source_root)
{
    using nlohmann::json;
    const auto matrix_root = std::filesystem::temp_directory_path() /
        ("segelphysik-modellformat03-matrix-" +
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Bereinigung {
        std::filesystem::path path;
        ~Bereinigung() { std::error_code error; std::filesystem::remove_all(path, error); }
    } cleanup{matrix_root};
    std::filesystem::create_directories(matrix_root);
    std::size_t case_count = 0;
    const auto reject = [&](const std::string& name,
                            const std::function<void(const std::filesystem::path&,
                                const std::filesystem::path&)>& mutate,
                            const std::string& folder = "bleistift") {
        const auto case_root = matrix_root / name;
        const auto case_schemas = case_root / "schemas";
        const auto case_model = case_root / folder;
        std::filesystem::create_directories(case_schemas);
        std::filesystem::copy(source_root / "schema_data.json",
            case_schemas / "schema_data.json");
        std::filesystem::copy(source_root / "schema_validation.json",
            case_schemas / "schema_validation.json");
        std::filesystem::copy(source_root / "bleistift", case_model,
            std::filesystem::copy_options::recursive);
        mutate(case_model, case_schemas);
        const auto validation_path = case_model / "modells_validation.json";
        const auto validation_before = std::filesystem::exists(validation_path)
            ? lesen(validation_path) : std::string{};
        const auto check =
            segelphysik::detail::pruefe_modellformat03(case_model, case_schemas, false);
        pruefung(!check.schemas_gueltig && !check.fehler.empty(),
            "Fehlerfall wurde nicht durch Core abgelehnt: " + name);
        pruefung(check.fehler.front().rfind("CORE-MOD-", 0) == 0,
            "Fehlerfall ohne Core-Diagnose: " + name);
        if (std::filesystem::exists(validation_path))
            pruefung(lesen(validation_path) == validation_before,
                "Schema-Pruefung hat Validierungsdatei geaendert: " + name);

        Umgebung schema_root("SEGELPHYSIK_TEST_SCHEMA_ROOT");
        schema_root.setzen(case_schemas.u8string());
        const auto catalog = segelphysik::modellkatalog(case_root);
        const auto* entry = catalog.eintrag(folder);
        if (entry) {
            pruefung(!entry->schema_valide && entry->status != "verwendbar",
                "Ungueltiges Modell wurde im Katalog freigegeben: " + name);
            pruefung(!segelphysik::modelldiagnose(catalog, folder,
                entry->modell_fassung).empty(), "Katalogdiagnose fehlt: " + name);
            muss_fehlschlagen([&] {
                (void)segelphysik::readModelInspection(
                    catalog, folder, entry->modell_fassung);
            });
        }
        ++case_count;
    };

    reject("missing-data", [](const auto& model, const auto&) {
        std::filesystem::remove(model / "modells_data.json");
    });
    reject("missing-validation", [](const auto& model, const auto&) {
        std::filesystem::remove(model / "modells_validation.json");
    });
    reject("malformed-data", [](const auto& model, const auto&) {
        schreiben(model / "modells_data.json", "{");
    });
    reject("duplicate-json-key", [](const auto& model, const auto&) {
        schreiben(model / "modells_data.json", "{\"schema\": {}, \"schema\": {}}");
    });
    reject("malformed-validation", [](const auto& model, const auto&) {
        schreiben(model / "modells_validation.json", "{");
    });
    reject("duplicate-validation-key", [](const auto& model, const auto&) {
        schreiben(model / "modells_validation.json",
            "{\"identity\": {}, \"identity\": {}}");
    });
    reject("unknown-format-version", [](const auto& model, const auto&) {
        auto data = json::parse(lesen(model / "modells_data.json"));
        data["schema"]["schema_version"] = "0.4";
        schreiben(model / "modells_data.json", data.dump());
    });
    reject("data-schema-violation", [](const auto& model, const auto&) {
        auto data = json::parse(lesen(model / "modells_data.json"));
        data["identity"]["name"] = 42;
        schreiben(model / "modells_data.json", data.dump());
    });
    reject("missing-data-schema", [](const auto&, const auto& schemas) {
        std::filesystem::remove(schemas / "schema_data.json");
    });
    reject("invalid-data-schema", [](const auto&, const auto& schemas) {
        schreiben(schemas / "schema_data.json",
            R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":"unknown-type"})");
    });
    reject("unsupported-schema-dialect", [](const auto&, const auto& schemas) {
        auto schema = json::parse(lesen(schemas / "schema_data.json"));
        schema["$schema"] = "http://json-schema.org/draft-07/schema#";
        schreiben(schemas / "schema_data.json", schema.dump());
    });
    reject("missing-validation-schema-dialect", [](const auto&, const auto& schemas) {
        auto schema = json::parse(lesen(schemas / "schema_validation.json"));
        schema.erase("$schema");
        schreiben(schemas / "schema_validation.json", schema.dump());
    });
    reject("missing-validation-schema", [](const auto&, const auto& schemas) {
        std::filesystem::remove(schemas / "schema_validation.json");
    });
    reject("invalid-validation-schema", [](const auto&, const auto& schemas) {
        schreiben(schemas / "schema_validation.json",
            R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":"unknown-type"})");
    });
    reject("unsupported-validation-schema-dialect", [](const auto&, const auto& schemas) {
        auto schema = json::parse(lesen(schemas / "schema_validation.json"));
        schema["$schema"] = "http://json-schema.org/draft-07/schema#";
        schreiben(schemas / "schema_validation.json", schema.dump());
    });
    reject("validation-schema-violation", [](const auto& model, const auto&) {
        auto validation = json::parse(lesen(model / "modells_validation.json"));
        validation["schema"]["schema_version"] = 0.3;
        schreiben(model / "modells_validation.json", validation.dump());
    });
    reject("validation-schema-version-mismatch", [](const auto& model, const auto&) {
        auto validation = json::parse(lesen(model / "modells_validation.json"));
        validation["schema"]["schema_version"] = "0.4";
        schreiben(model / "modells_validation.json", validation.dump());
    });
    reject("identity-mismatch", [](const auto& model, const auto&) {
        auto validation = json::parse(lesen(model / "modells_validation.json"));
        validation["identity"]["id"] = "different";
        schreiben(model / "modells_validation.json", validation.dump());
    });
    reject("folder-identity-mismatch", [](const auto&, const auto&) {}, "not-bleistift");
    reject("missing-resource", [](const auto& model, const auto&) {
        auto data = json::parse(lesen(model / "modells_data.json"));
        data["groups"]["body"]["elements"][0]["display"]["geometry"] =
            "modells/visualisation/missing.obj";
        schreiben(model / "modells_data.json", data.dump());
    });
    reject("resource-path-escape", [](const auto& model, const auto&) {
        auto data = json::parse(lesen(model / "modells_data.json"));
        data["groups"]["body"]["elements"][0]["display"]["geometry"] = "../outside.obj";
        schreiben(model / "modells_data.json", data.dump());
    });
    reject("legacy-file-conflict", [](const auto& model, const auto&) {
        schreiben(model / "modells_information.json", "{}");
    });
    std::cout << case_count << " Schema-/Importfehler: Core-Diagnose, Katalogsperre, "
        "Inspektionssperre und unveraenderte Validierung nachgewiesen.\n";
}

}

int main(int argc, char** argv)
{
    if (argc == 3 && std::string(argv[1]) == "--schema-error-matrix") {
        try {
            pruefe_schema_fehlermatrix(std::filesystem::path(argv[2]));
            return 0;
        } catch (const std::exception& error) {
            std::cerr << error.what() << '\n';
            return 1;
        }
    }
    if (argc == 3 && std::string(argv[1]) == "--source-integration") {
        try {
            const std::filesystem::path root(argv[2]);
            Umgebung schema_root("SEGELPHYSIK_TEST_SCHEMA_ROOT");
            schema_root.setzen(root.u8string());
            const auto catalog = segelphysik::modellkatalog(root);
            pruefung(catalog.eintraege().size() == 11,
                "Nicht genau elf Modelle im 0.3-Katalog");
            for (const auto& entry : catalog.eintraege()) {
                if (!entry.schema_valide || !entry.fingerprints_aktuell ||
                    entry.status != "verwendbar") {
                    std::string message = "Modell nicht fuer Physik freigegeben: " +
                        entry.modell_id + " (status=" + entry.status + ", schema=" +
                        (entry.schema_valide ? "valid" : "invalid") + ", fingerprints=" +
                        (entry.fingerprints_aktuell ? "current" : "stale") + ")";
                    for (const auto& diagnostic : segelphysik::modelldiagnose(
                        catalog, entry.modell_id, entry.modell_fassung))
                        message += " [" + diagnostic.fehler_id + "] " + diagnostic.text;
                    throw std::runtime_error(message);
                }
                const auto inspection = segelphysik::readModelInspection(
                    catalog, entry.modell_id, entry.modell_fassung);
                pruefung(inspection->info().model_id == entry.modell_id,
                    "Inspektion hat eine falsche Modell-ID");
                if (inspection->info().status != "verwendbar") {
                    std::string message = entry.modell_id +
                        ": keine verwendbare Inspektion";
                    for (const auto& diagnostic : inspection->info().diagnostics)
                        message += " [" + diagnostic.fehler_id + "] " + diagnostic.text;
                    throw std::runtime_error(message);
                }
                if (entry.modell_id == "420")
                    pruefe_420_bewegungsvorschau(*inspection);
                std::cout << entry.modell_id << ": Katalog, Physikimport und Inspektion "
                    << inspection->info().status << '\n';
            }
            return 0;
        } catch (const std::exception& error) {
            std::cerr << error.what() << '\n';
            return 1;
        }
    }
    if (argc == 3 && std::string(argv[1]) == "--source-schemas") {
        try {
            const std::filesystem::path root(argv[2]);
            std::size_t count = 0;
            for (const auto& entry : std::filesystem::directory_iterator(root)) {
                if (!entry.is_directory() ||
                    !std::filesystem::exists(entry.path() / "modells_data.json")) continue;
                const auto validation_path = entry.path() / "modells_validation.json";
                const auto before = lesen(validation_path);
                const auto check =
                    segelphysik::detail::pruefe_modellformat03(entry.path(), root, false);
                if (!check.schemas_gueltig)
                    throw std::runtime_error(entry.path().filename().u8string() + ": " +
                        (check.fehler.empty() ? "Schema-Pruefung fehlgeschlagen" :
                            check.fehler.front()));
                pruefung(lesen(validation_path) == before,
                    "Bestandspruefung veraenderte Validierungsdatei");
                ++count;
            }
            pruefung(count == 11, "Nicht genau elf Modellordner schema-geprueft");
            std::cout << "11 Modelle: Schema, Identitaet und Ressourcen gueltig; "
                "Fingerprints und Import nicht durch diesen Test abgenommen.\n";
            return 0;
        } catch (const std::exception& error) {
            std::cerr << error.what() << '\n';
            return 1;
        }
    }
    pruefung(argc == 2, "Modellstammverzeichnis als Testargument erforderlich");
    const std::filesystem::path source_root(argv[1]);
    const auto temp_root = std::filesystem::temp_directory_path() /
        ("segelphysik-modellformat03-" +
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    const auto model = temp_root / "bleistift";
    std::filesystem::create_directories(temp_root);
    std::filesystem::copy(source_root / "schema_data.json", temp_root / "schema_data.json");
    std::filesystem::copy(source_root / "schema_validation.json",
        temp_root / "schema_validation.json");
    std::filesystem::copy(source_root / "bleistift", model,
        std::filesystem::copy_options::recursive);

    try {
        pruefe_defaults(temp_root);
        Umgebung schema_root("SEGELPHYSIK_TEST_SCHEMA_ROOT");
        schema_root.setzen(temp_root.u8string());
        const auto validation_file = model / "modells_validation.json";
        auto stale_validation = nlohmann::json::parse(lesen(validation_file));
        stale_validation["model"]["fingerprints"]["visualisation"]["hash"] =
            std::string(64, '0');
        schreiben(validation_file, stale_validation.dump());
        const auto before = lesen(validation_file);
        const auto update = segelphysik::aktualisiere_modell_fingerprints(model,
            {segelphysik::ModellFingerprintBereich::visualisation});
        pruefung(update.validierungsdatei_aktualisiert, "Erstes Fingerprint-Update fehlgeschlagen");
        pruefung(update.werte.size() == 1, "Unerwartete Anzahl Fingerprint-Ergebnisse");
        const auto visual_hash = update.werte.front().hash;
        const auto after_visual = lesen(validation_file);
        pruefung(before != after_visual, "Fingerprint-Datei wurde nicht aktualisiert");
        const auto old_fingerprints = nlohmann::json::parse(before)["model"]["fingerprints"];
        const auto new_fingerprints = nlohmann::json::parse(after_visual)["model"]["fingerprints"];
        for (auto item = old_fingerprints.begin(); item != old_fingerprints.end(); ++item)
            if (item.key() != "visualisation")
                pruefung(new_fingerprints.at(item.key()) == item.value(),
                    "Nicht angeforderter Fingerprint wurde geaendert");

        const auto repeat = segelphysik::aktualisiere_modell_fingerprints(model,
            {segelphysik::ModellFingerprintBereich::visualisation});
        pruefung(!repeat.validierungsdatei_aktualisiert, "Idempotentes Update hat geschrieben");
        pruefung(repeat.werte.front().hash == visual_hash, "Idempotenter Hash stimmt nicht");
        pruefung(lesen(validation_file) == after_visual, "Idempotenter Aufruf veraenderte die Datei");

        auto visual_files = model / "modells" / "visualisation";
        std::filesystem::create_directories(visual_files);
        const auto hidden = visual_files / ".fingerprint-test";
        schreiben(hidden, "hidden");
        const auto changed = segelphysik::aktualisiere_modell_fingerprints(model,
            {segelphysik::ModellFingerprintBereich::visualisation});
        pruefung(changed.validierungsdatei_aktualisiert, "Geaenderte Ressource wurde nicht aktualisiert");
        pruefung(changed.werte.front().hash != visual_hash, "Versteckte Datei fehlt im Hash");

        const auto before_duplicate = lesen(validation_file);
        bool duplicate_rejected = false;
        try {
            (void)segelphysik::aktualisiere_modell_fingerprints(model,
                {segelphysik::ModellFingerprintBereich::mass,
                 segelphysik::ModellFingerprintBereich::mass});
        } catch (const std::runtime_error&) {
            duplicate_rejected = true;
        }
        pruefung(duplicate_rejected, "Doppelte Fingerprint-Bereiche wurden akzeptiert");
        pruefung(lesen(validation_file) == before_duplicate,
            "Fehlerhafter Aufruf veraenderte die Validierungsdatei");

        const auto all = segelphysik::aktualisiere_modell_fingerprints(model,
            {segelphysik::ModellFingerprintBereich::visualisation,
             segelphysik::ModellFingerprintBereich::mass,
             segelphysik::ModellFingerprintBereich::hydrostatics,
             segelphysik::ModellFingerprintBereich::aerodynamics,
             segelphysik::ModellFingerprintBereich::modelData});
        pruefung(all.werte.size() == 5, "Nicht alle Bereiche geliefert");
        pruefung(all.werte[3].hash ==
            "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
            "SHA-256 des leeren Bereichs ist falsch");
        const auto valid = segelphysik::detail::pruefe_modellformat03(model, temp_root);
        pruefung(valid.schemas_gueltig && valid.veraltete_bereiche.empty()
            && valid.fehler_bereiche.empty(), "Aktualisierter Bestand ist nicht gueltig");

        const auto model_data =
            nlohmann::json::parse(lesen(model / "modells_data.json"));
        const auto display_file = model / model_data["groups"]["body"]["elements"][0]
            ["display"]["geometry"].get<std::string>();
        const auto display_contents = lesen(display_file);
        schreiben(display_file, "not an OBJ mesh");
        (void)segelphysik::aktualisiere_modell_fingerprints(model,
            {segelphysik::ModellFingerprintBereich::visualisation});
        const auto metadata_catalog = segelphysik::modellkatalog(temp_root);
        const auto* metadata_entry = metadata_catalog.eintrag("bleistift");
        pruefung(metadata_entry && metadata_entry->status == "verwendbar",
            "Katalogpruefung hat Darstellungsgeometrie statt Fingerprints eingelesen");
        schreiben(display_file, display_contents);
        (void)segelphysik::aktualisiere_modell_fingerprints(model,
            {segelphysik::ModellFingerprintBereich::visualisation});

        const auto aero_dir = model / "modells" / "aerodynamics";
        std::filesystem::create_directories(aero_dir);
        schreiben(aero_dir / "deferred.json", "{ not parsed or hashed");
        const auto deferred_aero_catalog = segelphysik::modellkatalog(temp_root);
        const auto* deferred_aero_entry = deferred_aero_catalog.eintrag("bleistift");
        pruefung(deferred_aero_entry &&
            deferred_aero_entry->status == "verwendbar" &&
            deferred_aero_entry->fingerprints_aktuell,
            "Aerodynamikdateien beeinflussen den vorlaeufigen Katalogstatus");
        std::filesystem::remove_all(aero_dir);

        auto catalog = segelphysik::modellkatalog(temp_root);
        const auto* valid_entry = catalog.eintrag("bleistift");
        pruefung(valid_entry && valid_entry->status == "verwendbar",
            "Aktueller 0.3-Katalog ist nicht verwendbar");
        const auto structure =
            segelphysik::modellstruktur(catalog, "bleistift", valid_entry->modell_fassung);
        pruefung(!structure.gruppen.empty() && !structure.gesamtmasse_kg &&
            !structure.huellquader_m &&
            structure.gruppen.front().elemente.front().referenz_visualisierung &&
            structure.gruppen.front().elemente.front().referenz_hydrostatik,
            "0.3-Objektbaum liest Physik oder verliert Referenzkennungen");
        const auto inspection = segelphysik::readModelInspection(
            catalog, "bleistift", valid_entry->modell_fassung);
        if (inspection->info().status != "verwendbar") {
            std::string details;
            for (const auto& diagnostic : inspection->info().diagnostics)
                details += " [" + diagnostic.fehler_id + "] " + diagnostic.text;
            throw std::runtime_error(
                "Gueltiges 0.3-Modell hat keine verwendbare Inspektion:" + details);
        }
        pruefung(!segelphysik::darstellungsgeometrie(*inspection).teile.empty(),
            "0.3-Inspektion hat keine Geometrie");
        const auto before_failure = lesen(validation_file);
        muss_fehlschlagen([&] {
            segelphysik::aktualisiere_modell_fingerprints(model, {});
        });
        std::filesystem::create_directory(model / ".modells_validation.lock");
        muss_fehlschlagen([&] {
            segelphysik::aktualisiere_modell_fingerprints(model,
                {segelphysik::ModellFingerprintBereich::visualisation});
        });
        std::filesystem::remove(model / ".modells_validation.lock");
        schreiben(model / "modells_data.json", lesen(model / "modells_data.json") + "\n");
        const auto stale = segelphysik::detail::pruefe_modellformat03(model, temp_root);
        pruefung(stale.schemas_gueltig && stale.veraltete_bereiche ==
            std::vector<std::string>{"modelData"}, "Falscher Bereich als veraltet gemeldet");
        pruefung(lesen(validation_file) == before_failure, "Pruefung hat Validierung geschrieben");
        catalog = segelphysik::modellkatalog(temp_root);
        const auto* stale_entry = catalog.eintrag("bleistift");
        pruefung(stale_entry && stale_entry->schema_valide &&
            stale_entry->visualisierung_hash_aktuell && !stale_entry->fingerprints_aktuell &&
            stale_entry->status == "fehlerhaft", "Falsche Katalog-Freigaben bei veraltetem modelData");
        const auto view_only = segelphysik::readModelInspection(
            catalog, "bleistift", stale_entry->modell_fassung);
        pruefung(view_only->info().status == "nur_ansicht" &&
            !segelphysik::darstellungsgeometrie(*view_only).teile.empty(),
            "Physik-Hashfehler sperrt faelschlich die Ansicht");
        muss_fehlschlagen([&] { (void)segelphysik::lade_modell(model / "modells_data.json"); });
        const auto schema_path = temp_root / "schema_data.json";
        const auto original_schema = lesen(schema_path);
        auto schema = nlohmann::json::parse(original_schema);
        schema["allOf"].push_back({{"properties", {{"identity", {{"required", {"missing"}}}}}}});
        schreiben(schema_path, schema.dump());
        const auto invalid_catalog = segelphysik::modellkatalog(temp_root);
        const auto* invalid_entry = invalid_catalog.eintrag("bleistift");
        pruefung(invalid_entry && !invalid_entry->schema_valide &&
            !invalid_entry->visualisierung_hash_aktuell &&
            !invalid_entry->fingerprints_aktuell, "Schemafehler gibt Modell frei");
        pruefung(!segelphysik::modelldiagnose(invalid_catalog, "bleistift",
            invalid_entry->modell_fassung).empty(), "Schema-Diagnose nicht abrufbar");
        muss_fehlschlagen([&] {
            segelphysik::aktualisiere_modell_fingerprints(model,
                {segelphysik::ModellFingerprintBereich::visualisation});
        });
        pruefung(lesen(validation_file) == before_failure, "Schemafehler hat Validierung geaendert");
        schreiben(schema_path, original_schema);
        pruefe_ungueltige_modelle(model, temp_root);

        auto data = nlohmann::json::parse(lesen(model / "modells_data.json"));
        auto material_data = data;
        auto& material_mass =
            material_data["groups"]["body"]["elements"][0]["physics"]["mass"];
        material_mass.erase("density_kg_per_m3");
        material_mass["material"] = "wood";
        const auto data_path = model / "modells_data.json";
        const auto original_data = lesen(data_path);
        schreiben(data_path, material_data.dump());
        const auto material_schema =
            segelphysik::detail::pruefe_modellformat03(model, temp_root, false);
        schreiben(data_path, original_data);
        pruefung(material_schema.schemas_gueltig,
            "Schema-valide Materialreferenz wurde vom Schema abgelehnt");
        muss_fehlschlagen([&] {
            (void)segelphysik::detail::normalisiere_modellformat03(material_data, model);
        });

        auto tack_data = data;
        auto& tack_element = tack_data["groups"]["body"]["elements"][0];
        tack_element["placement"] = {
            {"translation_m", {0, 0, 0}},
            {"rotation_xyzw", {0, 0, 0, 1}},
            {"originalTack", "port"}};
        const auto tack_normalized =
            segelphysik::detail::normalisiere_modellformat03(tack_data, model);
        pruefung(tack_normalized["gruppen"]["body"]["elemente"][0]
            ["setting"]["original_buglage"] == "backbord",
            "originalTack wird nicht in die Modell-Buglage uebernommen");

        auto movement_data = data;
        const auto make_element = [](const std::string& id, const nlohmann::json& translation) {
            return nlohmann::json{
                {"id", id},
                {"display", {{"hidden", false}}},
                {"placement", {
                    {"translation_m", translation},
                    {"rotation_xyzw", {0, 0, 0, 1}}}}
            };
        };
        const nlohmann::json mainsail_movement{
            {"type", "rotate"}, {"axisModel", {0, 0, 1}},
            {"pivotPointModelM", {0.85070675, 0, 0.1}}};
        const nlohmann::json jib_movement{
            {"type", "rotate"}, {"axisModel", {-0.2776051021, 0, 0.9606952729}},
            {"pivotPointModelM", {2.00570675, 0, 0.616428}}};
        movement_data["groups"] = {
            {"mainsail", {
                {"movement", mainsail_movement},
                {"elements", {
                    make_element("Mainsail", {0.85, 0, 1.35}),
                    make_element("Boom", {0.02, -0.19, 0})}}}},
            {"jib", {
                {"movement", jib_movement},
                {"elements", {make_element("Jib", {2.0, 0, 0.62})}}}},
            {"keel", {
                {"movement", mainsail_movement},
                {"elements", {
                    make_element("Rigging", {0, 0, 0}),
                    {{"id", "Centerboard"}, {"display", {{"hidden", false}}},
                     {"physics", {{"hydrostatic", {{"geometry", "schwert.obj"}}}}}}}}}}
        };
        const auto movement_normalized =
            segelphysik::detail::normalisiere_modellformat03(movement_data, model);
        const auto& mainsail_elements =
            movement_normalized["gruppen"]["mainsail"]["elemente"];
        pruefung(mainsail_elements[0]["Bewegung"]["art"] == "drehen" &&
            mainsail_elements[0]["Bewegung"]["achse_modell"] == mainsail_movement["axisModel"] &&
            mainsail_elements[0]["Bewegung"]["mitbewegte_elemente"] ==
                nlohmann::json::array({"Boom"}),
            "Hydrostatikfreie Gruppenbewegung wird nicht auf die Gruppe abgebildet");
        pruefung(mainsail_elements[1]["setting"]["translation_m"] ==
            nlohmann::json({0.02, -0.19, 0}),
            "Gruppenbewegung verwirft die individuelle Elementplatzierung");
        const auto& jib_element =
            movement_normalized["gruppen"]["jib"]["elemente"][0];
        pruefung(jib_element["Bewegung"]["drehpunkt_modell_m"] ==
            jib_movement["pivotPointModelM"],
            "Bewegung der einzelnen Segelgruppe wird nicht erhalten");
        const auto& keel_elements =
            movement_normalized["gruppen"]["keel"]["elemente"];
        pruefung(!keel_elements[0].contains("Bewegung") &&
            keel_elements[1]["Bewegung"]["art"] == "drehen",
            "Vorhandener hydrostatischer Bewegungscontroller wurde veraendert");

        const auto metadata_model = temp_root / "tack-metadata";
        std::filesystem::create_directories(metadata_model);
        const auto metadata_file = metadata_model / "modells_information.json";
        nlohmann::json metadata{
            {"schema_version", "0.1-entwurf"}, {"id", "tack-metadata"},
            {"gruppen", nlohmann::json::object()}};
        metadata["gruppen"]["mainsail"]["elemente"] = nlohmann::json::array({
            {{"id", "sail"}, {"setting", {
                {"translation_m", {0, 0, 0}}, {"rotation_xyzw", {0, 0, 0, 1}},
                {"konfiguriert", true}, {"original_buglage", "backbord"}}}}
        });
        schreiben(metadata_file, metadata.dump());
        pruefung(segelphysik::lies_beschreibung(metadata_file).elemente.size() == 1,
            "Original-Buglage ohne aerodynamische Quelle wird abgelehnt");
        for (auto& group : data.at("groups"))
            for (auto& element : group.at("elements"))
                element["display"]["hidden"] = true;
        const auto normalized = segelphysik::detail::normalisiere_modellformat03(data, model);
        for (const auto& group : normalized.at("gruppen"))
            for (const auto& element : group.at("elemente"))
                pruefung(element.at("darstellung_verborgen").get<bool>(),
                    "Ausgeblendetes Element verliert hidden-Kennung");
        schreiben(model / "modells_data.json", data.dump());
        segelphysik::aktualisiere_modell_fingerprints(model,
            {segelphysik::ModellFingerprintBereich::modelData});
        catalog = segelphysik::modellkatalog(temp_root);
        const auto* hidden_entry = catalog.eintrag("bleistift");
        pruefung(hidden_entry && hidden_entry->status == "verwendbar",
            "Ausblenden hat Physikfreigabe entfernt");
        pruefung(segelphysik::darstellungsgeometrie(catalog, "bleistift",
            hidden_entry->modell_fassung).teile.empty(),
            "Ausgeblendetes Element liefert Geometrie oder Volumenersatz");
    } catch (const std::exception& error) {
        std::filesystem::remove_all(temp_root);
        std::cerr << error.what() << '\n';
        return 1;
    }

    std::filesystem::remove_all(temp_root);
    return 0;
}
