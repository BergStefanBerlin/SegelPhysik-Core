#include "segelphysik/inspektion.hpp"
#include "segelphysik/hydrostatik.hpp"
#include "segelphysik/segelkonfigurator.hpp"
#include "katalog_intern.hpp"
#include "modell_intern.hpp"
#include "modellformat_03_intern.hpp"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

namespace segelphysik {
namespace {

using Json = nlohmann::json;

[[noreturn]] void inspection_error(const char* id, const std::string& text)
{
    throw std::runtime_error(std::string(id) + ": " + text);
}

std::string pointer_escape(const std::string& text)
{
    std::string result;
    for (const char c : text) {
        if (c == '~') result += "~0";
        else if (c == '/') result += "~1";
        else result += c;
    }
    return result;
}

std::string path_fingerprint(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) inspection_error("CORE-INSP-0003", "Quelldaten waehrend Einlesen nicht lesbar");
    std::uint64_t hash = UINT64_C(14695981039346656037);
    std::uint64_t length = 0;
    std::string bytes{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    if (input.bad()) inspection_error("CORE-INSP-0003", "Lesefehler waehrend Einlesen");
    length = static_cast<std::uint64_t>(bytes.size());
    for (unsigned i = 0; i < 8; ++i) {
        hash = (hash ^ static_cast<unsigned char>(length & 255U)) * UINT64_C(1099511628211);
        length >>= 8U;
    }
    for (const unsigned char byte : bytes)
        hash = (hash ^ byte) * UINT64_C(1099511628211);
    return std::to_string(hash);
}

std::string without_paths(std::string text, const std::filesystem::path& base)
{
    for (const auto& form : {base.u8string(), base.generic_u8string()}) {
        if (form.empty()) continue;
        std::size_t position = 0;
        while ((position = text.find(form, position)) != std::string::npos) {
            text.replace(position, form.size(), ".");
            ++position;
        }
    }
    return text;
}

std::string error_id(const std::string& text, const std::string& fallback)
{
    if (text.rfind("CORE-", 0) != 0) return fallback;
    const auto end = text.find_first_of(" :[");
    return text.substr(0, end);
}

std::string safe_text(const std::string& value);
std::string logical_location(std::string value);

KatalogDiagnose diagnostic(const std::string& text, const std::filesystem::path& base,
                           const std::string& fallback)
{
    const auto id = error_id(text, fallback);
    std::string message = text;
    if (message.rfind(id, 0) == 0) message.erase(0, id.size());
    const auto close = message.find("]: ");
    std::string location;
    if (message.rfind(" [", 0) == 0 && close != std::string::npos) {
        location = message.substr(2, close - 2);
        message.erase(0, close + 3);
    } else if (message.rfind(": ", 0) == 0) message.erase(0, 2);
    return {id, safe_text(without_paths(message, base)),
        logical_location(without_paths(location, base))};
}

std::string safe_text(const std::string& value)
{
    if (value.find('\\') != std::string::npos || value.find('/') != std::string::npos)
        return "[Pfadreferenz ausgeblendet]";
    return value;
}

std::string logical_location(std::string value)
{
    const auto model_file = value.find("modells_information.json");
    const bool model_diagnostic = model_file != std::string::npos;
    if (model_file != std::string::npos) {
        value.erase(0, model_file + std::string("modells_information.json").size());
        while (!value.empty() && (value.front() == ',' || value.front() == ':' ||
               value.front() == ' ' || value.front() == '\\' || value.front() == '/'))
            value.erase(value.begin());
    }
    if (!model_diagnostic) {
        const auto comma = value.find(',');
        if (comma != std::string::npos) value.erase(comma);
    }
    return safe_text(value);
}

Json sanitized(Json value, const std::string& key = {})
{
    if (key == "datei") return Json{{"@geometrie", "geometrie:"}};
    if (value.is_string()) return safe_text(value.get<std::string>());
    if (value.is_array()) {
        for (auto& child : value) child = sanitized(std::move(child));
        return value;
    }
    if (value.is_object()) {
        for (auto it = value.begin(); it != value.end(); ++it)
            it.value() = sanitized(std::move(it.value()), it.key());
    }
    return value;
}

std::string value_type(const InspectionValue& value)
{
    return std::visit([](const auto& item) -> std::string {
        using T = std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<T, std::monostate>) return "null";
        else if constexpr (std::is_same_v<T, bool>) return "bool";
        else if constexpr (std::is_same_v<T, std::int64_t>) return "int";
        else if constexpr (std::is_same_v<T, double>) return "real";
        else if constexpr (std::is_same_v<T, std::string>) return "text";
        else if constexpr (std::is_same_v<T, std::vector<double>>) return "vector";
        else if constexpr (std::is_same_v<T, InspectionMatrix>) return "matrix";
        else if (item.is_array()) return "list";
        else return "object";
    }, value);
}

InspectionValue inspection_value(const Json& value)
{
    if (value.is_null()) return std::monostate{};
    if (value.is_boolean()) return value.get<bool>();
    if (value.is_number_integer()) return value.get<std::int64_t>();
    if (value.is_number()) return value.get<double>();
    if (value.is_string()) return value.get<std::string>();
    if (value.is_array()) {
        std::vector<double> vector;
        vector.reserve(value.size());
        for (const auto& entry : value) {
            if (!entry.is_number()) return value;
            vector.push_back(entry.get<double>());
        }
        if (vector.size() == 3) return vector;
        return value;
    }
    return value;
}

Json json_value(const InspectionValue& value)
{
    return std::visit([](const auto& item) -> Json {
        using T = std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<T, std::monostate>) return nullptr;
        else if constexpr (std::is_same_v<T, InspectionMatrix>)
            return Json{{"rows", item.rows}, {"columns", item.columns}, {"values", item.values}};
        else return item;
    }, value);
}

void add_property(std::vector<InspectionProperty>& properties, const std::string& node_id,
                  const std::string& object_path, const std::string& id, const std::string& name,
                  const std::string& unit, const std::string& status, InspectionValue value)
{
    const auto area = [&] {
        if (id == "@huellquader_m") return std::string("Geometrie");
        if (id == "@gesamtmasse_kg" || id == "@schwerpunkt_m" ||
            id == "@traegheit_kg_m2" || id == "@masse_kg" ||
            id == "@massenart" || id == "@dichte_kg_pro_m3" ||
            id == "@wanddicke_m")
            return std::string("Masse");
        if (id.rfind("@", 0) == 0 && id != "@element_id") return std::string("Bewegung");

        std::vector<std::string> parts;
        std::size_t start = object_path.empty() || object_path.front() != '/' ? 0 : 1;
        while (start < object_path.size()) {
            const auto end = object_path.find('/', start);
            parts.push_back(object_path.substr(start,
                end == std::string::npos ? std::string::npos : end - start));
            if (end == std::string::npos) break;
            start = end + 1;
        }
        std::string field;
        std::size_t field_index = 0;
        if (parts.size() >= 5 && parts[0] == "gruppen" && parts[2] == "elemente") {
            field_index = 4;
            field = parts[field_index];
        } else if (parts.size() >= 3 && parts[0] == "gruppen") {
            field_index = 2;
            field = parts[field_index];
        } else if (!parts.empty()) {
            field = parts.front();
        }
        if (field == "Darstellung") {
            for (std::size_t i = field_index + 1; i < parts.size(); ++i)
                if (parts[i] == "datei" || parts[i] == "mesh" || parts[i] == "geometrie")
                    return std::string("Geometrie");
            return std::string("Darstellung");
        }
        if (field == "Hydrostatik") {
            for (std::size_t i = field_index + 1; i < parts.size(); ++i) {
                if (parts[i] == "masse" || parts[i] == "massenart" ||
                    parts[i] == "dichte_kg_pro_m3" || parts[i] == "wanddicke_m")
                    return std::string("Masse");
                if (parts[i] == "material" || parts[i] == "materialreferenz")
                    return std::string("Material");
            }
            return std::string("Hydrostatik");
        }
        if (field == "Hydrodynamik") {
            for (std::size_t i = field_index + 1; i < parts.size(); ++i)
                if (parts[i] == "materialreferenz") return std::string("Material");
            return std::string("Hydrodynamik");
        }
        if (field == "Bewegung") return std::string("Bewegung");
        if (field == "geometrie" || field == "quelle" || field == "quellmeshes")
            return std::string("Geometrie");
        return std::string("Model");
    }();
    properties.push_back({node_id, object_path, id, name, value_type(value), unit,
        status, std::move(value), area});
}

std::vector<double> vector_value(const Vektor3& value)
{
    return {value[0], value[1], value[2]};
}

InspectionMatrix tensor_value(const std::array<double, 9>& value)
{
    return {3, 3, std::vector<double>(value.begin(), value.end())};
}

std::vector<std::string> unique_ids(std::vector<std::string> ids)
{
    std::set<std::string> seen;
    ids.erase(std::remove_if(ids.begin(), ids.end(), [&](const std::string& id) {
        return !seen.insert(id).second;
    }), ids.end());
    return ids;
}

bool is_domain_key(const std::string& key)
{
    return key == "Darstellung" || key == "Hydrostatik" ||
        key == "Hydrodynamik" || key == "Bewegung";
}

