#include "segelphysik/inspektion.hpp"
#include "segelphysik/hydrostatik.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <stdexcept>
#include <string>

using namespace segelphysik;
using Json = nlohmann::json;
namespace fs = std::filesystem;

namespace {

void require(bool condition, const std::string& message)
{
    if (!condition) throw std::runtime_error(message);
}

template <class F>
std::string error_from(F&& action)
{
    try { action(); }
    catch (const std::exception& error) { return error.what(); }
    return {};
}

std::string read_file(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

void write_file(const fs::path& path, const std::string& text)
{
    std::ofstream output(path, std::ios::binary);
    output << text;
}

const InspectionProperty* property(const std::vector<InspectionProperty>& values,
                                   const std::string& node, const std::string& id)
{
    const auto found = std::find_if(values.begin(), values.end(), [&](const auto& value) {
        return value.node_id == node && value.id == id;
    });
    return found == values.end() ? nullptr : &*found;
}

void collect_keys(const Json& value, std::set<std::string>& keys)
{
    if (value.is_object()) {
        for (const auto& [key, child] : value.items()) {
            keys.insert(key);
            collect_keys(child, keys);
        }
    } else if (value.is_array()) {
        for (const auto& child : value) collect_keys(child, keys);
    }
}

bool has_diagnostic(const InspectionInfo& info, const std::string& id)
{
    return std::any_of(info.diagnostics.begin(), info.diagnostics.end(),
        [&](const KatalogDiagnose& value) { return value.fehler_id == id; });
}

void verify_snapshot(const fs::path& models, const fs::path& scratch)
{
    fs::create_directories(scratch);
    fs::copy(models / "420", scratch / "420", fs::copy_options::recursive);
    const auto copied_models = scratch;
    auto catalogue = modellkatalog(copied_models);
    const auto* entry = catalogue.eintrag("420");
    require(entry != nullptr, "420 missing from catalogue");
    const auto snapshot = readModelInspection(catalogue, "420", entry->modell_fassung);
    require(snapshot->info().status == "verwendbar", "420 inspection should be usable");

    const auto tree_types = getAvailableObjectTreeTypes(*snapshot);
    std::set<std::string> tree_ids;
    for (const auto& type : tree_types) tree_ids.insert(type.id);
    for (const auto* type : {"modell", "objekte", "darstellung", "hydrostatik",
            "hydrodynamik", "bewegung"})
        require(tree_ids.count(type) != 0, std::string("missing tree type ") + type);

    const auto model_tree = getObjectTree(*snapshot, "modell");
    require(!model_tree.empty() && model_tree.front().id.empty() &&
        model_tree.front().kind == "modell", "model tree root is invalid");
    const auto group = std::find_if(model_tree.begin(), model_tree.end(), [](const auto& node) {
        return node.kind == "gruppe";
    });
    require(group != model_tree.end() && !group->element_ids.empty(), "group node missing element IDs");
    const auto element = std::find_if(model_tree.begin(), model_tree.end(), [](const auto& node) {
        return node.kind == "element";
    });
    require(element != model_tree.end() && element->id.find("/elemente/") != std::string::npos,
        "element node has no stable logical path");

    const auto object_tree = getObjectTree(*snapshot, "objekte");
    const auto object_elements = std::count_if(object_tree.begin(), object_tree.end(),
        [](const InspectionNode& node) { return node.kind == "element"; });
    const auto object_type = std::find_if(tree_types.begin(), tree_types.end(),
        [](const InspectionType& type) { return type.id == "objekte"; });
    require(object_type != tree_types.end() &&
        object_elements == static_cast<std::ptrdiff_t>(object_type->element_ids.size()),
        "canonical object tree duplicated or omitted the model element");
    require(std::all_of(object_tree.begin(), object_tree.end(), [](const InspectionNode& node) {
        return node.kind == "modell" || node.kind == "gruppe" || node.kind == "element";
    }), "canonical object tree contains metadata/domain pseudo-objects");
    const auto canonical_root_properties = getObjectProperties(*snapshot, "objekte", {""}, true);
    require(std::any_of(canonical_root_properties.begin(), canonical_root_properties.end(),
        [](const InspectionProperty& value) {
            return value.object_path.rfind("/entwurf/", 0) == 0;
        }), "model metadata was lost when pseudo-object nodes were removed");
    const auto object_properties = getObjectProperties(*snapshot, "objekte", {element->id}, true);
    std::set<std::string> areas;
    for (const auto& value : object_properties) {
        require(!value.area.empty(), "object property has no explicit area");
        areas.insert(value.area);
    }
    for (const auto& area : areas)
        require(area == "Model" || area == "Geometrie" || area == "Darstellung" ||
            area == "Masse" || area == "Material" || area == "Hydrostatik" ||
            area == "Hydrodynamik" || area == "Bewegung", "unknown property area: " + area);
    require(areas.count("Masse") && areas.count("Darstellung"),
        "canonical object properties omitted mass or display area");

    const auto root_properties = getObjectProperties(*snapshot, "modell", {""}, false);
    const auto* total_mass = property(root_properties, "", "@gesamtmasse_kg");
    const auto* bounds = property(root_properties, "", "@huellquader_m");
    require(total_mass && total_mass->value_type == "real" && total_mass->unit == "kg" &&
        total_mass->status == "vorhanden", "total mass property missing or mistyped");
    require(bounds && bounds->value_type == "object" && bounds->unit == "m",
        "bounds property missing or mistyped");
    const auto source_document = Json::parse(read_file(scratch / "420" / "modell.json"));
    std::set<std::string> source_keys;
    collect_keys(source_document, source_keys);
    std::set<std::string> represented;
    for (const auto& node : model_tree) represented.insert(node.name);
    for (const auto& value : getObjectProperties(*snapshot, "modell", {""}, true))
        represented.insert(value.name);
    for (const auto& key : source_keys)
        if (key != "gruppen" && key != "elemente")
            require(represented.count(key) != 0, "source field omitted from inspection tree: " + key);

    const auto movement_tree = getObjectTree(*snapshot, "bewegung");
    const auto movement = std::find_if(movement_tree.begin(), movement_tree.end(), [](const auto& node) {
        return node.name == "Bewegung";
    });
    require(movement != movement_tree.end(), "movement subtree missing");
    const auto movement_properties = getObjectProperties(*snapshot, "bewegung", {movement->id}, false);
    require(property(movement_properties, movement->id, "@hubweg_m") != nullptr,
        "derived movement lift distance missing");
    const auto structure = modellstruktur(*snapshot);
    const auto moving_element = std::find_if(structure.gruppen.begin(), structure.gruppen.end(),
        [](const StrukturGruppe& value) {
            return std::any_of(value.elemente.begin(), value.elemente.end(),
                [](const StrukturElement& item) { return item.bewegung.has_value(); });
        });
    require(moving_element != structure.gruppen.end(), "movement definition missing from structure");
    const auto moving_definition = std::find_if(moving_element->elemente.begin(),
        moving_element->elemente.end(), [](const StrukturElement& item) { return item.bewegung.has_value(); });
    const auto preview = getMovementPreview(*snapshot, moving_definition->id);
    require(preview && preview->frame == "modell" && preview->poses.size() == 3 &&
        preview->minimum_value == std::optional<double>(0.) &&
        preview->maximum_value == std::optional<double>(100.) &&
        preview->poses[0].label == "normal" && !preview->poses[0].meshes.empty(),
        "typed movement preview lacks normal/minimum/maximum geometry");
    const auto preview_json = Json::parse(movementPreviewJson(*snapshot, moving_definition->id));
    require(preview_json["preview"]["element_id"] == moving_definition->id &&
        preview_json["preview"]["poses"].size() == preview->poses.size(),
        "movement preview JSON differs from typed API");
    const auto& preview_mesh = preview->poses.front().meshes.front();
    const auto source_meshes = getMesh(*snapshot, "darstellung",
        std::vector<std::string>{preview_mesh.element_id});
    const auto source_mesh = std::find_if(source_meshes.begin(), source_meshes.end(),
        [&](const InspectionMesh& mesh) { return mesh.id == preview_mesh.id.substr(
            0, preview_mesh.id.find("/preview/")); });
    require(source_mesh != source_meshes.end(), "movement preview source mesh missing");
    Modell movement_model;
    Element movement_controller;
    movement_controller.id = moving_definition->id;
    movement_controller.bewegung = moving_definition->bewegung;
    movement_model.elemente.push_back(movement_controller);
    const auto transform = bewegungstransformation(movement_model, source_mesh->element_id,
        {{moving_definition->id, preview->reference_value}});
    const auto& source_point = source_mesh->points_m.front();
    const std::array<double, 3> transformed_point{
        transform[0] * source_point[0] + transform[1] * source_point[1] +
            transform[2] * source_point[2] + transform[3],
        transform[4] * source_point[0] + transform[5] * source_point[1] +
            transform[6] * source_point[2] + transform[7],
        transform[8] * source_point[0] + transform[9] * source_point[1] +
            transform[10] * source_point[2] + transform[11]};
    for (std::size_t axis = 0; axis < 3; ++axis)
        require(std::abs(preview_mesh.points_m.front()[axis] - transformed_point[axis]) < 1e-10,
            "movement preview did not use Core movement transform");

    const auto mesh_types = getAvailableMeshTypes(*snapshot);
    std::set<std::string> mesh_ids;
    for (const auto& type : mesh_types) mesh_ids.insert(type.id);
    for (const auto* type : {"darstellung", "hydrostatik_volumen", "hydrodynamik",
            "massen_mesh", "massenpunkte", "flutrand"})
        require(mesh_ids.count(type) != 0, std::string("missing mesh type ") + type);
    const auto display_meshes = getMesh(*snapshot, "darstellung");
    require(!display_meshes.empty(), "display mesh missing");
    const auto points = getMesh(*snapshot, "massenpunkte");
    require(!points.empty() && points.front().topology == "punkte" &&
        points.front().triangles.empty(), "point topology is not preserved");

    const auto overlapping = getObjectProperties(
        *snapshot, "modell", {"", group->id, element->id}, true);
    std::set<std::pair<std::string, std::string>> unique_properties;
    for (const auto& value : overlapping)
        require(unique_properties.emplace(value.node_id, value.id).second,
            "overlapping selection duplicated a property");

    const auto json_info = Json::parse(inspectionInfoJson(*snapshot));
    const auto json_props = Json::parse(inspectionPropertiesJson(*snapshot, "modell", {""}, false));
    const auto json_object_props = Json::parse(
        inspectionPropertiesJson(*snapshot, "objekte", {element->id}, true));
    const auto json_meshes = Json::parse(inspectionMeshJson(*snapshot, "darstellung"));
    require(json_info["inspektionsversion"] == "1" &&
        json_info["info"]["snapshot_id"] == snapshot->info().snapshot_id,
        "info JSON differs from C++ API");
    const auto json_total_mass = std::find_if(json_props["properties"].begin(),
        json_props["properties"].end(), [](const Json& value) {
            return value["id"] == "@gesamtmasse_kg";
        });
    require(json_total_mass != json_props["properties"].end() &&
        (*json_total_mass)["value"] == std::get<double>(total_mass->value),
        "typed C++ and JSON properties differ");
    require(!json_object_props["properties"].empty() &&
        json_object_props["properties"].front().contains("area"),
        "property JSON omitted the additive area field");
    require(json_meshes["frame"] == "modell" && json_meshes["length_unit"] == "m" &&
        json_meshes["meshes"].size() == display_meshes.size(),
        "mesh JSON differs from C++ result");
    const auto json_text = inspectionInfoJson(*snapshot) + inspectionTreeJson(*snapshot, "modell") +
        inspectionPropertiesJson(*snapshot, "modell", {""}) + inspectionMeshJson(*snapshot, "darstellung");
    require(json_text.find(scratch.u8string()) == std::string::npos &&
        json_text.find("\\\\") == std::string::npos, "filesystem path leaked into inspection JSON");

    const auto old_structure = modellstruktur(*snapshot);
    const auto old_display = darstellungsgeometrie(*snapshot);
    require(old_structure.gesamtmasse_kg == std::optional<double>(std::get<double>(total_mass->value)),
        "compatible structure projection disagrees on total mass");
    require(old_display.teile.size() == display_meshes.size(),
        "compatible display projection disagrees with snapshot mesh");
    require(modellstruktur(catalogue, "420", entry->modell_fassung).gesamtmasse_kg ==
        old_structure.gesamtmasse_kg, "legacy structure API changed");
    const auto legacy_display = darstellungsgeometrie(catalogue, "420", entry->modell_fassung);
    require(legacy_display.teile.size() == old_display.teile.size(),
        "legacy display API changed");

    const auto model_json = scratch / "420" / "modell.json";
    write_file(model_json, read_file(model_json) + "\n");
    for (const auto& path : {scratch / "420" / "meshes" / "gui" / "rumpf.obj",
             scratch / "420" / "meshes" / "masse" / "rumpf.obj"}) {
        std::error_code ignored;
        fs::remove(path, ignored);
    }
    const auto tree_after_change = getObjectTree(*snapshot, "modell");
    const auto object_tree_after_change = getObjectTree(*snapshot, "objekte");
    require(tree_after_change.size() == model_tree.size() &&
        std::equal(tree_after_change.begin(), tree_after_change.end(), model_tree.begin(),
            [](const InspectionNode& a, const InspectionNode& b) {
                return a.id == b.id && a.parent_id == b.parent_id && a.name == b.name;
            }), "snapshot tree changed after source files changed");
    require(object_tree_after_change.size() == object_tree.size(),
        "canonical object tree changed after source files changed");
    require(getObjectProperties(*snapshot, "modell", {""}, false).size() == root_properties.size(),
        "snapshot properties changed after source files changed");
    require(getMesh(*snapshot, "darstellung").size() == display_meshes.size(),
        "snapshot display mesh changed after source files changed");
    require(darstellungsgeometrie(*snapshot).teile.size() == old_display.teile.size(),
        "compatible display projection reread files");
    require(error_from([&] { (void)darstellungsgeometrie(catalogue, "420", entry->modell_fassung); })
        .rfind("CORE-KAT-0004", 0) == 0,
        "legacy display API no longer enforces catalogue version lifecycle");
    require(error_from([&] { (void)getObjectTree(*snapshot, "missing"); })
        .rfind("CORE-INSP-0001", 0) == 0, "unknown tree type returned success");
    require(error_from([&] { (void)getObjectProperties(*snapshot, "modell", {"/missing"}); })
        .rfind("CORE-INSP-0002", 0) == 0, "unknown node returned success");
    require(error_from([&] { (void)getMesh(*snapshot, "darstellung", std::vector<std::string>{"missing"}); })
        .rfind("CORE-INSP-0002", 0) == 0, "unknown element returned success");
}

void verify_partial_and_unreadable(const fs::path& models, const fs::path& scratch)
{
    const auto partial_folder = scratch / "partial";
    fs::create_directories(partial_folder);
    fs::copy(models / "quader", partial_folder / "quader", fs::copy_options::recursive);
    const auto partial_file = partial_folder / "quader" / "modell.json";
    auto document = Json::parse(read_file(partial_file));
    auto& mass = document["gruppen"]["rumpf"]["elemente"][0]["Hydrostatik"]["masse"];
    mass["dichte_kg_pro_m3"] = -1;
    write_file(partial_file, document.dump(2));
    const auto partial_catalogue = modellkatalog(partial_folder);
    const auto* partial_entry = partial_catalogue.eintrag("quader");
    require(partial_entry != nullptr, "invalid model missing from catalogue");
    const auto partial = readModelInspection(partial_catalogue, "quader", partial_entry->modell_fassung);
    require(partial->info().status == "nur_ansicht" &&
        has_diagnostic(partial->info(), "CORE-INSP-0004"),
        "invalid physics did not produce a partial view diagnostic");
    const auto partial_info = inspectionInfoJson(*partial);
    require(partial_info.find("modell.json") == std::string::npos &&
        partial_info.find(scratch.u8string()) == std::string::npos &&
        partial_info.find("\\\\") == std::string::npos,
        "partial-view diagnostics leaked a source path");
    const auto invalid_properties = getObjectProperties(*partial, "hydrostatik", {""}, true);
    const auto invalid_density = std::find_if(invalid_properties.begin(), invalid_properties.end(),
        [](const InspectionProperty& value) { return value.id == "dichte_kg_pro_m3"; });
    require(invalid_density != invalid_properties.end() &&
        invalid_density->status == "fehlerhaft" &&
        std::holds_alternative<std::monostate>(invalid_density->value),
        "invalid source physics was exposed as a valid property");
    require(!getMesh(*partial, "darstellung").empty(),
        "readable display geometry was lost for an invalid physical model");

    const auto unreadable_folder = scratch / "unreadable";
    fs::create_directories(unreadable_folder / "broken");
    write_file(unreadable_folder / "broken" / "modell.json", "{");
    const auto unreadable_catalogue = modellkatalog(unreadable_folder);
    const auto* unreadable_entry = unreadable_catalogue.eintrag("broken");
    require(unreadable_entry != nullptr, "unreadable model missing from catalogue");
    require(error_from([&] {
        (void)readModelInspection(unreadable_catalogue, "broken", unreadable_entry->modell_fassung);
    }).rfind("CORE-INSP-0003", 0) == 0,
        "unreadable model did not fail snapshot capture");
}

}

int main(int argc, char** argv)
{
    try {
        if (argc != 3) throw std::runtime_error("expected model and scratch directories");
        const auto scratch = fs::u8path(argv[2]) / "inspection-test";
        std::error_code ignored;
        fs::remove_all(scratch, ignored);
        verify_snapshot(fs::u8path(argv[1]), scratch);
        verify_partial_and_unreadable(fs::u8path(argv[1]), scratch);
        fs::remove_all(scratch, ignored);
        std::cout << "Core inspection contracts passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
