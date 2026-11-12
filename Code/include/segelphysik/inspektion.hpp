#pragma once

#include "segelphysik/katalog.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace segelphysik {

struct InspectionMatrix {
    std::size_t rows{};
    std::size_t columns{};
    std::vector<double> values;
};

using InspectionValue = std::variant<
    std::monostate, bool, std::int64_t, double, std::string,
    std::vector<double>, InspectionMatrix, nlohmann::json>;

struct InspectionType {
    std::string id;
    std::string name;
    std::vector<std::string> element_ids;
};

struct InspectionNode {
    std::string id;
    std::string parent_id;
    std::string name;
    std::string kind;
    std::vector<std::string> element_ids;
    std::vector<std::string> mesh_ids;
};

struct InspectionProperty {
    std::string node_id;
    std::string object_path;
    std::string id;
    std::string name;
    std::string value_type;
    std::string unit;
    std::string status;
    InspectionValue value;
    std::string area;
};

struct MontageReferenz {
    std::string element_id;
    Vektor3 bezugspunkt_lokal_m{};
    Vektor3 laengsrichtung_lokal{0., 0., 1.};
    Vektor3 querrichtung_lokal{-1., 0., 0.};
    QuaternionXyzw rotation_xyzw{0., 0., 0., 1.};
    Vektor3 translation_m{};
    std::string herkunft;
    std::string quellen_fingerprint;
};

struct InspectionMesh {
    std::string id;
    std::string element_id;
    std::string group;
    std::string type;
    std::string origin;
    std::string topology;
    std::vector<Vektor3> points_m;
    std::vector<std::array<std::size_t, 3>> triangles;
    std::optional<std::array<double, 3>> color_rgb;
    std::optional<std::string> surface;
    std::string source_id;
    std::string provenance;
    std::string derivation;
    std::vector<std::string> element_ids;
    std::optional<MontageReferenz> montage;
};

struct InspectionMovementPose {
    std::string label;
    double value{};
    std::string unit;
    std::vector<InspectionMesh> meshes;
};

struct InspectionMovementPreview {
    std::string element_id;
    std::string kind;
    std::string frame = "modell";
    std::optional<Vektor3> axis_point_m;
    Vektor3 axis_direction{};
    std::string reference_label;
    double reference_value{};
    std::string value_unit;
    std::optional<double> minimum_value;
    std::optional<double> maximum_value;
    std::vector<InspectionMovementPose> poses;
};

struct InspectionGroupMovementPreview {
    std::string group_id;
    std::string kind;
    std::string frame = "modell";
    std::optional<Vektor3> axis_point_m;
    Vektor3 axis_direction{};
    std::string reference_label;
    double reference_value{};
    std::string value_unit;
    std::optional<double> minimum_value;
    std::optional<double> maximum_value;
    std::vector<InspectionMovementPose> poses;
    std::vector<KatalogDiagnose> diagnostics;
};

struct InspectionInfo {
    std::string model_id;
    std::string model_version;
    std::string snapshot_id;
    std::string status;
    std::vector<KatalogDiagnose> diagnostics;
};

class ModelInspection {
public:
    ~ModelInspection();
    ModelInspection(const ModelInspection&) = delete;
    ModelInspection& operator=(const ModelInspection&) = delete;
    ModelInspection(ModelInspection&&) = delete;
    ModelInspection& operator=(ModelInspection&&) = delete;

    const InspectionInfo& info() const;

private:
    struct Impl;
    explicit ModelInspection(std::unique_ptr<Impl> impl);
    std::unique_ptr<const Impl> impl_;

    friend std::shared_ptr<const ModelInspection> readModelInspection(
        const Modellkatalog&, const std::string&, const std::string&);
    friend Modellstruktur modellstruktur(const ModelInspection&);
    friend Darstellungsgeometrie darstellungsgeometrie(
        const ModelInspection&, const std::optional<std::string>&);
    friend std::vector<InspectionType> getAvailableObjectTreeTypes(const ModelInspection&);
    friend std::vector<InspectionNode> getObjectTree(const ModelInspection&, const std::string&);
    friend std::vector<InspectionProperty> getObjectProperties(
        const ModelInspection&, const std::string&, const std::vector<std::string>&, bool);
    friend std::optional<InspectionMovementPreview> getMovementPreview(
        const ModelInspection&, const std::string&);
    friend std::optional<InspectionGroupMovementPreview> getGroupMovementPreview(
        const ModelInspection&, const std::string&);
    friend std::vector<InspectionType> getAvailableMeshTypes(const ModelInspection&);
    friend std::vector<InspectionMesh> getMesh(
        const ModelInspection&, const std::string&,
        const std::optional<std::vector<std::string>>&);
    friend std::string inspectionInfoJson(const ModelInspection&);
    friend std::string inspectionTreeTypesJson(const ModelInspection&);
    friend std::string inspectionTreeJson(const ModelInspection&, const std::string&);
    friend std::string inspectionPropertiesJson(
        const ModelInspection&, const std::string&, const std::vector<std::string>&, bool);
    friend std::string movementPreviewJson(
        const ModelInspection&, const std::string&);
    friend std::string inspectionMeshTypesJson(const ModelInspection&);
    friend std::string inspectionMeshJson(
        const ModelInspection&, const std::string&,
        const std::optional<std::vector<std::string>>&);
};

std::shared_ptr<const ModelInspection> readModelInspection(
    const Modellkatalog& katalog, const std::string& model_id,
    const std::string& expected_model_version);
std::vector<InspectionType> getAvailableObjectTreeTypes(const ModelInspection& inspection);
std::vector<InspectionNode> getObjectTree(
    const ModelInspection& inspection, const std::string& tree_type);
std::vector<InspectionProperty> getObjectProperties(
    const ModelInspection& inspection, const std::string& tree_type,
    const std::vector<std::string>& node_ids, bool includeChildren = true);
std::optional<InspectionMovementPreview> getMovementPreview(
    const ModelInspection& inspection, const std::string& element_id);
std::optional<InspectionGroupMovementPreview> getGroupMovementPreview(
    const ModelInspection& inspection, const std::string& group_id);
std::vector<InspectionType> getAvailableMeshTypes(const ModelInspection& inspection);
std::vector<InspectionMesh> getMesh(
    const ModelInspection& inspection, const std::string& mesh_type,
    const std::optional<std::vector<std::string>>& element_ids = std::nullopt);

std::string inspectionInfoJson(const ModelInspection& inspection);
std::string inspectionTreeTypesJson(const ModelInspection& inspection);
std::string inspectionTreeJson(const ModelInspection& inspection, const std::string& tree_type);
std::string inspectionPropertiesJson(
    const ModelInspection& inspection, const std::string& tree_type,
    const std::vector<std::string>& node_ids, bool includeChildren = true);
std::string movementPreviewJson(
    const ModelInspection& inspection, const std::string& element_id);
std::string groupMovementPreviewJson(
    const ModelInspection& inspection, const std::string& group_id);
std::string inspectionMeshTypesJson(const ModelInspection& inspection);
std::string inspectionMeshJson(
    const ModelInspection& inspection, const std::string& mesh_type,
    const std::optional<std::vector<std::string>>& element_ids = std::nullopt);

}