Json filtered_document(const Json& source, const std::string& tree_type,
                       std::vector<std::string>& element_ids)
{
    if (tree_type == "modell" || tree_type == "objekte") {
        const auto groups = source.find("gruppen");
        if (groups != source.end() && groups->is_object())
            for (const auto& group : *groups)
                if (group.contains("elemente") && group["elemente"].is_array())
                    for (const auto& element : group["elemente"])
                        if (element.contains("id") && element["id"].is_string())
                            element_ids.push_back(element["id"].get<std::string>());
        return source;
    }
    Json result = Json::object();
    for (const auto* key : {"id", "name", "beschreibung", "schema_version", "status", "entwurf"})
        if (source.contains(key)) result[key] = source[key];
    if (tree_type == "hydrodynamik" && source.contains("Hydrodynamische_Traegheit"))
        result["Hydrodynamische_Traegheit"] = source["Hydrodynamische_Traegheit"];
    const auto groups = source.find("gruppen");
    if (groups == source.end() || !groups->is_object()) return result;
    Json selected_groups = Json::object();
    for (const auto& [group_name, group] : groups->items()) {
        if (!group.is_object() || !group.contains("elemente") || !group["elemente"].is_array()) continue;
        Json selected_elements = Json::array();
        for (const auto& element : group["elemente"]) {
            if (!element.is_object() || !element.contains("id") || !element["id"].is_string()) continue;
            Json selected = Json::object();
            selected["id"] = element["id"];
            bool included = false;
            for (const auto& [key, value] : element.items()) {
                if (tree_type == "darstellung" && key == "Darstellung") {
                    selected[key] = value;
                    included = true;
                } else if (tree_type == "hydrostatik" && key == "Hydrostatik") {
                    selected[key] = value;
                    included = true;
                } else if (tree_type == "hydrodynamik" && key == "Hydrodynamik") {
                    selected[key] = value;
                    included = true;
                } else if (tree_type == "bewegung" && key == "Bewegung") {
                    selected[key] = value;
                    included = true;
                }
            }
            if (tree_type == "darstellung" && !selected.contains("Darstellung") &&
                element.contains("Hydrostatik") && element["Hydrostatik"].contains("volumen")) {
                selected["Hydrostatik"] = element["Hydrostatik"];
                included = true;
            }
            if (included) {
                selected_elements.push_back(std::move(selected));
                element_ids.push_back(element["id"].get<std::string>());
            }
        }
        if (tree_type == "bewegung" && group.contains("Bewegung")) {
            selected_elements = Json::array();
            for (const auto& element : group["elemente"]) {
                element_ids.push_back(element.at("id").get<std::string>());
                selected_elements.push_back({{"id", element.at("id")}});
            }
            selected_groups[group_name] = Json{
                {"elemente", std::move(selected_elements)}, {"Bewegung", group["Bewegung"]}};
        } else if (!selected_elements.empty())
            selected_groups[group_name] = Json{{"elemente", std::move(selected_elements)}};
    }
    result["gruppen"] = std::move(selected_groups);
    return result;
}

struct TreeData {
    std::vector<InspectionNode> nodes;
    std::vector<InspectionProperty> properties;
    std::map<std::string, std::size_t> node_index;
};

std::string child_path(const std::string& parent, const std::string& segment)
{
    return parent + "/" + pointer_escape(segment);
}

void append_value(TreeData& tree, const Json& value, const std::string& node_id,
                  const std::string& object_path, const std::string& field,
                  const std::string& element_id, const std::string& parent_kind,
                  const std::string& inherited_unit = {})
{
    const auto add_node = [&](const std::string& id, const std::string& parent,
                              const std::string& name, const std::string& kind) {
        tree.node_index[id] = tree.nodes.size();
        tree.nodes.push_back({id, parent, name, kind, element_id.empty()
            ? std::vector<std::string>{} : std::vector<std::string>{element_id}});
    };
    if (field == "datei") {
        add_property(tree.properties, node_id, object_path, pointer_escape(field), field, "",
            "vorhanden", std::string("geometrie:") + element_id);
        return;
    }
    if (value.is_object() || value.is_array()) {
        const auto id = child_path(node_id, field);
        add_node(id, node_id, field, value.is_array() ? "liste" : "objekt");
        const auto child_unit = value.is_object() && value.contains("einheit") &&
            value["einheit"].is_string() ? value["einheit"].get<std::string>() : inherited_unit;
        if (value.is_object()) {
            for (const auto& [key, child] : value.items())
                append_value(tree, child, id, child_path(object_path, key), key,
                    element_id, "objekt", child_unit);
        } else {
            for (std::size_t i = 0; i < value.size(); ++i)
                append_value(tree, value[i], id, child_path(object_path, std::to_string(i)),
                    std::to_string(i), element_id, "liste", inherited_unit);
        }
        return;
    }
    Json normalized = value;
    if (normalized.is_string()) normalized = safe_text(normalized.get<std::string>());
    std::string unit = inherited_unit;
    if (field == "wert") {
        unit = inherited_unit;
    } else if (field == "masse_kg" || field == "gesamtmasse_kg") {
        unit = "kg";
    } else if (field == "dichte_kg_pro_m3") {
        unit = "kg/m³";
    } else if (field == "traegheit_kg_m2" || field.find("traegheit") != std::string::npos) {
        unit = "kg m²";
    } else if (field.size() >= 2 && field.compare(field.size() - 2, 2, "_m") == 0) {
        unit = "m";
    } else if (field.size() >= 4 && field.compare(field.size() - 4, 4, "_rad") == 0) {
        unit = "rad";
    } else if (field == "reynolds_zahl" || field == "cl" || field == "cd") {
        unit = "1";
    }
    add_property(tree.properties, node_id, object_path, pointer_escape(field), field, unit,
        "vorhanden", inspection_value(normalized));
    (void)parent_kind;
}

void append_flat_properties(TreeData& tree, const Json& value, const std::string& node_id,
                            const std::string& object_path, const std::string& field,
                            const std::string& property_id, const std::string& inherited_unit = {})
{
    if (value.is_object() && !value.empty()) {
        const auto child_unit = value.contains("einheit") && value["einheit"].is_string()
            ? value["einheit"].get<std::string>() : inherited_unit;
        for (const auto& [key, child] : value.items())
            append_flat_properties(tree, child, node_id, child_path(object_path, key), key,
                property_id + "/" + pointer_escape(key), child_unit);
        return;
    }

    Json normalized = value;
    if (normalized.is_string()) normalized = safe_text(normalized.get<std::string>());
    std::string unit = inherited_unit;
    if (field == "masse_kg" || field == "gesamtmasse_kg") unit = "kg";
    else if (field == "dichte_kg_pro_m3") unit = "kg/m³";
    else if (field == "traegheit_kg_m2" || field.find("traegheit") != std::string::npos)
        unit = "kg m²";
    else if (field.size() >= 2 && field.compare(field.size() - 2, 2, "_m") == 0) unit = "m";
    else if (field.size() >= 4 && field.compare(field.size() - 4, 4, "_rad") == 0) unit = "rad";
    else if (field == "reynolds_zahl" || field == "cl" || field == "cd") unit = "1";
    add_property(tree.properties, node_id, object_path, property_id, field, unit,
        "vorhanden", inspection_value(normalized));
}

void build_tree(TreeData& tree, const Json& source, const std::string& tree_type,
                const std::vector<std::string>& element_ids)
{
    tree.node_index[""] = 0;
    tree.nodes.push_back({"", "", "Modell", "modell", element_ids});
    if (tree_type == "objekte") {
        for (const auto& [key, value] : source.items()) {
            if (key == "gruppen") continue;
            append_flat_properties(tree, value, "", child_path("", key), key,
                pointer_escape(key));
        }
        const auto groups = source.find("gruppen");
        if (groups == source.end() || !groups->is_object()) return;
        for (const auto& [group_name, group] : groups->items()) {
            const auto group_id = child_path(child_path("", "gruppen"), group_name);
            const auto group_members = group.value("elemente", Json::array());
            std::vector<std::string> group_ids;
            for (const auto& element : group_members)
                if (element.contains("id") && element["id"].is_string())
                    group_ids.push_back(element["id"].get<std::string>());
            tree.node_index[group_id] = tree.nodes.size();
            tree.nodes.push_back({group_id, "", group_name, "gruppe", unique_ids(group_ids)});
            for (const auto& [key, value] : group.items())
                if (key != "elemente")
                    append_flat_properties(tree, value, group_id, child_path(group_id, key), key,
                        pointer_escape(key));
            for (const auto& element : group_members) {
                const auto element_id = element["id"].get<std::string>();
                const auto node_id = child_path(child_path(group_id, "elemente"), element_id);
                tree.node_index[node_id] = tree.nodes.size();
                tree.nodes.push_back({node_id, group_id, element_id, "element", {element_id}});
                add_property(tree.properties, node_id, node_id, "@element_id", "Element-ID", "",
                    "vorhanden", element_id);
                for (const auto& [key, value] : element.items()) {
                    if (key == "id") continue;
                    append_flat_properties(tree, value, node_id, child_path(node_id, key), key,
                        pointer_escape(key));
                }
            }
        }
        return;
    }
    for (const auto& [key, value] : source.items()) {
        if (key == "gruppen" || (key == "Hydrodynamische_Traegheit" &&
            tree_type == "hydrodynamik")) continue;
        append_value(tree, value, "", child_path("", key), key, "", "modell");
    }
    if (tree_type == "hydrodynamik" && source.contains("Hydrodynamische_Traegheit"))
        append_value(tree, source["Hydrodynamische_Traegheit"], "", "/Hydrodynamische_Traegheit",
            "Hydrodynamische_Traegheit", "", "modell");
    const auto groups = source.find("gruppen");
    if (groups == source.end() || !groups->is_object()) return;
    for (const auto& [group_name, group] : groups->items()) {
        const auto group_id = child_path(child_path("", "gruppen"), group_name);
        const auto group_members = group.value("elemente", Json::array());
        std::vector<std::string> group_ids;
        for (const auto& element : group_members)
            if (element.contains("id") && element["id"].is_string())
                group_ids.push_back(element["id"].get<std::string>());
        tree.node_index[group_id] = tree.nodes.size();
        tree.nodes.push_back({group_id, "", group_name, "gruppe", unique_ids(group_ids)});
        for (const auto& [key, value] : group.items())
            if (key != "elemente")
                append_value(tree, value, group_id, child_path(group_id, key), key, "", "gruppe");
        const auto& elements = group["elemente"];
        for (const auto& element : elements) {
            const auto element_id = element["id"].get<std::string>();
            const auto id = child_path(child_path(group_id, "elemente"), element_id);
            tree.node_index[id] = tree.nodes.size();
            tree.nodes.push_back({id, group_id, element_id, "element", {element_id}});
            add_property(tree.properties, id, id, "@element_id", "Element-ID", "",
                "vorhanden", element_id);
            for (const auto& [key, value] : element.items()) {
                if (key == "id") continue;
                append_value(tree, value, id, child_path(id, key), key, element_id, "element");
            }
        }
    }
}

