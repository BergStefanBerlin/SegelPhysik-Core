#include "segelphysik/inspektion.hpp"
#include "segelphysik/simulation.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <stdexcept>

namespace fs = std::filesystem;
using namespace segelphysik;
using Json = nlohmann::json;

namespace {
void require(bool ok, const std::string& message)
{
    if (!ok) throw std::runtime_error(message);
}
std::string read(const fs::path& path)
{
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
void write(const fs::path& path, const std::string& text)
{
    std::ofstream out(path, std::ios::binary);
    out << text;
}
template<class F> void rejects(F action, const std::string& code)
{
    try { action(); }
    catch (const std::exception& e) {
        require(std::string(e.what()).find(code) != std::string::npos, e.what());
        return;
    }
    throw std::runtime_error("Expected rejection: " + code);
}
void same_mesh(const Mesh& a, const Mesh& b)
{
    require(a.punkte == b.punkte && a.dreiecke == b.dreiecke, "Mesh changed");
}
void same_optional_mesh(const std::optional<Mesh>& a, const std::optional<Mesh>& b)
{
    require(a.has_value() == b.has_value(), "Mesh presence changed");
    if (a) same_mesh(*a, *b);
}
void same_model(const Modell& a, const Modell& b)
{
    require(a.id == b.id && a.elemente.size() == b.elemente.size(), "Structure changed");
    require(a.masse_kg == b.masse_kg && a.schwerpunkt_m == b.schwerpunkt_m &&
        a.traegheit_kg_m2 == b.traegheit_kg_m2, "Total physical properties changed");
    for (std::size_t i = 0; i < a.elemente.size(); ++i) {
        const auto& x = a.elemente[i];
        const auto& y = b.elemente[i];
        require(x.id == y.id && x.masse_kg == y.masse_kg && x.schwerpunkt_m == y.schwerpunkt_m &&
            x.traegheit_kg_m2 == y.traegheit_kg_m2 && x.massenart == y.massenart &&
            x.dichte_kg_pro_m3 == y.dichte_kg_pro_m3 && x.wanddicke_m == y.wanddicke_m &&
            x.hydrodynamik_rolle == y.hydrodynamik_rolle && x.flutrand_m == y.flutrand_m &&
            x.massenpunkte_m == y.massenpunkte_m, "Element physics changed: " + x.id);
        same_optional_mesh(x.volumen, y.volumen);
        same_optional_mesh(x.massen_mesh, y.massen_mesh);
        require(x.bewegung.has_value() == y.bewegung.has_value(), "Movement missing");
        if (x.bewegung) {
            const auto& p = *x.bewegung;
            const auto& q = *y.bewegung;
            require(p.art == q.art && p.ruderwinkel == q.ruderwinkel &&
                p.achse_modell == q.achse_modell && p.drehpunkt_modell_m == q.drehpunkt_modell_m &&
                p.richtung_modell == q.richtung_modell && p.hubweg_m == q.hubweg_m &&
                p.mitbewegte_elemente == q.mitbewegte_elemente, "Movement changed");
        }
    }
}
void compare_display(const Darstellungsgeometrie& a, const Darstellungsgeometrie& b)
{
    require(a.teile.size() == b.teile.size(), "Display count changed");
    for (std::size_t i = 0; i < a.teile.size(); ++i) {
        const auto& x = a.teile[i]; const auto& y = b.teile[i];
        require(x.element_id == y.element_id && x.gruppe == y.gruppe && x.herkunft == y.herkunft &&
            x.punkte_m == y.punkte_m && x.dreiecke == y.dreiecke &&
            x.farbe_rgb == y.farbe_rgb && x.oberflaeche == y.oberflaeche, "Display changed");
    }
}
void compare_simulation(const fs::path& old_model, const fs::path& new_model, const fs::path& output)
{
    Laufoptionen options;
    options.dynamisch = true;
    options.dauer_s = .04;
    options.dt_s = .01;
    options.start.geschwindigkeit_welt_m_s = {0.4, 0.1, 0.};
    const auto old_out = output / "alt.jsonl";
    const auto new_out = output / "neu.jsonl";
    const auto a = simulieren(old_model, old_out, options);
    const auto b = simulieren(new_model, new_out, options);
    require(a.status == "abgeschlossen" && b.status == a.status && a.zustaende == b.zustaende,
        "Simulation failed: " + a.diagnose + "; " + b.diagnose);
    std::ifstream old_stream(old_out), new_stream(new_out);
    std::string old_line, new_line;
    while (std::getline(old_stream, old_line)) {
        require(static_cast<bool>(std::getline(new_stream, new_line)), "Simulation states missing");
        auto x = Json::parse(old_line), y = Json::parse(new_line);
        auto remove_version = [](Json& state) {
            state.erase("modell_fassung");
            if (state.contains("hydrodynamik")) state["hydrodynamik"].erase("modell_fassung");
            if (state.contains("testantrieb")) state["testantrieb"].erase("modell_fassung");
        };
        remove_version(x); remove_version(y);
        require(x == y, "Simulation output changed");
    }
    require(!std::getline(new_stream, new_line), "Extra simulation states");
}
void reference_models(const fs::path& source, const fs::path& output)
{
    const auto old_catalogue = modellkatalog(source);
    for (const auto* name : {"optimist", "420", "folkeboot", "bleistift", "kegel", "kugel",
        "platte", "pyramide", "quader", "rolle", "unregelmaessig"}) {
        const auto old_file = source / name / "bootsmodell.json";
        const auto target = output / name;
        const auto original = read(old_file);
        konvertiere_modell_v2(old_file, target);
        require(read(old_file) == original, "Original JSON changed");
        const auto new_file = target / "bootsmodell.json";
        same_model(lade_modell(old_file), lade_modell(new_file));
        require(modell_fassung(new_file) == lade_modell(new_file).fassung, "Fingerprint inconsistent");
        const auto catalogue = modellkatalog(output);
        const auto* old_entry = old_catalogue.eintrag(name);
        const auto* entry = catalogue.eintrag(name);
        require(old_entry && entry && old_entry->status == entry->status, "Catalogue status changed");
        const auto old_snapshot = readModelInspection(old_catalogue, name, old_entry->modell_fassung);
        const auto snapshot = readModelInspection(catalogue, name, entry->modell_fassung);
        require(old_snapshot->info().status == snapshot->info().status &&
            old_snapshot->info().diagnostics.size() == snapshot->info().diagnostics.size(),
            "Inspection diagnostics changed");
        compare_display(darstellungsgeometrie(*old_snapshot), darstellungsgeometrie(*snapshot));
        const auto old_tree = modellstruktur(*old_snapshot);
        const auto new_tree = modellstruktur(*snapshot);
        require(old_tree.gruppen.size() == new_tree.gruppen.size(), "Groups changed");
        for (std::size_t i = 0; i < old_tree.gruppen.size(); ++i) {
            const auto& x = old_tree.gruppen[i]; const auto& y = new_tree.gruppen[i];
            require(x.id == y.id && x.name == y.name && x.elemente.size() == y.elemente.size(),
                "Group structure changed");
            for (std::size_t j = 0; j < x.elemente.size(); ++j)
                require(x.elemente[j].id == y.elemente[j].id &&
                    x.elemente[j].darstellung == y.elemente[j].darstellung &&
                    x.elemente[j].hydrostatik_volumen == y.elemente[j].hydrostatik_volumen,
                    "Element structure changed");
        }
        const auto old_document = Json::parse(original);
        const auto new_document = Json::parse(read(new_file));
        require(new_document.at("schema_version") == "2", "Schema-2 version missing");
        for (const auto* key : {"name", "status", "entwurf", "Hydrodynamische_Traegheit"})
            if (old_document.contains(key))
                require(old_document[key] == new_document[key], "Original metadata changed");
        for (const auto& [group_name, group] : old_document["gruppen"].items())
            for (std::size_t i = 0; i < group["elemente"].size(); ++i) {
                const auto& x = group["elemente"][i];
                const auto& y = new_document["gruppen"][group_name]["elemente"][i];
                for (const auto* role : {"volumen", "masse"})
                    if (y.contains("Hydrostatik") && y["Hydrostatik"].contains(role))
                        require(!y["Hydrostatik"][role].contains("ableitung"),
                            "Schema-3 derivation leaked into schema 2");
                if (y.contains("Darstellung"))
                    require(!y["Darstellung"].contains("ableitung"),
                        "Display derivation leaked into schema 2");
                for (const auto* key : {"Hydrodynamik", "Bewegung"})
                    if (x.contains(key)) require(x[key] == y[key], "Physical domain data changed");
            }
        for (const auto& d : old_snapshot->info().diagnostics)
            require(std::any_of(snapshot->info().diagnostics.begin(), snapshot->info().diagnostics.end(),
                [&](const KatalogDiagnose& x) { return d.fehler_id == x.fehler_id; }),
                "Diagnostic ID changed");
        const auto sources = getMesh(*snapshot, "quelle");
        require(!sources.empty(), "Source mesh registry missing");
        std::set<std::string> ids;
        for (const auto& mesh : sources) {
            require(ids.insert(mesh.source_id).second && mesh.provenance == "quelle" &&
                mesh.origin == "OBJ.o:" + mesh.source_id, "Invalid source provenance");
            for (const auto& id : mesh.element_ids) {
                const auto filtered = getMesh(*snapshot, "quelle", std::vector<std::string>{id});
                require(std::any_of(filtered.begin(), filtered.end(), [&](const InspectionMesh& m) {
                    return m.id == mesh.id;
                }), "Shared source filter missing");
            }
        }
        for (const auto& type : getAvailableMeshTypes(*snapshot))
            for (const auto& mesh : getMesh(*snapshot, type.id)) {
                require(!mesh.derivation.empty(), "Missing provenance description");
                if (!mesh.source_id.empty()) require(ids.count(mesh.source_id) != 0, "Dangling source ID");
                if (type.id == "flutrand") require(mesh.provenance == "abgeleitet", "Rim provenance wrong");
                if (type.id != "quelle") {
                    const auto old_meshes = getMesh(*old_snapshot, type.id);
                    const auto old = std::find_if(old_meshes.begin(), old_meshes.end(),
                        [&](const InspectionMesh& x) { return x.element_id == mesh.element_id; });
                    require(old != old_meshes.end() && old->points_m == mesh.points_m &&
                        old->triangles == mesh.triangles && old->topology == mesh.topology,
                        "Inspection geometry changed");
                }
            }
        for (const auto& node : getObjectTree(*snapshot, "modell"))
            if (node.kind == "element")
                require(!node.mesh_ids.empty(), "Element missing mesh links");
        const auto before = inspectionMeshJson(*snapshot, "quelle");
        const auto obj = read(target / "boot.obj");
        write(target / "boot.obj", obj + "\n# changed\n");
        require(inspectionMeshJson(*snapshot, "quelle") == before, "Snapshot not immutable");
        rejects([&] { readModelInspection(catalogue, name, entry->modell_fassung); }, "CORE-KAT-0004");
        write(target / "boot.obj", obj);
        rejects([&] { konvertiere_modell_v2(old_file, target); }, "CORE-MOD-0005");
        if (std::string(name) == "quader") compare_simulation(old_file, new_file, target);
        std::cout << name << ": exact physics/display/movement equivalence\n";
    }
}
void schema_v3_migration(const fs::path& output)
{
    const std::set<std::string> conflicted{"420", "folkeboot", "optimist"};
    const auto v3_output = output / "schema-v3-modelle";
    fs::create_directory(v3_output);
    for (const auto* name : {"bleistift", "kegel", "kugel", "platte", "pyramide",
        "quader", "rolle", "unregelmaessig", "420", "folkeboot", "optimist"}) {
        const auto old_file = output / name / "bootsmodell.json";
        const auto target = v3_output / name;
        const auto original = read(old_file);
        if (conflicted.count(name)) {
            rejects([&] { konvertiere_modell_v3(old_file, target); }, "CORE-MOD-0009");
            require(!fs::exists(target), "Conflicted migration created target directory");
            require(read(old_file) == original, "Conflicted migration changed source");
            continue;
        }

        konvertiere_modell_v3(old_file, target);
        const auto new_file = target / "bootsmodell.json";
        require(read(old_file) == original, "Schema-3 migration changed source");
        same_model(lade_modell(old_file), lade_modell(new_file));
        const auto document = Json::parse(read(new_file));
        require(document["schema_version"] == "3", "Schema-3 version missing");
        require(document["geometrie"]["datei"] == "boot.obj" &&
            document["geometrie"]["laengeneinheit"] == "m", "Schema-3 shared OBJ missing");
        std::set<std::string> mesh_ids;
        for (const auto& [group_name, group] : document["gruppen"].items()) {
            for (const auto& element : group["elemente"]) {
                std::vector<std::string> references;
                if (element.contains("Darstellung"))
                    references.push_back(element["Darstellung"]["mesh"].get<std::string>());
                if (element.contains("Hydrostatik")) {
                    const auto& hydro = element["Hydrostatik"];
                    if (hydro.contains("volumen"))
                        references.push_back(hydro["volumen"]["mesh"].get<std::string>());
                    if (hydro.contains("masse") &&
                        hydro["masse"].value("art", "") != "punktmasse")
                        references.push_back(hydro["masse"]["mesh"].get<std::string>());
                }
                if (references.empty()) continue;
                require(std::all_of(references.begin(), references.end(),
                    [&](const std::string& id) { return id == references.front(); }),
                    "Element references more than one schema-3 mesh: " +
                    element["id"].get<std::string>());
                require(mesh_ids.insert(references.front()).second,
                    "Schema-3 mesh shared between geometric elements");
            }
        }
    }

    const auto v2_file = output / "quader" / "bootsmodell.json";
    const auto v2_target = v3_output / "quader-from-v2";
    konvertiere_modell_v3(v2_file, v2_target);
    same_model(lade_modell(v2_file), lade_modell(v2_target / "bootsmodell.json"));
}

void derived_migration(const fs::path& output)
{
    const auto converted = output / "schema-v3-derived";
    fs::create_directory(converted);
    auto near = [](double a, double b, double tolerance, const std::string& field) {
        require(std::isfinite(a) && std::isfinite(b) && std::abs(a-b) <= tolerance,
            field + ": " + std::to_string(a) + " vs " + std::to_string(b));
    };
    for (const auto* name : {"420", "folkeboot", "optimist"}) {
        const auto old_file = output / name / "bootsmodell.json";
        const auto target = converted / name;
        const auto original = read(old_file);
        konvertiere_modell_v3(old_file, target, true);
        const auto new_file = target / "bootsmodell.json";
        const auto before = lade_modell(old_file), after = lade_modell(new_file);
        require(read(old_file) == original, "Derived migration changed original");
        near(before.masse_kg, after.masse_kg, 1e-9*before.masse_kg, "Total mass");
        double maximum_volume_delta = 0.;
        for (std::size_t i = 0; i < before.elemente.size(); ++i) {
            const auto& a = before.elemente[i]; const auto& b = after.elemente.at(i);
            require(a.id == b.id && a.hydrodynamik_rolle == b.hydrodynamik_rolle,
                "Derived migration changed identity/role");
            near(a.masse_kg, b.masse_kg, 1e-9*std::max(1., a.masse_kg), a.id + " mass");
            for (std::size_t j = 0; j < 3; ++j)
                near(a.schwerpunkt_m[j], b.schwerpunkt_m[j], 1e-8, a.id + " mass centre");
            for (std::size_t j = 0; j < 9; ++j)
                near(a.traegheit_kg_m2[j], b.traegheit_kg_m2[j],
                    1e-8*std::max(1., std::abs(a.traegheit_kg_m2[j])), a.id + " inertia");
            auto rim_a = a.flutrand_m, rim_b = b.flutrand_m;
            std::sort(rim_a.begin(), rim_a.end()); std::sort(rim_b.begin(), rim_b.end());
            require(rim_a == rim_b, a.id + " flooding rim changed");
            auto supports_a = a.massenpunkte_m, supports_b = b.massenpunkte_m;
            std::sort(supports_a.begin(), supports_a.end()); std::sort(supports_b.begin(), supports_b.end());
            require(supports_a == supports_b, a.id + " mass support coordinates changed");
            require(a.volumen.has_value() == b.volumen.has_value(), "Volume missing");
            if (a.volumen) for (const auto& pose : {
                    Pose({0,0,-.8}, {0,0,0,1}), Pose({0,0,0}, {0,0,0,1}),
                    Pose({0,0,.2}, {0,0,0,1}), Pose({0,0,-.2}, {.08,.04,0,1})}) {
                const auto x = wasseranteil(*a.volumen, pose);
                const auto y = wasseranteil(*b.volumen, pose);
                maximum_volume_delta = std::max(maximum_volume_delta, std::abs(x.volumen_m3-y.volumen_m3));
                near(x.volumen_m3, y.volumen_m3, 1e-8 + 1e-6*x.volumen_m3, a.id + " wet volume");
                require(x.schwerpunkt_m.has_value() == y.schwerpunkt_m.has_value(),
                    a.id + " wet centre availability");
                if (x.schwerpunkt_m) for (std::size_t j = 0; j < 3; ++j)
                    near((*x.schwerpunkt_m)[j], (*y.schwerpunkt_m)[j], 1e-6, a.id + " wet centre");
            }
        }
        for (const auto& pose : {Pose({0,0,0}, {0,0,0,1}), Pose({0,0,-.2}, {.08,.04,0,1})}) {
            require(flutung(before, pose, {}) == flutung(after, pose, {}), "Flooding behaviour changed");
            const auto x = hydrostatik(before, pose), y = hydrostatik(after, pose);
            for (std::size_t j = 0; j < 3; ++j) {
                near(x.kraft_welt_n[j], y.kraft_welt_n[j], .001, "Hydrostatic force");
                near(x.moment_modell_nm[j], y.moment_modell_nm[j], .001, "Hydrostatic moment");
            }
        }
        Laufoptionen options;
        options.dynamisch = true; options.dauer_s = .2; options.dt_s = .02;
        options.start.geschwindigkeit_welt_m_s = {.4,.1,0};
        const auto old_output = target / "alt.jsonl", new_output = target / "neu.jsonl";
        const auto x = simulieren(old_file, old_output, options);
        const auto y = simulieren(new_file, new_output, options);
        require(x.status == "abgeschlossen" && y.status == x.status && x.zustaende == y.zustaende,
            "Derived model simulation failed: " + x.diagnose + "; " + y.diagnose);
        std::ifstream old_stream(old_output), new_stream(new_output);
        std::string old_line, new_line;
        double maximum_state_delta = 0.;
        while (std::getline(old_stream, old_line)) {
            require(static_cast<bool>(std::getline(new_stream, new_line)), "Missing simulation state");
            const auto a = Json::parse(old_line), b = Json::parse(new_line);
            for (const auto* field : {"position_m", "orientierung_xyzw", "geschwindigkeit_welt_m_s", "omega_modell_rad_s"}) {
                require(a.at(field).size() == b.at(field).size(), "State shape changed");
                for (std::size_t j = 0; j < a.at(field).size(); ++j) {
                    const double p = a.at(field).at(j), q = b.at(field).at(j);
                    maximum_state_delta = std::max(maximum_state_delta, std::abs(p-q));
                    near(p,q,1e-4,std::string("Simulation ")+field);
                }
            }
        }
        require(!std::getline(new_stream,new_line), "Extra simulation state");
        const auto catalog = modellkatalog(converted);
        const auto* entry = catalog.eintrag(name);
        require(entry && entry->status == "verwendbar", "Derived model catalog failed");
        const auto snapshot = readModelInspection(catalog, name, entry->modell_fassung);
        std::set<std::string> owners;
        for (const auto& mesh : getMesh(*snapshot,"quelle")) {
            require(mesh.element_ids.size() == 1 && owners.insert(mesh.element_ids.front()).second,
                "Derived model has competing sources");
        }
        bool has_derived_volume = false;
        for (const auto& mesh : getMesh(*snapshot,"hydrostatik_volumen"))
            if (mesh.provenance == "abgeleitet") {
                require(!mesh.source_id.empty() && mesh.derivation.find("z_schnitt") != std::string::npos,
                    "Derived volume provenance missing");
                has_derived_volume = true;
            }
        require(has_derived_volume, "Derived volume not exposed");
        std::cout << name << ": derived source regression; maximum wet volume delta="
            << maximum_volume_delta << " m3, maximum state component delta=" << maximum_state_delta << '\n';
    }

    const auto negative = converted / "invalid";
    fs::create_directory(negative);
    auto document = Json::parse(read(output / "schema-v3-modelle" / "quader" / "bootsmodell.json"));
    write(negative / "boot.obj", read(output / "schema-v3-modelle" / "quader" / "boot.obj"));
    auto& volume = document["gruppen"].begin().value()["elemente"][0]["Hydrostatik"]["volumen"];
    for (const auto& definition : {Json{{"art","unknown"},{"z_m",0.}},
            Json{{"art","z_schnitt"},{"z_m",1e6}},
            Json{{"art","z_schnitt"},{"z_m","invalid"}},
            Json{{"art","z_schnitt"},{"z_m",0.},{"steigung_x","invalid"}},
            Json{{"art","ohne_deckflaeche"},{"z_m",0.}}}) {
        volume["ableitung"] = definition;
        write(negative / "bootsmodell.json", document.dump());
        rejects([&] { lade_modell(negative / "bootsmodell.json"); }, "CORE-MOD-0009");
    }
    volume.erase("ableitung");
    auto& shell = document["gruppen"].begin().value()["elemente"][0]["Hydrostatik"]["masse"];
    shell["art"] = "schale"; shell["wanddicke_m"] = .01;
    shell["ableitung"] = {{"art","ohne_deckflaeche"},{"z_m",1e6}};
    write(negative / "bootsmodell.json", document.dump());
    rejects([&] { lade_modell(negative / "bootsmodell.json"); }, "CORE-MOD-0009");
    rejects([&] { konvertiere_modell_v3(negative / "bootsmodell.json", converted / "must-not-exist", true); },
        "CORE-MOD-0009");
    require(!fs::exists(converted / "must-not-exist"), "Invalid derivation created a target");
}
void validation(const fs::path& output)
{
    const auto target = output / "validation";
    fs::create_directory(target);
    const auto template_file = output / "quader" / "bootsmodell.json";
    auto root = Json::parse(read(template_file));
    const auto geometry = read(output / "quader" / "boot.obj");
    const auto file = target / "bootsmodell.json";
    write(target / "boot.obj", geometry);
    auto test = [&](const Json& document, const std::string& code) {
        write(file, document.dump());
        rejects([&] { lade_modell(file); }, code);
    };
    auto& element = root["gruppen"].begin().value()["elemente"][0];
    auto bad = root;
    bad["geometrie"]["laengeneinheit"] = "cm";
    test(bad, "CORE-MOD-0002");
    const auto original_element = element;
    element["Darstellung"]["mesh"] = "missing";
    test(root, "CORE-MOD-0006");
    element = original_element;
    element["Hydrodynamik"].erase("rolle");
    test(root, "CORE-MOD-0003");
    element = original_element;
    element["Hydrodynamik"]["rolle"] = "rudder";
    test(root, "CORE-MOD-0003");
    element = original_element;
    element["Hydrostatik"]["volumen"]["datei"] = "other.obj";
    test(root, "CORE-MOD-0002");
    element = original_element;
    element["Hydrostatik"]["masse"].erase("mesh");
    test(root, "CORE-MOD-0004");
    element = original_element;
    element["Hydrostatik"]["volumen"]["mesh"] = 17;
    test(root, "CORE-MOD-0002");
    element = original_element;
    const auto mesh_id = element["Hydrostatik"]["volumen"]["mesh"].get<std::string>();
    auto repeated = root;
    repeated["gruppen"].begin().value()["elemente"].push_back(original_element);
    test(repeated, "CORE-MOD-0003");
    write(file, root.dump());
    auto repeated_role = read(file);
    const auto role_at = repeated_role.find("\"rolle\":\"rumpf\"");
    require(role_at != std::string::npos, "Role fixture absent");
    repeated_role.replace(role_at, std::string("\"rolle\":\"rumpf\"").size(),
        "\"rolle\":\"rumpf\",\"rolle\":\"kiel\"");
    write(file, repeated_role);
    rejects([&] { lade_modell(file); }, "CORE-MOD-0001");
    root["id"] = "unrelated-model-name";
    element["id"] = "arbitrary-stable-id";
    write(file, root.dump());
    require(lade_modell(file).elemente.front().hydrodynamik_rolle == HydrodynamikRolle::rumpf,
        "Role inferred from name");
    element.erase("Hydrodynamik");
    root["id"] = "quader";
    write(file, root.dump());
    require(!lade_modell(file).elemente.front().hydrodynamik_rolle, "Implicit role in v2");
    element = original_element;
    write(file, root.dump());
    write(target / "boot.obj", geometry + "\no " + mesh_id + "\n");
    rejects([&] { lade_modell(file); }, "CORE-MOD-0006");
    write(target / "boot.obj", "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n");
    rejects([&] { lade_modell(file); }, "CORE-MOD-0006");
    // Shared global vertices, negative indices and independent o selection.
    write(target / "boot.obj",
        "v 0 0 0\nv 1 0 0\nv 0 1 0\no alpha\nf -3 -2 -1\n"
        "o beta\nv 0 0 1\nf 1 3 4\no unbenutzt\nf 1 2 4\n");
    root["id"] = "object-selection-fixture";
    root["gruppen"] = {{"test", {{"elemente", Json::array({
        {{"id", "first"}, {"Darstellung", {{"mesh", "alpha"}}},
            {"Hydrostatik", {{"masse", {{"art", "punktmasse"}, {"masse_kg", 1.},
                {"position_m", {0., 0., 0.}}}}}}},
        {{"id", "second"}, {"Darstellung", {{"mesh", "alpha"}}}},
        {{"id", "third"}, {"Darstellung", {{"mesh", "beta"}}}}
    })}}}};
    write(file, root.dump());
    require(lade_modell(file).masse_kg == 1., "Point mass changed");
    const auto desc = lies_beschreibung(file);
    require(desc.quellmeshes.size() == 3, "Source objects not selected");
    require(desc.quellmeshes[0].mesh.punkte == std::vector<Vektor3>{{0,0,0},{1,0,0},{0,1,0}} &&
        desc.quellmeshes[1].mesh.punkte == std::vector<Vektor3>{{0,0,0},{0,1,0},{0,0,1}},
        "Global/negative OBJ indices changed");
    const auto catalogue = modellkatalog(output);
    const auto* entry = catalogue.eintrag(root["id"].get<std::string>());
    require(entry != nullptr, "Custom model missing");
    const auto snapshot = readModelInspection(catalogue, entry->modell_id, entry->modell_fassung);
    const auto meshes = getMesh(*snapshot, "quelle");
    require(meshes.front().element_ids == std::vector<std::string>{"first", "second"},
        "Shared source relationships missing");
    require(meshes.size() == 3 && meshes.back().element_ids.empty(), "Unused source omitted");
    write(target / "boot.obj",
        "o alpha\nv 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\nf 1 2 3 4\n");
    rejects([&] { lade_modell(file); }, "CORE-MOD-0006");
    const auto invalid_catalogue = modellkatalog(output);
    const auto* invalid_entry = invalid_catalogue.eintrag(root["id"].get<std::string>());
    const auto invalid_snapshot = readModelInspection(invalid_catalogue,
        invalid_entry->modell_id, invalid_entry->modell_fassung);
    require(invalid_snapshot->info().status == "nur_ansicht" &&
        darstellungsgeometrie(*invalid_snapshot).teile.empty(), "Invalid v2 polygon auto-repaired");
    require(getMesh(*snapshot, "quelle").size() == 3, "Old source snapshot changed after invalid file");
    fs::remove_all(target);
}
}

int main(int argc, char* argv[])
{
    if (argc != 3) return 2;
    const fs::path output = fs::path(argv[2]) / "v2-modelle-test";
    try {
        if (fs::exists(output)) fs::remove_all(output);
        fs::create_directory(output);
        reference_models(argv[1], output);
        schema_v3_migration(output);
        derived_migration(output);
        validation(output);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