void add_derived_properties(TreeData& tree, const ModellBeschreibung& description,
                            const std::optional<Modell>& model,
                            const Darstellungsgeometrie& display,
                            const std::string& tree_type)
{
    if (tree_type == "modell" || tree_type == "objekte" || tree_type == "hydrostatik") {
        if (model) {
            add_property(tree.properties, "", "/", "@gesamtmasse_kg", "Gesamtmasse", "kg",
                "vorhanden", model->masse_kg);
            add_property(tree.properties, "", "/", "@schwerpunkt_m", "Gesamtschwerpunkt", "m",
                "vorhanden", vector_value(model->schwerpunkt_m));
            add_property(tree.properties, "", "/", "@traegheit_kg_m2", "Gesamtträgheit", "kg m²",
                "vorhanden", tensor_value(model->traegheit_kg_m2));
        } else {
            add_property(tree.properties, "", "/", "@gesamtmasse_kg", "Gesamtmasse", "kg",
                "fehlerhaft", std::monostate{});
        }
        std::optional<std::array<Vektor3, 2>> bounds;
        for (const auto& part : display.teile) for (const auto& point : part.punkte_m) {
            if (!bounds) bounds = std::array<Vektor3, 2>{point, point};
            for (std::size_t axis = 0; axis < 3; ++axis) {
                (*bounds)[0][axis] = std::min((*bounds)[0][axis], point[axis]);
                (*bounds)[1][axis] = std::max((*bounds)[1][axis], point[axis]);
            }
        }
        if (bounds) {
            Json box{{"min", vector_value((*bounds)[0])}, {"max", vector_value((*bounds)[1])}};
            add_property(tree.properties, "", "/", "@huellquader_m", "Hüllquader", "m",
                "vorhanden", box);
        } else {
            add_property(tree.properties, "", "/", "@huellquader_m", "Hüllquader", "m",
                "nicht_vorhanden", std::monostate{});
        }
    }
    if (!model) return;
    for (const auto& entry : model->elemente) {
        const auto group = std::find_if(description.elemente.begin(), description.elemente.end(),
            [&](const ElementBeschreibung& item) { return item.id == entry.id; });
        if (group == description.elemente.end()) continue;
        const auto node_id = child_path(
            child_path(child_path(child_path("", "gruppen"), group->gruppe), "elemente"),
            entry.id);
        if (tree_type == "modell" || tree_type == "objekte" || tree_type == "hydrostatik") {
            add_property(tree.properties, node_id, node_id, "@masse_kg", "Masse", "kg",
                "vorhanden", entry.masse_kg);
            add_property(tree.properties, node_id, node_id, "@schwerpunkt_m", "Schwerpunkt", "m",
                "vorhanden", vector_value(entry.schwerpunkt_m));
            add_property(tree.properties, node_id, node_id, "@traegheit_kg_m2", "Trägheit", "kg m²",
                "vorhanden", tensor_value(entry.traegheit_kg_m2));
            if (!entry.massenart.empty())
                add_property(tree.properties, node_id, node_id, "@massenart", "Massenart", "",
                    "vorhanden", entry.massenart);
            if (entry.dichte_kg_pro_m3)
                add_property(tree.properties, node_id, node_id, "@dichte_kg_pro_m3", "Dichte",
                    "kg/m³", "vorhanden", *entry.dichte_kg_pro_m3);
            if (entry.wanddicke_m)
                add_property(tree.properties, node_id, node_id, "@wanddicke_m", "Wanddicke",
                    "m", "vorhanden", *entry.wanddicke_m);
        }
        if ((tree_type == "bewegung" || tree_type == "objekte") && entry.bewegung &&
            !description.gruppenbewegungen.count(group->gruppe)) {
            const auto movement_id = child_path(node_id, "Bewegung");
            const auto property_node_id = tree_type == "objekte" ? node_id : movement_id;
            add_property(tree.properties, property_node_id, movement_id, "@hubweg_m", "Hubweg", "m",
                "vorhanden", entry.bewegung->hubweg_m);
            add_property(tree.properties, property_node_id, movement_id, "@steuerung",
                "Bediensemantik", "", "vorhanden",
                entry.bewegung->ruderwinkel ? std::string("ruderwinkel") : std::string("stellung_prozent"));
        }
    }
}

void mark_unvalidated_physics(TreeData& tree)
{
    for (auto& property : tree.properties) {
        const auto& path = property.object_path;
        if (path.find("/Hydrostatik") == std::string::npos &&
            path.find("/Hydrodynamik") == std::string::npos &&
            path.find("/Bewegung") == std::string::npos &&
            path.find("/Hydrodynamische_Traegheit") == std::string::npos)
            continue;
        property.status = "fehlerhaft";
        property.value = std::monostate{};
        property.value_type = "null";
    }
}

void append_mesh(std::vector<InspectionMesh>& meshes, const std::string& type,
                 const std::string& element_id, const std::string& group,
                 const std::string& origin, const std::string& topology,
                 const std::vector<Vektor3>& points,
                 const std::vector<std::array<std::size_t, 3>>& triangles,
                 std::optional<std::array<double, 3>> color = std::nullopt,
                 std::optional<std::string> surface = std::nullopt)
{
    meshes.push_back({"mesh/" + pointer_escape(type) + "/" + pointer_escape(element_id),
        element_id, group, type, origin, topology, points, triangles, color, surface});
}

Json diagnostic_json(const std::vector<KatalogDiagnose>& diagnostics)
{
    Json result = Json::array();
    for (const auto& entry : diagnostics)
        result.push_back({{"fehler_id", entry.fehler_id}, {"text", entry.text},
            {"fundstelle", entry.fundstelle}});
    return result;
}

Json envelope(const InspectionInfo& info, const std::string& payload,
              Json value, bool mesh = false)
{
    Json result{{"inspektionsversion", "1"}, {"model_id", info.model_id},
        {"model_version", info.model_version}, {"snapshot_id", info.snapshot_id},
        {"status", info.status}, {"diagnostics", diagnostic_json(info.diagnostics)},
        {payload, std::move(value)}};
    if (mesh) {
        result["frame"] = "modell";
        result["length_unit"] = "m";
    }
    return result;
}

Json node_json(const InspectionNode& node)
{
    return {{"id", node.id}, {"parent_id", node.parent_id}, {"name", node.name},
        {"kind", node.kind}, {"element_ids", node.element_ids}, {"mesh_ids", node.mesh_ids}};
}

Json property_json(const InspectionProperty& property)
{
    return {{"node_id", property.node_id}, {"object_path", property.object_path},
        {"id", property.id}, {"name", property.name}, {"area", property.area},
        {"value_type", property.value_type},
        {"unit", property.unit}, {"status", property.status},
        {"value", property.status == "vorhanden" ? json_value(property.value) : Json(nullptr)}};
}

Json mesh_json(const InspectionMesh& mesh)
{
    Json result{{"id", mesh.id}, {"element_id", mesh.element_id}, {"group", mesh.group},
        {"type", mesh.type}, {"origin", mesh.origin}, {"topology", mesh.topology},
        {"points_m", mesh.points_m}, {"triangles", mesh.triangles},
        {"color_rgb", mesh.color_rgb ? Json(*mesh.color_rgb) : Json(nullptr)},
        {"surface", mesh.surface ? Json(*mesh.surface) : Json(nullptr)},
        {"source_id", mesh.source_id}, {"provenance", mesh.provenance},
        {"derivation", mesh.derivation}, {"element_ids", mesh.element_ids}};
    if (mesh.montage) {
        const auto& mounting = *mesh.montage;
        result["montage"] = {{"element_id", mounting.element_id},
            {"bezugspunkt_lokal_m", mounting.bezugspunkt_lokal_m},
            {"laengsrichtung_lokal", mounting.laengsrichtung_lokal},
            {"querrichtung_lokal", mounting.querrichtung_lokal},
            {"rotation_xyzw", mounting.rotation_xyzw},
            {"translation_m", mounting.translation_m},
            {"herkunft", mounting.herkunft},
            {"quellen_fingerprint", mounting.quellen_fingerprint}};
    }
    return result;
}

Json movement_pose_json(const InspectionMovementPose& pose)
{
    Json meshes = Json::array();
    for (const auto& mesh : pose.meshes) meshes.push_back(mesh_json(mesh));
    return {{"label", pose.label}, {"value", pose.value}, {"unit", pose.unit},
        {"meshes", std::move(meshes)}};
}

Json movement_preview_json(const InspectionMovementPreview& preview)
{
    Json poses = Json::array();
    for (const auto& pose : preview.poses) poses.push_back(movement_pose_json(pose));
    return {{"element_id", preview.element_id}, {"kind", preview.kind},
        {"frame", preview.frame},
        {"axis_point_m", preview.axis_point_m ? Json(*preview.axis_point_m) : Json(nullptr)},
        {"axis_direction", preview.axis_direction},
        {"reference_label", preview.reference_label},
        {"reference_value", preview.reference_value}, {"value_unit", preview.value_unit},
        {"minimum_value", preview.minimum_value ? Json(*preview.minimum_value) : Json(nullptr)},
        {"maximum_value", preview.maximum_value ? Json(*preview.maximum_value) : Json(nullptr)},
        {"poses", std::move(poses)}};
}

InspectionMesh transformed_mesh(InspectionMesh mesh, const std::array<double, 16>& matrix,
                                const std::string& pose_label)
{
    for (auto& point : mesh.points_m) {
        const auto x = point[0], y = point[1], z = point[2];
        point = {
            matrix[0] * x + matrix[1] * y + matrix[2] * z + matrix[3],
            matrix[4] * x + matrix[5] * y + matrix[6] * z + matrix[7],
            matrix[8] * x + matrix[9] * y + matrix[10] * z + matrix[11]};
    }
    mesh.id += "/preview/" + pointer_escape(pose_label);
    mesh.provenance = "abgeleitet";
    mesh.derivation = "Bewegung/" + pose_label;
    return mesh;
}

}

struct ModelInspection::Impl {
    InspectionInfo info;
    ModellBeschreibung description;
    std::optional<Modell> model;
    Darstellungsgeometrie display;
    std::vector<InspectionType> tree_types;
    std::map<std::string, TreeData> trees;
    std::vector<InspectionType> mesh_types;
    std::vector<InspectionMesh> meshes;
    std::set<std::string> element_ids;
    bool legacy_model_usable{};
};

ModelInspection::ModelInspection(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}
ModelInspection::~ModelInspection() = default;
const InspectionInfo& ModelInspection::info() const { return impl_->info; }

std::shared_ptr<const ModelInspection> readModelInspection(
    const Modellkatalog& katalog, const std::string& model_id,
    const std::string& expected_model_version)
{
    const auto& model_file = katalog_datei(katalog, model_id, expected_model_version);
    const auto model_folder = model_file.parent_path();
    auto impl = std::make_unique<ModelInspection::Impl>();
    impl->info.model_id = model_id;
    impl->info.model_version = expected_model_version;
    const auto* catalogue_entry = katalog.eintrag(model_id);
    if (catalogue_entry &&
        (!catalogue_entry->schema_valide || !catalogue_entry->visualisierung_hash_aktuell))
        inspection_error("CORE-INSP-0003", "Schema- oder Visualisierungspruefung sperrt die Inspektion");
    impl->legacy_model_usable = catalogue_entry && catalogue_entry->status == "verwendbar";
    static std::atomic<std::uint64_t> next_snapshot{1};
    impl->info.snapshot_id = expected_model_version + ":" +
        std::to_string(next_snapshot.fetch_add(1, std::memory_order_relaxed));
    impl->info.status = "verwendbar";

    ModellInspektionsImport imported;
    try {
        imported = lade_inspektionsimport(model_file);
    } catch (const std::exception& error) {
        throw std::runtime_error("CORE-INSP-0003: " +
            without_paths(error.what(), model_folder));
    }
    impl->description = std::move(imported.beschreibung);
    const Json& mounting_document = imported.modelldokument;
    std::vector<std::pair<std::filesystem::path, std::string>> display_versions;
    for (const auto& entry : impl->description.elemente)
        if (entry.darstellung)
            display_versions.emplace_back(*entry.darstellung, path_fingerprint(*entry.darstellung));
    impl->display = darstellungsgeometrie_aus_beschreibung(
        model_id, expected_model_version, model_file, impl->description, std::nullopt);
    for (const auto& [path, before] : display_versions)
        if (path_fingerprint(path) != before)
            inspection_error("CORE-INSP-0003", "Darstellungsdatei waehrend Einlesen geaendert");
    impl->model = std::move(imported.modell);
    if (!imported.import_fehler.empty()) {
        impl->info.status = "nur_ansicht";
        impl->info.diagnostics.push_back({"CORE-INSP-0004",
            "Teilaufbereitung fehlgeschlagen", "Modellinspektion"});
    }
    for (const auto& entry : impl->display.diagnosen)
        impl->info.diagnostics.push_back({entry.fehler_id,
            safe_text(without_paths(entry.text, model_folder)),
            logical_location(without_paths(entry.fundstelle, model_folder))});
    try {
        for (const auto& entry : modelldiagnose(katalog, model_id, expected_model_version))
            impl->info.diagnostics.push_back({entry.fehler_id,
                safe_text(without_paths(entry.text, model_folder)),
                logical_location(without_paths(entry.fundstelle, model_folder))});
    } catch (const std::exception& error) {
        impl->info.diagnostics.push_back(diagnostic(error.what(), model_folder, "CORE-INSP-0004"));
    }
    if (!impl->info.diagnostics.empty()) impl->info.status = "nur_ansicht";

    const auto source = sanitized(impl->description.dokument);
    const std::array<std::pair<const char*, const char*>, 6> types{{
        {"modell", "Modell"}, {"objekte", "Objekte"}, {"darstellung", "GUI"},
        {"hydrostatik", "Hydrostatik"},
        {"hydrodynamik", "Hydrodynamik"}, {"bewegung", "Bewegung"}}};
    for (const auto& [type, name] : types) {
        std::vector<std::string> element_ids;
        auto view = filtered_document(source, type, element_ids);
        if (type != std::string("modell") && type != std::string("objekte") && element_ids.empty() &&
            !(type == std::string("hydrodynamik") && view.contains("Hydrodynamische_Traegheit")))
            continue;
        element_ids = unique_ids(std::move(element_ids));
        TreeData tree;
        build_tree(tree, view, type, element_ids);
        if (!impl->model) mark_unvalidated_physics(tree);
        impl->trees.emplace(type, std::move(tree));
        impl->tree_types.push_back({type, name, element_ids});
    }

    std::map<std::string, std::string> group_by_id;
    for (const auto& entry : impl->description.elemente) {
        group_by_id[entry.id] = entry.gruppe;
        impl->element_ids.insert(entry.id);
    }
    for (const auto& group : mounting_document.at("gruppen"))
        for (const auto& definition : group.at("elemente")) {
            if (definition.contains("Darstellung") || !definition.contains("Hydrostatik") ||
                !definition.at("Hydrostatik").contains("masse"))
                continue;
            const auto& mass = definition.at("Hydrostatik").at("masse");
            if (mass.value("art", std::string{}) != "punktmasse") continue;
            const auto id = definition.at("id").get<std::string>();
            if (std::any_of(impl->display.teile.begin(), impl->display.teile.end(),
                [&](const Darstellungsteil& part) { return part.element_id == id; }))
                continue;
            auto point = mass.at("position_m").get<Vektor3>();
            if (const auto* setting = element_setting(mounting_document, id))
                if (setting->at("konfiguriert").get<bool>())
                    point = setting_pose(*setting).modellpunkt_nach_welt(point);
            impl->display.teile.push_back({id, group_by_id.at(id), "punktmasse_referenz",
                {point}, {}, std::nullopt, std::nullopt});
        }
    std::set<std::string> displayed_ids;
    for (const auto& part : impl->display.teile) {
        if (!displayed_ids.insert(part.element_id).second)
            inspection_error("CORE-INSP-0004", "Mehrdeutige aktuelle Darstellungsreferenz: " + part.element_id);
        append_mesh(impl->meshes, "darstellung", part.element_id, part.gruppe,
            part.herkunft, part.dreiecke.empty() ? "punkte" : "dreiecke", part.punkte_m, part.dreiecke,
            part.farbe_rgb, part.oberflaeche);
    }
    if (impl->model) for (const auto& element : impl->model->elemente) {
        const auto group = group_by_id.find(element.id);
        const std::string group_name = group == group_by_id.end() ? "" : group->second;
        if (element.volumen) {
            append_mesh(impl->meshes, "hydrostatik_volumen", element.id, group_name,
                "Hydrostatik.volumen", "dreiecke", element.volumen->punkte,
                element.volumen->dreiecke);
            if (element.hydrodynamik_rolle)
                append_mesh(impl->meshes, "hydrodynamik", element.id, group_name,
                    "Hydrostatik.volumen", "dreiecke", element.volumen->punkte,
                    element.volumen->dreiecke);
        }
        if (element.massen_mesh)
            append_mesh(impl->meshes, "massen_mesh", element.id, group_name,
                "Hydrostatik.masse", "dreiecke", element.massen_mesh->punkte,
                element.massen_mesh->dreiecke);
        if (!element.massenpunkte_m.empty())
            append_mesh(impl->meshes, "massenpunkte", element.id, group_name,
                element.massenart == "punktmasse" ? "Hydrostatik.masse.punktmasse" :
                    "Hydrostatik.masse.Meshpunkte", "punkte", element.massenpunkte_m, {});
        if (!element.flutrand_m.empty())
            append_mesh(impl->meshes, "flutrand", element.id, group_name,
                "Hydrostatik.masse.Schalenrand", "punkte", element.flutrand_m, {});
    }
    for (auto& mesh : impl->meshes) {
        mesh.element_ids = {mesh.element_id};
        const auto entry = std::find_if(impl->description.elemente.begin(), impl->description.elemente.end(),
            [&](const ElementBeschreibung& e) { return e.id == mesh.element_id; });
        if (entry == impl->description.elemente.end()) continue;
        std::optional<std::string> quelle;
        if (mesh.type == "darstellung")
            quelle = entry->darstellung ? entry->darstellung_mesh : entry->volumen_mesh;
        else if (mesh.type == "hydrostatik_volumen" || mesh.type == "hydrodynamik")
            quelle = entry->volumen_mesh;
        else quelle = entry->masse_mesh;
        mesh.source_id = quelle.value_or("");
        if (mesh.type == "darstellung") {
            MontageReferenz mounting;
            mounting.element_id = mesh.element_id;
            mounting.herkunft = "Deklarierte Objektgeometrie im Modellbezug";
            if (const auto* setting = element_setting(impl->description.dokument, mesh.element_id)) {
                if (setting->at("konfiguriert").get<bool>()) {
                    mounting.rotation_xyzw = setting->at("rotation_xyzw").get<QuaternionXyzw>();
                    mounting.translation_m = setting->at("translation_m").get<Vektor3>();
                }
            }
            mounting.quellen_fingerprint = impl->info.model_version;
            for (const auto& group : mounting_document.at("gruppen"))
                for (const auto& definition : group.at("elemente")) {
                    if (definition.at("id") != mesh.element_id) continue;
                    if (mesh.origin == "punktmasse_referenz") {
                        mounting.bezugspunkt_lokal_m = definition.at("Hydrostatik").at("masse").at("position_m").get<Vektor3>();
                        mounting.herkunft = "Explizite Punktmassenposition im Modellbezug";
                    }
                    if (!definition.contains("Montage")) continue;
                    const auto& metadata = definition.at("Montage");
                    if (!metadata.contains("anschlag_vertex_indices")) continue;
                    if (!entry->darstellung)
                        inspection_error("CORE-INSP-0004", "Montageindices ohne deklarierte OBJ-Quelle");
                    const auto& path = *entry->darstellung;
                    const auto raw = lade_quellmesh(path, std::nullopt, true);
                    const auto reference_point = [&](const Json& indices) {
                        if (!indices.is_array() || indices.empty())
                            inspection_error("CORE-INSP-0004", "Leere OBJ-Montagereferenz");
                        Vektor3 result{};
                        for (const auto& index : indices) {
                            if (!index.is_number_unsigned() || index.get<std::size_t>() == 0 ||
                                index.get<std::size_t>() > raw.punkte.size())
                                inspection_error("CORE-INSP-0004", "Veraltete OBJ-Montagereferenz: " +
                                    mesh.element_id + "; " + index.dump() + "; Vertices=" +
                                    std::to_string(raw.punkte.size()));
                            const auto& point = raw.punkte[index.get<std::size_t>() - 1];
                            for (std::size_t i = 0; i < 3; ++i) result[i] += point[i];
                        }
                        for (auto& value : result) value /= static_cast<double>(indices.size());
                        return result;
                    };
                    mounting.bezugspunkt_lokal_m = reference_point(metadata.at("anschlag_vertex_indices"));
                    if (metadata.contains("ende_vertex_indices")) {
                        const auto end = reference_point(metadata.at("ende_vertex_indices"));
                        double length_squared = 0.;
                        for (std::size_t i = 0; i < 3; ++i) {
                            mounting.laengsrichtung_lokal[i] = end[i] - mounting.bezugspunkt_lokal_m[i];
                            length_squared += mounting.laengsrichtung_lokal[i] * mounting.laengsrichtung_lokal[i];
                        }
                        if (!(length_squared > 1e-24))
                            inspection_error("CORE-INSP-0004", "Entartete OBJ-Montageachse");
                    }
                    mounting.herkunft = "Explizite Objekt-Montageindices in deklarierter OBJ-Quelle";
                    mounting.quellen_fingerprint = path_fingerprint(path);
                }
            mesh.montage = std::move(mounting);
        }
        mesh.provenance = mesh.type == "flutrand" || mesh.type == "massenpunkte" ?
            "abgeleitet" : "quelle";
        if (mesh.type == "flutrand") mesh.derivation = "Offene Schalenrandknoten aus Kanteninzidenz";
        else if (mesh.type == "massenpunkte") mesh.derivation = quelle ?
            "Extrahierte Quellvertices; keine berechneten Massenpunkte" :
            "Punktposition aus expliziter JSON-Punktmasse";
        else mesh.derivation = "Unveraenderte Quellgeometrie fuer " + mesh.type;
        const auto role = mesh.type == "darstellung" ?
            (entry->darstellung ? "Darstellung" : "volumen") :
            (mesh.type == "hydrostatik_volumen" || mesh.type == "hydrodynamik" ? "volumen" : "masse");
        if (const auto* definition = geometrieableitung(impl->description, entry->id, role)) {
            mesh.provenance = "abgeleitet";
            mesh.derivation += "; " + definition->dump();
            if (mesh.type != "flutrand" && mesh.type != "massenpunkte")
                mesh.derivation = definition->dump();
        }
    }
    for (const auto& q : impl->description.quellmeshes) {
        InspectionMesh mesh;
        mesh.id = "mesh/quelle/" + pointer_escape(q.id);
        mesh.source_id = q.id;
        mesh.type = "quelle";
        mesh.origin = q.ursprung;
        mesh.provenance = "quelle";
        mesh.derivation = "Unveraenderte OBJ-Quellgeometrie";
        mesh.topology = "dreiecke";
        mesh.points_m = q.mesh.punkte;
        mesh.triangles = q.mesh.dreiecke;
        for (const auto& entry : impl->description.elemente)
            if (entry.darstellung_mesh == q.id || entry.volumen_mesh == q.id || entry.masse_mesh == q.id) {
                mesh.element_ids.push_back(entry.id);
                if (mesh.element_id.empty()) {
                    mesh.element_id = entry.id;
                    mesh.group = entry.gruppe;
                    mesh.color_rgb = entry.farbe_rgb;
                    mesh.surface = entry.oberflaeche;
                }
            }
        impl->meshes.push_back(std::move(mesh));
    }
    for (auto& [type, tree] : impl->trees) for (auto& node : tree.nodes) {
        if (node.kind != "element") continue;
        for (const auto& mesh : impl->meshes)
            if ((type != "objekte" || mesh.type == "darstellung") &&
                std::any_of(mesh.element_ids.begin(), mesh.element_ids.end(), [&](const std::string& id) {
                return std::find(node.element_ids.begin(), node.element_ids.end(), id) != node.element_ids.end();
            })) node.mesh_ids.push_back(mesh.id);
    }
    const std::array<std::pair<const char*, const char*>, 7> mesh_type_names{{
        {"darstellung", "GUI"}, {"hydrostatik_volumen", "Hydrostatik-Volumen"},
        {"hydrodynamik", "Hydrodynamik"}, {"massen_mesh", "Massenmesh"},
        {"massenpunkte", "Massenpunkte"}, {"flutrand", "Flutrand"}, {"quelle", "OBJ-Quellen"}}};
    for (const auto& [type, name] : mesh_type_names) {
        std::vector<std::string> ids;
        bool present = false;
        for (const auto& mesh : impl->meshes) if (mesh.type == type) {
            present = true;
            ids.insert(ids.end(), mesh.element_ids.begin(), mesh.element_ids.end());
        }
        ids = unique_ids(std::move(ids));
        if (present) impl->mesh_types.push_back({type, name, std::move(ids)});
    }

    try {
        katalog_datei(katalog, model_id, expected_model_version);
    } catch (const std::runtime_error& error) {
        if (std::string(error.what()).rfind("CORE-KAT-0004", 0) == 0)
            inspection_error("CORE-INSP-0003", "Quelldaten waehrend Einlesen geaendert");
        throw;
    }
    auto* current = impl.get();
    auto inspection = std::shared_ptr<const ModelInspection>(new ModelInspection(std::move(impl)));
    if (!detail::ist_modellformat03_datei(model_file)) {
    const auto sails = leseSegelKonfiguration(katalog, *inspection);
    if (sails.status == SegelKonfigurationsStatus::Fehler) {
        current->info.status = "nur_ansicht";
        for (const auto& diag : sails.diagnosen)
            current->info.diagnostics.push_back({diag.code, diag.nachricht, diag.element_id});
    } else {
        const auto geometry = leseKonfigurierteSegelgeometrie(sails);
        if (geometry.status == SegelErgebnisStatus::Fehler) {
            current->info.status = "nur_ansicht";
            for (const auto& diag : geometry.diagnosen)
                current->info.diagnostics.push_back({diag.code, diag.nachricht, diag.element_id});
        }
        for (const auto& replacement : geometry.meshes) {
            const auto reference = std::find_if(sails.referenzen.begin(), sails.referenzen.end(),
                [&](const SegelReferenz& ref) { return ref.element_id == replacement.element_id; });
            const auto pose = std::find_if(sails.lagen.begin(), sails.lagen.end(),
                [&](const SegelLage& lage) { return lage.element_id == replacement.element_id; });
            for (auto& mesh : current->meshes) {
                if (mesh.type != "darstellung" || mesh.element_id != replacement.element_id) continue;
                mesh.points_m = replacement.points_m;
                mesh.triangles = replacement.triangles;
                if (reference != sails.referenzen.end() && pose != sails.lagen.end()) {
                    mesh.montage = reference->montage;
                    mesh.montage->rotation_xyzw = pose->rotation_xyzw;
                    mesh.montage->translation_m = pose->translation_m;
                }
            }
            for (auto& part : current->display.teile) {
                if (part.element_id != replacement.element_id) continue;
                part.punkte_m = replacement.points_m;
                part.dreiecke = replacement.triangles;
            }
        }
        for (const auto& diag : sails.diagnosen)
            current->info.diagnostics.push_back({diag.code, diag.nachricht, diag.element_id});
    }
    }
    for (auto& [type, tree] : current->trees)
        add_derived_properties(tree, current->description, current->model, current->display, type);
    return inspection;
}

std::vector<InspectionType> getAvailableObjectTreeTypes(const ModelInspection& inspection)
{
    return inspection.impl_->tree_types;
}

std::vector<InspectionNode> getObjectTree(const ModelInspection& inspection,
                                          const std::string& tree_type)
{
    const auto found = inspection.impl_->trees.find(tree_type);
    if (found == inspection.impl_->trees.end())
        inspection_error("CORE-INSP-0001", "Unbekannter oder nicht verfuegbarer Baumtyp");
    return found->second.nodes;
}

std::vector<InspectionProperty> getObjectProperties(const ModelInspection& inspection,
    const std::string& tree_type, const std::vector<std::string>& node_ids, bool includeChildren)
{
    const auto found = inspection.impl_->trees.find(tree_type);
    if (found == inspection.impl_->trees.end())
        inspection_error("CORE-INSP-0001", "Unbekannter oder nicht verfuegbarer Baumtyp");
    const auto& tree = found->second;
    std::set<std::string> selected;
    for (const auto& id : node_ids) {
        if (tree.node_index.find(id) == tree.node_index.end())
            inspection_error("CORE-INSP-0002", "Unbekannte Knoten-ID im Baum");
        selected.insert(id);
        if (includeChildren && !id.empty()) {
            const auto prefix = id + "/";
            for (const auto& node : tree.nodes)
                if (node.id.rfind(prefix, 0) == 0) selected.insert(node.id);
        } else if (includeChildren && id.empty()) {
            for (const auto& node : tree.nodes) selected.insert(node.id);
        }
    }
    std::set<std::tuple<std::string, std::string, std::string>> emitted;
    std::vector<InspectionProperty> result;
    for (const auto& property : tree.properties)
        if (selected.count(property.node_id) &&
            emitted.emplace(tree_type, property.node_id, property.id).second)
            result.push_back(property);
    return result;
}

std::optional<InspectionMovementPreview> getMovementPreview(
    const ModelInspection& inspection, const std::string& element_id)
{
    const auto& impl = *inspection.impl_;
    if (!impl.element_ids.count(element_id))
        inspection_error("CORE-INSP-0002", "Unbekannte Element-ID");
    const auto description = std::find_if(impl.description.elemente.begin(),
        impl.description.elemente.end(),
        [&](const ElementBeschreibung& element) { return element.id == element_id; });
    if (description != impl.description.elemente.end() &&
        impl.description.gruppenbewegungen.count(description->gruppe))
        return std::nullopt;
    if (!impl.model)
        inspection_error("CORE-INSP-0004", "Bewegungsvorschau wegen ungueltiger Physik nicht verfuegbar");
    const auto controller = std::find_if(impl.model->elemente.begin(), impl.model->elemente.end(),
        [&](const Element& element) { return element.id == element_id; });
    if (controller == impl.model->elemente.end() || !controller->bewegung)
        return std::nullopt;

    const auto& definition = *controller->bewegung;
    InspectionMovementPreview preview;
    preview.element_id = element_id;
    preview.kind = definition.art == Bewegungsart::drehen ? "drehen" : "heben";
    if (definition.art == Bewegungsart::drehen) {
        preview.axis_point_m = definition.drehpunkt_modell_m;
        preview.axis_direction = definition.achse_modell;
    } else {
        preview.axis_direction = definition.richtung_modell;
    }
    preview.reference_label = "Normalstellung";
    preview.reference_value = definition.ruderwinkel ? 0. : 100.;
    preview.value_unit = definition.ruderwinkel ? "deg" : "%";
    if (!definition.ruderwinkel) {
        preview.minimum_value = 0.;
        preview.maximum_value = 100.;
    }

    std::vector<std::string> affected_ids{element_id};
    affected_ids.insert(affected_ids.end(), definition.mitbewegte_elemente.begin(),
        definition.mitbewegte_elemente.end());
    affected_ids = unique_ids(std::move(affected_ids));
    std::vector<const InspectionMesh*> source_meshes;
    for (const auto& mesh : impl.meshes)
        if (mesh.type == "darstellung" &&
            std::find(affected_ids.begin(), affected_ids.end(), mesh.element_id) != affected_ids.end())
            source_meshes.push_back(&mesh);
    if (source_meshes.empty())
        inspection_error("CORE-INSP-0004", "Bewegungsvorschau ohne Darstellungsmesh");

    const auto append_pose = [&](const std::string& label, double value) {
        InspectionMovementPose pose{label, value, preview.value_unit, {}};
        for (const auto* mesh : source_meshes) {
            const auto transform = bewegungstransformation(*impl.model, mesh->element_id,
                {{element_id, value}});
            pose.meshes.push_back(transformed_mesh(*mesh, transform, label));
        }
        preview.poses.push_back(std::move(pose));
    };
    append_pose("normal", preview.reference_value);
    if (preview.minimum_value) append_pose("minimum", *preview.minimum_value);
    if (preview.maximum_value) append_pose("maximum", *preview.maximum_value);
    return preview;
}

std::optional<InspectionGroupMovementPreview> getGroupMovementPreview(
    const ModelInspection& inspection, const std::string& group_id)
{
    const auto& impl = *inspection.impl_;
    if (std::find(impl.description.gruppen.begin(), impl.description.gruppen.end(), group_id) ==
        impl.description.gruppen.end())
        inspection_error("CORE-INSP-0002", "Unbekannte Gruppen-ID");
    const auto found = impl.description.gruppenbewegungen.find(group_id);
    if (found == impl.description.gruppenbewegungen.end()) return std::nullopt;
    const auto& movement = found->second;
    const auto& group = impl.description.dokument.at("gruppen").at(group_id);
    const auto& data = group.at("Bewegung");
    const bool sail_group = group_id == "mainsail" || group_id == "jib";
    InspectionGroupMovementPreview preview;
    preview.group_id = group_id;
    preview.kind = movement.art == Bewegungsart::drehen ? "drehen" : "heben";
    if (movement.art == Bewegungsart::drehen) {
        preview.axis_point_m = movement.drehpunkt_modell_m;
        preview.axis_direction = movement.achse_modell;
    } else {
        preview.axis_direction = movement.richtung_modell;
    }
    preview.reference_label = "Normalstellung";
    preview.value_unit = movement.art == Bewegungsart::drehen ? "deg" : "m";
    if (data.contains("control")) {
        const auto& control = data.at("control");
        const auto scale = control.value("unit", std::string{}) == "cm" ? .01 : 1.;
        preview.minimum_value = control.at("minimum").get<double>() * scale;
        preview.maximum_value = control.at("maximum").get<double>() * scale;
        preview.reference_value = control.value("neutral", control.at("minimum").get<double>()) * scale;
    }

    std::vector<const InspectionMesh*> meshes;
    for (const auto& member : movement.mitbewegte_elemente) {
        const auto description = std::find_if(impl.description.elemente.begin(),
            impl.description.elemente.end(),
            [&](const ElementBeschreibung& item) { return item.id == member; });
        const auto mesh = std::find_if(impl.meshes.begin(), impl.meshes.end(),
            [&](const InspectionMesh& item) {
                return description != impl.description.elemente.end() && description->darstellung &&
                    item.type == "darstellung" && item.element_id == member &&
                    item.origin != "volumen_ersatz" && item.origin != "punktmasse_referenz";
            });
        if (mesh != impl.meshes.end()) meshes.push_back(&*mesh);
        else preview.diagnostics.push_back({"CORE-INSP-0004",
            "Visualisierungsmesh fuer Gruppenmitglied nicht verfuegbar", member});
    }

    const auto dot = [](const Vektor3& a, const Vektor3& b) {
        return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
    };
    const auto subtract = [](const Vektor3& a, const Vektor3& b) {
        return Vektor3{a[0]-b[0], a[1]-b[1], a[2]-b[2]};
    };
    const auto cross = [](const Vektor3& a, const Vektor3& b) {
        return Vektor3{a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2],
            a[0]*b[1]-a[1]*b[0]};
    };
    const auto axis_length = std::sqrt(dot(preview.axis_direction, preview.axis_direction));
    if (!(axis_length > 1e-12))
        inspection_error("CORE-INSP-0004", "Ungueltige Gruppenbewegungsachse");
    Vektor3 axis = preview.axis_direction;
    for (auto& value : axis) value /= axis_length;
    std::set<std::string> sail_elements;
    if (sail_group) for (const auto& element : group.at("elemente")) {
        const auto placement = element.find("placement");
        const auto display = element.find("Darstellung");
        if ((placement != element.end() && placement->contains("originalTack")) ||
            (display != element.end() && display->value("oberflaeche", std::string{}) == "stoff"))
            sail_elements.insert(element.at("id").get<std::string>());
    }

    const auto mirror_sail = [&](InspectionMesh& mesh) -> bool {
        std::map<std::pair<std::size_t, std::size_t>, unsigned> edges;
        for (const auto& triangle : mesh.triangles)
            for (std::size_t i = 0; i < 3; ++i) {
                auto a = triangle[i], b = triangle[(i + 1) % 3];
                if (a >= mesh.points_m.size() || b >= mesh.points_m.size()) return false;
                if (a > b) std::swap(a, b);
                ++edges[{a, b}];
            }
        std::set<std::size_t> boundary;
        for (const auto& edge : edges) if (edge.second == 1) {
            boundary.insert(edge.first.first);
            boundary.insert(edge.first.second);
        }
        if (boundary.size() < 3 || !preview.axis_point_m) return false;
        double nearest = std::numeric_limits<double>::infinity(), farthest = -1.;
        std::size_t outer = 0;
        for (const auto index : boundary) {
            const auto relative = subtract(mesh.points_m[index], *preview.axis_point_m);
            const auto along = dot(relative, axis);
            const double distance = std::max(0., dot(relative, relative) - along * along);
            nearest = std::min(nearest, distance);
            if (distance > farthest) { farthest = distance; outer = index; }
        }
        const double tolerance = std::max(1e-16, farthest * 1e-10);
        std::optional<std::size_t> first, second;
        double min_along = std::numeric_limits<double>::infinity();
        double max_along = -std::numeric_limits<double>::infinity();
        for (const auto index : boundary) {
            const auto relative = subtract(mesh.points_m[index], *preview.axis_point_m);
            const auto along = dot(relative, axis);
            const double distance = std::max(0., dot(relative, relative) - along * along);
            if (distance > nearest + tolerance) continue;
            if (along < min_along) { min_along = along; first = index; }
            if (along > max_along) { max_along = along; second = index; }
        }
        if (!first || !second || *first == *second) return false;
        const auto normal = cross(subtract(mesh.points_m[*second], mesh.points_m[*first]),
            subtract(mesh.points_m[outer], mesh.points_m[*first]));
        const auto normal_squared = dot(normal, normal);
        if (!(normal_squared > 1e-20)) return false;
        const auto origin = mesh.points_m[*first];
        for (auto& point : mesh.points_m) {
            const auto distance = 2. * dot(subtract(point, origin), normal) /
                normal_squared;
            for (std::size_t i = 0; i < 3; ++i) point[i] -= distance * normal[i];
        }
        for (auto& triangle : mesh.triangles) std::swap(triangle[1], triangle[2]);
        return true;
    };

    const auto append_pose = [&](const std::string& label, double value) {
        InspectionMovementPose pose{label, value, preview.value_unit, {}};
        if (sail_group && label != "normal" && sail_elements.empty()) {
            preview.diagnostics.push_back({"CORE-INSP-0004",
                "Segel innerhalb der Gruppe nicht eindeutig bestimmbar", group_id});
            return;
        }
        for (const auto* original : meshes) {
            auto mesh = *original;
            if (sail_group && label != "normal" && sail_elements.count(mesh.element_id)) {
                const auto element = std::find_if(group.at("elemente").begin(),
                    group.at("elemente").end(), [&](const Json& entry) {
                        return entry.at("id") == mesh.element_id;
                    });
                if (element != group.at("elemente").end()) {
                    const auto tack = element->value("placement", Json::object())
                        .value("originalTack", std::string("unknown"));
                    if (tack == "unknown") {
                        preview.diagnostics.push_back({"CORE-INSP-0004",
                            "Originalbuglage des Segels ist unbekannt", mesh.element_id});
                        return;
                    }
                    const bool mirror = (value > 0. && tack == "starboard") ||
                        (value < 0. && tack == "port");
                    if (mirror && !mirror_sail(mesh)) {
                        preview.diagnostics.push_back({"CORE-INSP-0004",
                            "Spiegelebene aus Visualisierungsmesh nicht bestimmbar", mesh.element_id});
                        return;
                    }
                }
            }
            const auto delta = (value - preview.reference_value) * (sail_group ? -1. : 1.);
            const auto radians = delta * 3.14159265358979323846 / 180.;
            for (auto& point : mesh.points_m) {
                if (movement.art == Bewegungsart::heben) {
                    for (std::size_t i = 0; i < 3; ++i) point[i] += axis[i] * delta;
                } else {
                    const auto relative = subtract(point, *preview.axis_point_m);
                    const auto rotated = cross(axis, relative);
                    const auto projection = dot(axis, relative);
                    for (std::size_t i = 0; i < 3; ++i)
                        point[i] = (*preview.axis_point_m)[i] + relative[i] * std::cos(radians) +
                            rotated[i] * std::sin(radians) +
                            axis[i] * projection * (1. - std::cos(radians));
                }
            }
            mesh.id += "/preview/" + pointer_escape(label);
            mesh.provenance = "abgeleitet";
            mesh.derivation = "Gruppenbewegung/" + label;
            pose.meshes.push_back(std::move(mesh));
        }
        if (pose.meshes.size() == movement.mitbewegte_elemente.size())
            preview.poses.push_back(std::move(pose));
    };
    append_pose("normal", preview.reference_value);
    if (preview.minimum_value) append_pose("minimum", *preview.minimum_value);
    if (preview.maximum_value) append_pose("maximum", *preview.maximum_value);
    if (sail_group && movement.art == Bewegungsart::drehen) {
        append_pose("backbord", 90.);
        append_pose("steuerbord", -90.);
    }
    return preview;
}

std::vector<InspectionType> getAvailableMeshTypes(const ModelInspection& inspection)
{
    return inspection.impl_->mesh_types;
}

std::vector<InspectionMesh> getMesh(const ModelInspection& inspection, const std::string& mesh_type,
    const std::optional<std::vector<std::string>>& element_ids)
{
    const auto type = std::find_if(inspection.impl_->mesh_types.begin(), inspection.impl_->mesh_types.end(),
        [&](const InspectionType& value) { return value.id == mesh_type; });
    if (type == inspection.impl_->mesh_types.end())
        inspection_error("CORE-INSP-0001", "Unbekannter oder nicht verfuegbarer Mesh-Typ");
    std::set<std::string> selected;
    if (element_ids) {
        for (const auto& id : *element_ids) {
            if (!inspection.impl_->element_ids.count(id))
                inspection_error("CORE-INSP-0002", "Unbekannte Element-ID");
            selected.insert(id);
        }
    }
    std::vector<InspectionMesh> result;
    for (const auto& mesh : inspection.impl_->meshes)
        if (mesh.type == mesh_type &&
            (!element_ids || std::any_of(mesh.element_ids.begin(), mesh.element_ids.end(),
                [&](const std::string& id) { return selected.count(id) != 0; })))
            result.push_back(mesh);
    return result;
}

MastspitzenErgebnis bestimmeMastspitze(const ModelInspection& inspection)
{
    const auto structure = modellstruktur(inspection);
    std::vector<std::string> candidates;
    std::vector<std::string> main_masts;
    for (const auto& group : structure.gruppen) {
        for (const auto& element : group.elemente) {
            std::string id = element.id;
            std::transform(id.begin(), id.end(), id.begin(),
                [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
            if (id == "hauptmast" || id == "mainmast") {
                main_masts.push_back(element.id);
                candidates.push_back(element.id);
            } else if (id == "mast" || id == "vormast" || id == "besanmast" ||
                       id == "foremast" || id == "mizzenmast") {
                candidates.push_back(element.id);
            }
        }
    }
    if (main_masts.size() == 1) return bestimmeMastspitze(inspection, main_masts.front());
    if (main_masts.empty() && candidates.size() == 1)
        return bestimmeMastspitze(inspection, candidates.front());
    if (candidates.empty()) return {{}, {}, "Kein eindeutig bezeichneter Mast vorhanden"};
    return {{}, {}, "Hauptmast nicht eindeutig"};
}

MastspitzenErgebnis bestimmeMastspitze(const ModelInspection& inspection,
                                       const std::string& mast_element_id)
{
    const auto structure = modellstruktur(inspection);
    bool found = false;
    for (const auto& group : structure.gruppen) {
        for (const auto& element : group.elemente)
            found = found || element.id == mast_element_id;
    }
    if (mast_element_id.empty() || !found)
        return {{}, mast_element_id, "Angegebener Mast nicht vorhanden"};

    const auto types = getAvailableMeshTypes(inspection);
    if (std::none_of(types.begin(), types.end(),
                     [](const InspectionType& type) { return type.id == "darstellung"; }))
        return {{}, mast_element_id, "Keine Mast-Darstellungsgeometrie vorhanden"};
    const auto meshes = getMesh(inspection, "darstellung", std::vector<std::string>{mast_element_id});
    double max_z = -std::numeric_limits<double>::infinity();
    Vektor3 sum{0., 0., 0.};
    std::size_t count = 0;
    for (const auto& mesh : meshes) {
        if (mesh.origin != "darstellung") continue;
        for (const auto& point : mesh.points_m) {
            if (!std::isfinite(point[0]) || !std::isfinite(point[1]) || !std::isfinite(point[2]))
                return {{}, mast_element_id, "Mast-Darstellungsgeometrie enthaelt ungueltige Koordinaten"};
            if (point[2] > max_z) {
                max_z = point[2];
                sum = point;
                count = 1;
            } else if (point[2] == max_z) {
                sum[0] += point[0];
                sum[1] += point[1];
                sum[2] += point[2];
                ++count;
            }
        }
    }
    if (!count) return {{}, mast_element_id, "Keine verwendbare Mast-Darstellungsgeometrie vorhanden"};
    const double n = static_cast<double>(count);
    return {Vektor3{sum[0] / n, sum[1] / n, sum[2] / n}, mast_element_id, {},
        "mast_darstellungsgeometrie"};
}

Modellstruktur modellstruktur(const ModelInspection& inspection)
{
    Modellstruktur result;
    result.modell_id = inspection.impl_->info.model_id;
    result.modell_fassung = inspection.impl_->info.model_version;
    const auto tree = getObjectTree(inspection, "modell");
    for (const auto& node : tree) {
        if (node.kind == "gruppe") {
            StrukturGruppe group{node.name, node.name, {}};
            const auto movement = inspection.impl_->description.gruppenbewegungen.find(node.name);
            if (movement != inspection.impl_->description.gruppenbewegungen.end())
                group.bewegung = movement->second;
            for (const auto& child : tree)
                if (child.parent_id == node.id && child.kind == "element") {
                    const auto display = std::any_of(inspection.impl_->meshes.begin(),
                        inspection.impl_->meshes.end(), [&](const InspectionMesh& mesh) {
                            return mesh.element_id == child.name && mesh.type == "darstellung";
                        });
                    const auto volume = inspection.impl_->model &&
                        std::any_of(inspection.impl_->model->elemente.begin(),
                            inspection.impl_->model->elemente.end(), [&](const Element& element) {
                                return element.id == child.name && element.volumen.has_value();
                            });
                    const auto description = std::find_if(inspection.impl_->description.elemente.begin(),
                        inspection.impl_->description.elemente.end(), [&](const ElementBeschreibung& e) {
                            return e.id == child.name;
                        });
                    group.elemente.push_back({child.name, child.name, display, volume,
                        description == inspection.impl_->description.elemente.end()
                            ? std::nullopt : description->bewegung,
                        description != inspection.impl_->description.elemente.end() &&
                            description->referenz_visualisierung,
                        description != inspection.impl_->description.elemente.end() &&
                            description->referenz_hydrostatik,
                        description != inspection.impl_->description.elemente.end() &&
                            description->referenz_hydrodynamik,
                        description != inspection.impl_->description.elemente.end() &&
                            description->referenz_aerodynamik});
                }
            result.gruppen.push_back(std::move(group));
        }
    }
    if (inspection.impl_->legacy_model_usable && inspection.impl_->model)
        result.gesamtmasse_kg = inspection.impl_->model->masse_kg;
    for (const auto& mesh : inspection.impl_->meshes) if (mesh.type == "darstellung")
        for (const auto& point : mesh.points_m) {
            if (!result.huellquader_m) result.huellquader_m = std::array<Vektor3, 2>{point, point};
            for (std::size_t axis = 0; axis < 3; ++axis) {
                (*result.huellquader_m)[0][axis] =
                    std::min((*result.huellquader_m)[0][axis], point[axis]);
                (*result.huellquader_m)[1][axis] =
                    std::max((*result.huellquader_m)[1][axis], point[axis]);
            }
        }
    return result;
}

Darstellungsgeometrie darstellungsgeometrie(const ModelInspection& inspection,
    const std::optional<std::string>& element_id)
{
    Darstellungsgeometrie result{inspection.impl_->info.model_id,
        inspection.impl_->info.model_version, {}, inspection.impl_->display.diagnosen};
    if (element_id && !inspection.impl_->element_ids.count(*element_id))
        throw std::runtime_error("CORE-KAT-0006: Unbekannte Element-ID: " + *element_id);
    for (const auto& mesh : inspection.impl_->meshes) {
        if (mesh.type != "darstellung" || (element_id && mesh.element_id != *element_id)) continue;
        result.teile.push_back({mesh.element_id, mesh.group, mesh.origin,
            mesh.points_m, mesh.triangles, mesh.color_rgb, mesh.surface});
    }
    return result;
}

std::string inspectionInfoJson(const ModelInspection& inspection)
{
    const auto& info = inspection.info();
    Json payload{{"model_id", info.model_id}, {"model_version", info.model_version},
        {"snapshot_id", info.snapshot_id}, {"status", info.status},
        {"diagnostics", diagnostic_json(info.diagnostics)}};
    return envelope(info, "info", std::move(payload)).dump();
}

std::string inspectionTreeTypesJson(const ModelInspection& inspection)
{
    Json values = Json::array();
    for (const auto& type : getAvailableObjectTreeTypes(inspection))
        values.push_back({{"id", type.id}, {"name", type.name}, {"element_ids", type.element_ids}});
    return envelope(inspection.info(), "tree_types", std::move(values)).dump();
}

std::string inspectionTreeJson(const ModelInspection& inspection, const std::string& tree_type)
{
    Json values = Json::array();
    for (const auto& node : getObjectTree(inspection, tree_type)) values.push_back(node_json(node));
    return envelope(inspection.info(), "nodes", std::move(values)).dump();
}

std::string inspectionPropertiesJson(const ModelInspection& inspection,
    const std::string& tree_type, const std::vector<std::string>& node_ids, bool includeChildren)
{
    Json values = Json::array();
    for (const auto& property : getObjectProperties(inspection, tree_type, node_ids, includeChildren))
        values.push_back(property_json(property));
    return envelope(inspection.info(), "properties", std::move(values)).dump();
}

std::string movementPreviewJson(const ModelInspection& inspection, const std::string& element_id)
{
    const auto preview = getMovementPreview(inspection, element_id);
    return envelope(inspection.info(), "preview",
        preview ? movement_preview_json(*preview) : Json(nullptr)).dump();
}

std::string groupMovementPreviewJson(const ModelInspection& inspection, const std::string& group_id)
{
    const auto preview = getGroupMovementPreview(inspection, group_id);
    if (!preview) return envelope(inspection.info(), "preview", Json(nullptr)).dump();
    Json poses = Json::array();
    for (const auto& pose : preview->poses) poses.push_back(movement_pose_json(pose));
    Json result{{"group_id", preview->group_id}, {"kind", preview->kind},
        {"frame", preview->frame},
        {"axis_point_m", preview->axis_point_m ? Json(*preview->axis_point_m) : Json(nullptr)},
        {"axis_direction", preview->axis_direction},
        {"reference_label", preview->reference_label},
        {"reference_value", preview->reference_value}, {"value_unit", preview->value_unit},
        {"minimum_value", preview->minimum_value ? Json(*preview->minimum_value) : Json(nullptr)},
        {"maximum_value", preview->maximum_value ? Json(*preview->maximum_value) : Json(nullptr)},
        {"poses", std::move(poses)}, {"diagnostics", diagnostic_json(preview->diagnostics)}};
    return envelope(inspection.info(), "preview", std::move(result)).dump();
}

std::string inspectionMeshTypesJson(const ModelInspection& inspection)
{
    Json values = Json::array();
    for (const auto& type : getAvailableMeshTypes(inspection))
        values.push_back({{"id", type.id}, {"name", type.name}, {"element_ids", type.element_ids}});
    return envelope(inspection.info(), "mesh_types", std::move(values)).dump();
}

std::string inspectionMeshJson(const ModelInspection& inspection, const std::string& mesh_type,
    const std::optional<std::vector<std::string>>& element_ids)
{
    Json values = Json::array();
    for (const auto& mesh : getMesh(inspection, mesh_type, element_ids))
        values.push_back(mesh_json(mesh));
    return envelope(inspection.info(), "meshes", std::move(values), true).dump();
}

}
