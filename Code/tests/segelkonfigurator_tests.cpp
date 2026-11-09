#include "segelphysik/segelkonfigurator.hpp"

#include <algorithm>
#include <chrono>
#include <atomic>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {

using namespace segelphysik;
namespace fs = std::filesystem;

void require(bool condition, const std::string& message)
{
    if (!condition) throw std::runtime_error(message);
}

bool close(double a, double b, double tolerance = 1e-9)
{
    return std::abs(a - b) <= tolerance;
}

double dot(const Vektor3& a, const Vektor3& b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

Vektor3 add(const Vektor3& a, const Vektor3& b)
{
    return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}

Vektor3 subtract(const Vektor3& a, const Vektor3& b)
{
    return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

Vektor3 scale(const Vektor3& a, double f)
{
    return {a[0] * f, a[1] * f, a[2] * f};
}

Vektor3 rotate(const QuaternionXyzw& q, const Vektor3& p)
{
    const double x = q[0], y = q[1], z = q[2], w = q[3];
    return {
        (1. - 2. * (y * y + z * z)) * p[0] +
            2. * (x * y - z * w) * p[1] + 2. * (x * z + y * w) * p[2],
        2. * (x * y + z * w) * p[0] +
            (1. - 2. * (x * x + z * z)) * p[1] + 2. * (y * z - x * w) * p[2],
        2. * (x * z - y * w) * p[0] + 2. * (y * z + x * w) * p[1] +
            (1. - 2. * (x * x + y * y)) * p[2]};
}

Vektor3 transform_point(const SegelLage& lage, const Vektor3& p)
{
    return add(rotate(lage.rotation_xyzw, p), lage.translation_m);
}

const SegelReferenz& reference(const SegelKonfigurationSnapshot& snapshot,
                               const std::string& id)
{
    for (const auto& value : snapshot.referenzen)
        if (value.element_id == id) return value;
    throw std::runtime_error("Expected sail reference is absent: " + id);
}

const SegelLage& lage(const SegelKonfigurationSnapshot& snapshot,
                      const std::string& id)
{
    for (const auto& value : snapshot.lagen)
        if (value.element_id == id) return value;
    throw std::runtime_error("Expected sail pose is absent: " + id);
}

void replace_lage(SegelKonfigurationSnapshot& snapshot, const SegelLage& replacement)
{
    for (auto& value : snapshot.lagen) {
        if (value.element_id == replacement.element_id) {
            value = replacement;
            return;
        }
    }
    throw std::runtime_error("Cannot update test snapshot pose");
}

void check_status(const SegelKonfigurationErgebnis& result, SegelErgebnisStatus expected,
                 const std::string& context)
{
    if (result.status != expected) {
        const auto detail = result.diagnosen.empty() ? std::string{} :
            ": " + result.diagnosen.front().code + " " + result.diagnosen.front().nachricht;
        throw std::runtime_error(context + detail);
    }
}

double point_distance(const Vektor3& a, const Vektor3& b)
{
    const auto delta = subtract(a, b);
    return std::sqrt(dot(delta, delta));
}

std::string read_binary_file(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    require(static_cast<bool>(input), "Cannot read test file: " + path.string());
    return std::string(std::istreambuf_iterator<char>(input),
                       std::istreambuf_iterator<char>());
}

nlohmann::json& model_element(nlohmann::json& model, const std::string& id)
{
    for (auto& group : model["gruppen"])
        for (auto& element : group["elemente"])
            if (element.value("id", std::string{}) == id) return element;
    throw std::runtime_error("Test element missing: " + id);
}

void strip_settings(nlohmann::json& model)
{
    model.erase("konfiguration");
    for (auto& group : model["gruppen"])
        for (auto& element : group["elemente"])
            element.erase("setting");
}

bool has_temporary_configuration_file(const fs::path& root)
{
    const auto prefix = std::string("modells_information.json.tmp.");
    for (const auto& entry : fs::directory_iterator(root))
        if (entry.path().filename().u8string().find(prefix) == 0) return true;
    return false;
}

Vektor3 unit(const Vektor3& value)
{
    return scale(value, 1. / point_distance({0., 0., 0.}, value));
}

Vektor3 triangle_normal(const std::vector<Vektor3>& points,
                        const std::array<std::size_t, 3>& triangle)
{
    const auto a = subtract(points[triangle[1]], points[triangle[0]]);
    const auto b = subtract(points[triangle[2]], points[triangle[0]]);
    return unit({a[1] * b[2] - a[2] * b[1],
                 a[2] * b[0] - a[0] * b[2],
                 a[0] * b[1] - a[1] * b[0]});
}

std::vector<Vektor3> read_obj_vertices_independently(const fs::path& path)
{
    std::ifstream input(path);
    require(static_cast<bool>(input), "Cannot independently read rig OBJ: " + path.string());
    std::vector<Vektor3> vertices;
    std::string line;
    while (std::getline(input, line)) {
        std::istringstream row(line);
        std::string marker;
        row >> marker;
        if (marker != "v") continue;
        Vektor3 value{};
        require(static_cast<bool>(row >> value[0] >> value[1] >> value[2]),
                "Malformed rig OBJ vertex in independent reference check");
        vertices.push_back(value);
    }
    require(!input.bad(), "Failed reading rig OBJ in independent reference check");
    return vertices;
}

Vektor3 independent_section_center(const std::vector<Vektor3>& vertices,
                                   std::size_t first, std::size_t count)
{
    require(count > 0 && first <= vertices.size() && count <= vertices.size() - first,
            "Independent OBJ reference section is absent");
    Vektor3 result{};
    for (std::size_t i = first; i < first + count; ++i)
        result = add(result, vertices[i]);
    return scale(result, 1. / static_cast<double>(count));
}

double angle_between(const Vektor3& a, const Vektor3& b)
{
    const Vektor3 product{
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0]};
    const double product_norm = std::sqrt(dot(product, product));
    return std::atan2(product_norm, dot(a, b));
}

SegelKonfigurationSnapshot fixture(bool quadrilateral)
{
    SegelKonfigurationSnapshot snapshot;
    snapshot.modell_id = "fixture";
    snapshot.revision = "7";
    snapshot.status = SegelKonfigurationsStatus::Bereit;

    SegelReferenz reference;
    reference.element_id = "FixtureSegel";
    reference.intrinsische_quelle_id = "fixture-source";
    reference.darstellungs_mesh_id = "fixture-mesh";
    reference.lookup_id = "fixture-lookup";
    reference.art = SegelArt::Grosssegel;
    reference.ecken.hals_m = {0., 0., 0.};
    reference.ecken.schothorn_m = {-2., 0., 0.};
    reference.ecken.kopf_vorliek_m = {0., 0., 3.};
    if (quadrilateral) reference.ecken.kopf_achterliek_m = Vektor3{-0.2, 0., 3.};
    reference.rig_hals_anschlag_m = {0., 0., 0.};
    reference.rig_vorliek_richtung_modell = {0., 0., 1.};
    reference.rig_unterliek_richtung_modell = {-1., 0., 0.};
    reference.original_buglage = SegelBuglage::Backbord;
    snapshot.referenzen.push_back(reference);
    SegelLage initial_lage;
    initial_lage.element_id = "FixtureSegel";
    initial_lage.original_buglage = SegelBuglage::Backbord;
    snapshot.lagen.push_back(initial_lage);

    InspectionMesh mesh;
    mesh.id = "fixture-mesh";
    mesh.element_id = "FixtureSegel";
    mesh.points_m = {{0., 0., 0.}, {-2., 0., 0.}, {0., 0., 3.}};
    if (quadrilateral) mesh.points_m.push_back({-0.2, 0., 3.});
    mesh.triangles = {{0, 1, 2}};
    if (quadrilateral) mesh.triangles.push_back({1, 3, 2});
    snapshot.grundmeshes.push_back(mesh);
    return snapshot;
}

void test_synthetic_operations()
{
    const auto plane_cases = std::array<std::pair<SegelEbene, Vektor3>, 3>{{
        {SegelEbene::XY, {1., 2., 0.}},
        {SegelEbene::XZ, {1., 0., 2.}},
        {SegelEbene::YZ, {0., 1., 2.}}}};
    for (const bool quadrilateral : {false, true}) {
        const auto snapshot = fixture(quadrilateral);
        for (const auto& test_case : plane_cases) {
            SegelAktionAnforderung translation;
            translation.aktion = SegelAktion::Translation;
            translation.element_id = "FixtureSegel";
            translation.ebene = test_case.first;
            translation.translation_m = test_case.second;
            const auto translated = pruefeSegelAktion(snapshot, translation);
            check_status(translated, SegelErgebnisStatus::Vorschau,
                         "Synthetic plane translation should preserve shape");
            require(translated.meshes.size() == 1, "Fixture preview must return one mesh");
            const auto& original_points = snapshot.grundmeshes[0].points_m;
            const auto& transformed_points = translated.meshes[0].points_m;
            for (std::size_t i = 0; i < original_points.size(); ++i)
                for (std::size_t axis = 0; axis < 3; ++axis)
                    require(close(transformed_points[i][axis] - original_points[i][axis],
                                  test_case.second[axis]),
                            "Translation must be constrained to selected model axes");

            SegelAktionAnforderung rotate_corner;
            rotate_corner.aktion = SegelAktion::Rotation;
            rotate_corner.element_id = "FixtureSegel";
            rotate_corner.ebene = test_case.first;
            rotate_corner.bewegte_ecke = quadrilateral ?
                SegelEcke::KopfAchterliek : SegelEcke::Schothorn;
            rotate_corner.winkel_grad = 37.;
            const auto rotated = pruefeSegelAktion(snapshot, rotate_corner);
            check_status(rotated, SegelErgebnisStatus::Vorschau,
                         "Synthetic triangular/quadrilateral rotation should succeed");
            require(rotated.lage.has_value(), "Rotation must include transformed rigid pose");
            const auto pivot = transform_point(*rotated.lage, {0., 0., 0.});
            require(close(pivot[0], 0.) && close(pivot[1], 0.) && close(pivot[2], 0.),
                    "Non-tack corner rotation must keep hals pivot fixed");
            const auto& rotation_source_points = snapshot.grundmeshes[0].points_m;
            const auto& rotation_result_points = rotated.meshes[0].points_m;
            for (std::size_t i = 0; i < rotation_source_points.size(); ++i)
                for (std::size_t j = i + 1; j < rotation_source_points.size(); ++j)
                    require(close(point_distance(rotation_source_points[i],
                                           rotation_source_points[j]),
                                  point_distance(rotation_result_points[i],
                                           rotation_result_points[j]), 1e-10),
                            "Rigid rotation must preserve all point-pair distances");

            rotate_corner.bewegte_ecke = SegelEcke::Hals;
            const auto hals_drag = pruefeSegelAktion(snapshot, rotate_corner);
            check_status(hals_drag, SegelErgebnisStatus::Vorschau,
                         "Hals drag must be a rigid rotation around head");
            require(hals_drag.lage.has_value(), "Hals drag must include rigid pose");
            const auto head_pivot = transform_point(*hals_drag.lage, {0., 0., 3.});
            require(close(head_pivot[0], 0.) && close(head_pivot[1], 0.) &&
                    close(head_pivot[2], 3.),
                    "Dragging hals must keep the forward head pivot fixed");
        }
    }

    auto angle_snapshot = fixture(true);
    const auto positive = stelleSegelStellwinkelVorschauDar(
        angle_snapshot, "FixtureSegel", 90.);
    const auto negative = stelleSegelStellwinkelVorschauDar(
        angle_snapshot, "FixtureSegel", -90.);
    check_status(positive, SegelErgebnisStatus::Vorschau,
                 "Positive local -Z angle must render to backbord");
    check_status(negative, SegelErgebnisStatus::Vorschau,
                 "Negative local -Z angle must render to steuerbord");
    require(close(positive.meshes[0].points_m[1][1], 2.) &&
            close(negative.meshes[0].points_m[1][1], -2.),
            "Stellwinkel sign must follow local -Z convention");

    auto incompatible = fixture(false);
    incompatible.referenzen[0].rig_unterliek_richtung_modell = {0., 0., 1.};
    SegelAktionAnforderung automatic;
    automatic.aktion = SegelAktion::Automatisch;
    automatic.element_id = "FixtureSegel";
    check_status(pruefeSegelAktion(incompatible, automatic),
                 SegelErgebnisStatus::Fehler,
                 "Incompatible automatic directions must be rejected");

    auto invalid_request = automatic;
    invalid_request.aktion = static_cast<SegelAktion>(255);
    check_status(pruefeSegelAktion(fixture(false), invalid_request),
                 SegelErgebnisStatus::Fehler,
                 "Unknown action enum values must be rejected");
    invalid_request = {};
    invalid_request.aktion = SegelAktion::Rotation;
    invalid_request.element_id = "FixtureSegel";
    invalid_request.bewegte_ecke = SegelEcke::KopfAchterliek;
    check_status(pruefeSegelAktion(fixture(false), invalid_request),
                 SegelErgebnisStatus::Fehler,
                 "Rotation handles absent from a triangular sail must be rejected");
    invalid_request = {};
    invalid_request.aktion = SegelAktion::OriginalBuglage;
    invalid_request.element_id = "FixtureSegel";
    invalid_request.original_buglage = static_cast<SegelBuglage>(255);
    check_status(pruefeSegelAktion(fixture(false), invalid_request),
                 SegelErgebnisStatus::Fehler,
                 "Unknown original tack-side enum values must be rejected");

    auto reflected = fixture(true);
    reflected.grundmeshes[0].points_m = {
        {0., 0., 0.}, {-2., .4, 0.}, {0., .2, 3.}, {-.2, .5, 3.}};
    const auto original_points = reflected.grundmeshes[0].points_m;
    const auto original_triangle = reflected.grundmeshes[0].triangles[0];
    const auto original_normal = triangle_normal(original_points, original_triangle);
    const auto mirrored = stelleSegelStellwinkelVorschauDar(
        reflected, "FixtureSegel", -90.);
    check_status(mirrored, SegelErgebnisStatus::Vorschau,
                 "A different-tack angle must reflect the sail's camber geometry");
    const QuaternionXyzw mirror_turn{0., 0., std::sin(3.14159265358979323846 / 4.),
                                     std::cos(3.14159265358979323846 / 4.)};
    for (std::size_t i = 0; i < original_points.size(); ++i) {
        const Vektor3 reflected_point{
            original_points[i][0], -original_points[i][1], original_points[i][2]};
        const auto expected = rotate(mirror_turn, reflected_point);
        for (std::size_t axis = 0; axis < 3; ++axis)
            require(close(mirrored.meshes[0].points_m[i][axis], expected[axis]),
                    "Mirroring must reflect camber points before the local setting rotation");
    }
    const auto& mirrored_triangle = mirrored.meshes[0].triangles[0];
    require(mirrored_triangle[0] == original_triangle[0] &&
            mirrored_triangle[1] == original_triangle[2] &&
            mirrored_triangle[2] == original_triangle[1],
            "Mirrored warped geometry must reverse triangle winding");
    const auto rendered_normal = triangle_normal(mirrored.meshes[0].points_m,
                                                 mirrored_triangle);
    const Vektor3 reflected_normal{original_normal[0], -original_normal[1],
                                   original_normal[2]};
    const auto expected_normal = rotate(mirror_turn, reflected_normal);
    require(point_distance(rendered_normal, expected_normal) <= 1e-9,
            "Mirrored triangle winding must yield the correspondingly transformed face normal");
    const auto mirrored_again = stelleSegelStellwinkelVorschauDar(
        reflected, "FixtureSegel", -90.);
    require(mirrored_again.meshes[0].points_m == mirrored.meshes[0].points_m &&
            mirrored_again.meshes[0].triangles == mirrored.meshes[0].triangles &&
            reflected.grundmeshes[0].points_m == original_points,
            "Repeated mirrored previews must be idempotent and leave source profile untouched");
}

fs::path copy_model(const fs::path& source)
{
    const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto destination = fs::current_path() /
        ("segelkonfigurator-test-" + std::to_string(unique));
    for (const auto& entry : fs::recursive_directory_iterator(source)) {
        const auto relative = fs::relative(entry.path(), source);
        const auto filename = entry.path().filename().u8string();
        if (filename == "segelkonfiguration.json" || filename == "segelkonfiguration.lock" ||
            filename == "modells_information.lock")
            continue;
        const auto relative_text = relative.generic_u8string();
        const bool lookup_product = relative_text.find("LookUp/") != std::string::npos ||
                                    relative_text.find("LookUp\\") != std::string::npos;
        if (lookup_product && entry.is_regular_file() && filename != "lookup.json") continue;
        const auto target = destination / relative;
        if (entry.is_directory()) {
            fs::create_directories(target);
        } else if (entry.is_regular_file()) {
            fs::create_directories(target.parent_path());
            fs::copy_file(entry.path(), target, fs::copy_options::overwrite_existing);
        }
    }
    const auto model_path = destination / "modells_information.json";
    auto model = nlohmann::json::parse(read_binary_file(model_path));
    strip_settings(model);
    std::ofstream output(model_path, std::ios::binary | std::ios::trunc);
    output << model.dump(2) << '\n';
    return destination;
}

int write_from_worker(const fs::path& root, const std::string& revision)
{
    try {
        const auto catalog = modellkatalog(root);
        const auto* entry = catalog.eintrag("420");
        if (entry == nullptr) return 2;
        const auto inspection = readModelInspection(catalog, "420", entry->modell_fassung);
        const auto snapshot = leseSegelKonfiguration(catalog, *inspection);
        SegelAktionAnforderung request;
        request.aktion = SegelAktion::OriginalBuglage;
        request.element_id = "Großsegel";
        request.original_buglage = SegelBuglage::Backbord;
        const auto result = bestaetigeSegelAktion(
            catalog, *inspection, snapshot, request, revision);
        return result.status == SegelErgebnisStatus::Gespeichert ? 0 : 1;
    } catch (...) {
        return 2;
    }
}

#ifdef _WIN32
PROCESS_INFORMATION launch_writer(const fs::path& executable, const fs::path& root,
                                  const std::string& revision)
{
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    std::wstring command = L"\"" + executable.wstring() + L"\" --writer \"" +
        root.wstring() + L"\" " + std::to_wstring(std::stoull(revision));
    std::vector<wchar_t> mutable_command(command.begin(), command.end());
    mutable_command.push_back(L'\0');
    if (!CreateProcessW(executable.c_str(), mutable_command.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process))
        throw std::runtime_error("Unable to start concurrent writer process");
    return process;
}

DWORD finish_writer(PROCESS_INFORMATION& process)
{
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD exit_code = 2;
    GetExitCodeProcess(process.hProcess, &exit_code);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return exit_code;
}
#endif

void test_configuration(const fs::path& source_model, const fs::path& executable)
{
    const auto root = copy_model(source_model.parent_path());
    try {
        const auto catalog = modellkatalog(root);
        const auto* entry = catalog.eintrag("420");
        require(entry != nullptr && entry->status == "verwendbar",
                "Copied 420 model should be a valid catalogue entry");
        const auto inspection = readModelInspection(catalog, "420", entry->modell_fassung);
        const auto immutable_snapshot_id = inspection->info().snapshot_id;
        const auto immutable_meshes = getMesh(
            *inspection, "darstellung", std::vector<std::string>{"Großsegel", "Fock"});
        auto snapshot = leseSegelKonfiguration(catalog, *inspection);
        if (snapshot.status != SegelKonfigurationsStatus::NichtKonfiguriert) {
            std::string details = "Missing config must be a non-error unconfigured state (status " +
                std::to_string(static_cast<int>(snapshot.status)) + ")";
            for (const auto& diagnostic : snapshot.diagnosen)
                details += "\n" + diagnostic.code + ": " + diagnostic.nachricht +
                    " [" + diagnostic.element_id + "]";
            throw std::runtime_error(details);
        }
        require(snapshot.revision == "0", "Initial revision must be zero");
        auto initial_model = nlohmann::json::parse(read_binary_file(root / "modells_information.json"));
        initial_model["konfiguration"] = {
            {"schema_version", 1}, {"revision", 0}};
        for (auto& group : initial_model["gruppen"])
            for (auto& element : group["elemente"])
                element["setting"] = {
                    {"translation_m", {0., 0., 0.}},
                    {"rotation_xyzw", {0., 0., 0., 1.}},
                    {"konfiguriert", false}};
        {
            std::ofstream output(root / "modells_information.json", std::ios::binary | std::ios::trunc);
            output << initial_model.dump(2) << '\n';
        }
        snapshot = leseSegelKonfiguration(catalog, *inspection);
        require(snapshot.status == SegelKonfigurationsStatus::NichtKonfiguriert &&
                snapshot.revision == "0",
                "Unbound empty generic configuration must allow initial sail configuration");
        require(snapshot.referenzen.size() == 2, "Two explicit 420 sail references are expected");
        require(reference(snapshot, "Großsegel").intrinsische_quelle_id == "420_grosssegel",
                "Element and intrinsic sail IDs must remain separate");
        require(reference(snapshot, "Fock").intrinsische_quelle_id == "420_fock",
                "Fock mapping must be explicit");
        require(snapshot.grundmeshes.size() >= 2, "Original sail meshes must be available");

        const auto mast_vertices = read_obj_vertices_independently(
            root / "modells/visualisation/mast.obj");
        const auto boom_vertices = read_obj_vertices_independently(
            root / "modells/visualisation/grossbaum.obj");
        const auto stay_vertices = read_obj_vertices_independently(
            root / "modells/visualisation/vorstag.obj");
        const auto boom_cap_a = boom_vertices.at(boom_vertices.size() - 2);
        const auto boom_cap_b = boom_vertices.back();
        const auto boom_section_a = independent_section_center(boom_vertices, 0, 24);
        const auto boom_section_b = independent_section_center(boom_vertices, 24, 24);
        require(point_distance(boom_cap_a, boom_section_a) <= 1e-6 &&
                point_distance(boom_cap_b, boom_section_b) <= 1e-6,
                "OBJ boom cap centers must match their independently measured sections");
        const auto mast_lower = independent_section_center(mast_vertices, 0, 24);
        const auto mast_upper = independent_section_center(mast_vertices, 24, 24);
        const double boom_height = (boom_cap_a[2] + boom_cap_b[2]) / 2.;
        const double mast_fraction =
            (boom_height - mast_lower[2]) / (mast_upper[2] - mast_lower[2]);
        const auto obj_mast_attachment = add(
            mast_lower, scale(subtract(mast_upper, mast_lower), mast_fraction));
        const auto boom_near = point_distance(boom_cap_a, obj_mast_attachment) <
            point_distance(boom_cap_b, obj_mast_attachment) ? boom_cap_a : boom_cap_b;
        const auto boom_far = boom_near == boom_cap_a ? boom_cap_b : boom_cap_a;
        const auto obj_boom_axis = subtract(boom_far, boom_near);
        const Vektor3 neutral_boom_axis{
            -std::hypot(obj_boom_axis[0], obj_boom_axis[1]), 0., obj_boom_axis[2]};
        const auto obj_stay_deck = independent_section_center(stay_vertices, 0, 8);
        const auto obj_stay_next = independent_section_center(stay_vertices, 8, 8);
        const auto obj_stay_axis = subtract(obj_stay_next, obj_stay_deck);
        const auto& main_reference = reference(snapshot, "Großsegel");
        const auto& fock_reference = reference(snapshot, "Fock");
        require(point_distance(main_reference.rig_hals_anschlag_m,
                               obj_mast_attachment) <= 1e-6 &&
                angle_between(main_reference.rig_vorliek_richtung_modell,
                              subtract(mast_upper, mast_lower)) <= 1e-6 &&
                angle_between(main_reference.rig_unterliek_richtung_modell,
                              obj_boom_axis) <= 1e-6,
                "Main-sail rig metadata must align with independent mast/boom OBJ sections");
        require(point_distance(fock_reference.rig_hals_anschlag_m, obj_stay_deck) <= 1e-6 &&
                angle_between(fock_reference.rig_vorliek_richtung_modell,
                              obj_stay_axis) <= 1e-6,
                "Fock rig metadata must align with independent stay OBJ sections");

        const auto initial_geometry = leseKonfigurierteSegelgeometrie(snapshot);
        check_status(initial_geometry, SegelErgebnisStatus::Vorschau,
                     "Initial configured geometry should be readable");
        require(initial_geometry.meshes.size() == snapshot.grundmeshes.size(),
                "Read-only geometry must return all sail meshes");

        SegelAktionAnforderung main_auto;
        main_auto.aktion = SegelAktion::Automatisch;
        main_auto.element_id = "Großsegel";
        const auto main_preview = pruefeSegelAktion(snapshot, main_auto);
        check_status(main_preview, SegelErgebnisStatus::Vorschau,
                     "Main automatic preview should succeed");
        require(nlohmann::json::parse(read_binary_file(root / "modells_information.json"))["konfiguration"] ==
                    initial_model["konfiguration"],
                "Preview must not persist configuration");
        require(main_preview.lage.has_value(), "Preview must include resulting pose");
        const auto main_tack = transform_point(*main_preview.lage,
                                               reference(snapshot, "Großsegel").ecken.hals_m);
        require(point_distance(main_tack, obj_mast_attachment) <= 1e-6,
                "Main tack must reach the independently derived mast attachment");
        const auto main_luff = rotate(main_preview.lage->rotation_xyzw,
            reference(snapshot, "Großsegel").vorliek_richtung_lokal);
        const auto main_foot = rotate(main_preview.lage->rotation_xyzw,
            reference(snapshot, "Großsegel").unterliek_richtung_lokal);
        require(angle_between(main_luff, subtract(mast_upper, mast_lower)) <= 1e-6 &&
                angle_between(main_foot, neutral_boom_axis) <= 1e-6,
                "Main luff/foot must match mast and neutralized boom axis, not raw opening");
        const auto repeated_main = pruefeSegelAktion(snapshot, main_auto);
        check_status(repeated_main, SegelErgebnisStatus::Vorschau,
                     "Automatic main positioning must be repeatable");
        require(repeated_main.lage->rotation_xyzw == main_preview.lage->rotation_xyzw &&
                repeated_main.lage->translation_m == main_preview.lage->translation_m,
                "Automatic positioning must be idempotent");
        const auto main_committed = bestaetigeSegelAktion(
            catalog, *inspection, snapshot, main_auto, snapshot.revision);
        check_status(main_committed, SegelErgebnisStatus::Gespeichert,
                     "Main automatic commit should persist atomically");
        require(main_committed.revision == "1", "First commit must increment revision");
        auto partial_model = nlohmann::json::parse(read_binary_file(root / "modells_information.json"));
        require(model_element(partial_model, "Großsegel")["setting"]["konfiguriert"] == true &&
                model_element(partial_model, "Fock")["setting"]["konfiguriert"] == false &&
                !partial_model["konfiguration"].contains("elemente"),
                "First save must persist only the confirmed sail");
        const auto partial_reload = leseSegelKonfiguration(catalog, *inspection);
        require(partial_reload.status == SegelKonfigurationsStatus::Bereit &&
                lage(partial_reload, "Großsegel").konfiguriert &&
                !lage(partial_reload, "Fock").konfiguriert,
                "Partial configuration must preserve the confirmed sail and leave Fock initial");
        require(modell_fassung(root / "modells_information.json") == entry->modell_fassung,
                "Saving configuration must preserve base model fingerprint");
        auto with_extension = partial_model;
        auto& mast_setting = model_element(with_extension, "Mast")["setting"];
        mast_setting = {
            {"translation_m", {0., 0., 0.}}, {"rotation_xyzw", {0., 0., 0., 1.}},
            {"konfiguriert", true}, {"zukunft", {{"sensor", "erhalten"}}}};
        auto& main_setting = model_element(with_extension, "Großsegel")["setting"];
        main_setting["unbekannt"] = {{"format", 7}};
        main_setting["zukunft"] = "erhalten";
        with_extension["konfiguration"]["erweiterung"] = {{"version", 4}};
        {
            std::ofstream output(root / "modells_information.json", std::ios::binary | std::ios::trunc);
            output << with_extension.dump(2) << '\n';
        }
        const auto extension_reload = leseSegelKonfiguration(catalog, *inspection);
        require(extension_reload.status == SegelKonfigurationsStatus::Bereit,
                "Valid non-sail configuration and unknown fields must be accepted");
        snapshot.revision = main_committed.revision;
        replace_lage(snapshot, *main_committed.lage);

        auto fock_auto = main_auto;
        fock_auto.element_id = "Fock";
        const auto fock_committed = bestaetigeSegelAktion(
            catalog, *inspection, snapshot, fock_auto, snapshot.revision);
        check_status(fock_committed, SegelErgebnisStatus::Gespeichert,
                     "Fock automatic commit should persist");
        auto after_extension = nlohmann::json::parse(read_binary_file(root / "modells_information.json"));
        require(model_element(after_extension, "Mast")["setting"] == mast_setting &&
                model_element(after_extension, "Großsegel")["setting"] == main_setting &&
                after_extension["konfiguration"]["erweiterung"] ==
                    with_extension["konfiguration"]["erweiterung"],
                "Save must preserve unrelated entries, prior sail state and all unknown fields");
        auto before_base = with_extension;
        auto after_base = after_extension;
        strip_settings(before_base);
        strip_settings(after_base);
        require(before_base == after_base,
                "Saving must preserve all model content outside configuration");
        const auto fock_luff = rotate(fock_committed.lage->rotation_xyzw,
            reference(snapshot, "Fock").vorliek_richtung_lokal);
        const auto fock_foot = rotate(fock_committed.lage->rotation_xyzw,
            reference(snapshot, "Fock").unterliek_richtung_lokal);
        require(angle_between(fock_luff, obj_stay_axis) <= 1e-6 &&
                dot(fock_foot, Vektor3{-1., 0., 0.}) > 0. &&
                close(fock_committed.lage->fock_versatz_cm, 0., 1e-9),
                "Fock automatic position must align its stay and face aft with zero offset");
        const auto fock_tack = transform_point(*fock_committed.lage,
                                               fock_reference.ecken.hals_m);
        require(point_distance(fock_tack, obj_stay_deck) <= 1e-6,
                "Fock tack must reach the independently derived stay deck attachment");
        snapshot.revision = fock_committed.revision;
        replace_lage(snapshot, *fock_committed.lage);
        const auto persisted = leseSegelKonfiguration(catalog, *inspection);
        require(persisted.status == SegelKonfigurationsStatus::Bereit &&
                persisted.revision == snapshot.revision,
                "A fresh inspection must restore committed configuration");
        require(persisted.quellen_fingerprint == snapshot.quellen_fingerprint,
                "Configuration actions must not change source or lookup fingerprints");
        const auto restored_geometry = leseKonfigurierteSegelgeometrie(persisted);
        check_status(restored_geometry, SegelErgebnisStatus::Vorschau,
                     "Persisted geometry should render without mutating inspection");
        SegelAktionAnforderung foreign;
        foreign.aktion = SegelAktion::Automatisch;
        foreign.element_id = "Mast";
        check_status(pruefeSegelAktion(persisted, foreign), SegelErgebnisStatus::Fehler,
                     "A rig element must not be accepted as a sail");
        SegelAktionAnforderung invalid_number;
        invalid_number.aktion = SegelAktion::Translation;
        invalid_number.element_id = "Fock";
        invalid_number.translation_m = {std::numeric_limits<double>::quiet_NaN(), 0., 0.};
        check_status(pruefeSegelAktion(persisted, invalid_number), SegelErgebnisStatus::Fehler,
                     "Non-finite action values must be rejected");
        const std::array<double, 3> non_finite_values{
            std::numeric_limits<double>::quiet_NaN(),
            std::numeric_limits<double>::infinity(),
            -std::numeric_limits<double>::infinity()};
        for (const double value : non_finite_values) {
            invalid_number = {};
            invalid_number.aktion = SegelAktion::Rotation;
            invalid_number.element_id = "Fock";
            invalid_number.winkel_grad = value;
            check_status(pruefeSegelAktion(persisted, invalid_number),
                         SegelErgebnisStatus::Fehler,
                         "GUI rotation input must reject NaN and both infinities");
            invalid_number = {};
            invalid_number.aktion = SegelAktion::FockVersatzCm;
            invalid_number.element_id = "Fock";
            invalid_number.fock_versatz_cm = value;
            check_status(pruefeSegelAktion(persisted, invalid_number),
                         SegelErgebnisStatus::Fehler,
                         "GUI absolute Fock offset must reject NaN and both infinities");
            require(stelleSegelStellwinkelVorschauDar(
                        persisted, "Fock", value).status == SegelErgebnisStatus::Fehler,
                    "GUI setting-angle API must reject NaN and both infinities");
        }
        invalid_number = {};
        invalid_number.aktion = SegelAktion::Translation;
        invalid_number.element_id = "Fock";
        invalid_number.translation_m = {
            std::numeric_limits<double>::infinity(), 0., 0.};
        check_status(pruefeSegelAktion(persisted, invalid_number),
                     SegelErgebnisStatus::Fehler,
                     "GUI translation input must reject infinity as well as NaN");
        const auto rejected_commit = bestaetigeSegelAktion(
            catalog, *inspection, persisted, invalid_number, persisted.revision);
        check_status(rejected_commit, SegelErgebnisStatus::Fehler,
                     "Non-finite GUI input must not be committed");
        require(leseSegelKonfiguration(catalog, *inspection).revision == persisted.revision,
                "Rejected non-finite commit must leave the persisted revision unchanged");

        SegelAktionAnforderung offset;
        offset.aktion = SegelAktion::FockVersatzCm;
        offset.element_id = "Fock";
        offset.fock_versatz_cm = 12.5;
        const auto offset_first = pruefeSegelAktion(persisted, offset);
        check_status(offset_first, SegelErgebnisStatus::Vorschau,
                     "Absolute Fock offset should preview");
        const auto offset_repeat = pruefeSegelAktion(persisted, offset);
        check_status(offset_repeat, SegelErgebnisStatus::Vorschau,
                     "Repeated absolute Fock offset should preview");
        require(offset_first.lage && offset_repeat.lage &&
                close(offset_first.lage->translation_m[0], offset_repeat.lage->translation_m[0]) &&
                close(offset_first.lage->translation_m[2], offset_repeat.lage->translation_m[2]),
                "Absolute cm input must not accumulate");
        const auto& fock_ref = reference(persisted, "Fock");
        const auto fock_hals = transform_point(*offset_first.lage, fock_ref.ecken.hals_m);
        const auto d_length = std::sqrt(dot(fock_ref.fock_versatzrichtung_modell,
                                            fock_ref.fock_versatzrichtung_modell));
        const auto d = scale(fock_ref.fock_versatzrichtung_modell, 1. / d_length);
        require(close(dot(subtract(fock_hals, fock_ref.fock_grundhals_m), d), .125, 1e-9),
                "Absolute cm formula must use fixed deck reference and directed stay");
        const auto initial_fock_hals = transform_point(lage(persisted, "Fock"),
                                                       fock_ref.ecken.hals_m);
        const auto initial_projection = dot(
            subtract(initial_fock_hals, fock_ref.fock_grundhals_m), d);
        for (const double centimetres : {-10., 0., 10.}) {
            offset.fock_versatz_cm = centimetres;
            const auto signed_preview = pruefeSegelAktion(persisted, offset);
            check_status(signed_preview, SegelErgebnisStatus::Vorschau,
                         "Signed absolute cm value should preview");
            const auto signed_hals = transform_point(*signed_preview.lage,
                                                      fock_ref.ecken.hals_m);
            const auto projection = dot(
                subtract(signed_hals, fock_ref.fock_grundhals_m), d);
            require(close(projection, centimetres / 100., 1e-9),
                    "Absolute cm values must support zero and either sign");
            const auto correction = subtract(signed_hals, initial_fock_hals);
            const auto perpendicular = subtract(correction,
                scale(d, dot(correction, d)));
            require(std::sqrt(dot(perpendicular, perpendicular)) < 1e-9,
                    "Fock cm operation must preserve perpendicular manual correction");
            require(close(dot(subtract(signed_hals, fock_ref.fock_grundhals_m), d) -
                              initial_projection,
                          centimetres / 100. - initial_projection, 1e-9),
                    "Absolute cm offset must be based on fixed reference, not accumulated");
        }
        SegelAktionAnforderung side_correction;
        side_correction.aktion = SegelAktion::Translation;
        side_correction.element_id = "Fock";
        side_correction.ebene = SegelEbene::XY;
        side_correction.translation_m = {0., .1, 0.};
        const auto corrected = pruefeSegelAktion(persisted, side_correction);
        check_status(corrected, SegelErgebnisStatus::Vorschau,
                     "Manual perpendicular Fock correction should preview");
        auto corrected_snapshot = persisted;
        replace_lage(corrected_snapshot, *corrected.lage);
        offset.fock_versatz_cm = -10.;
        const auto corrected_offset = pruefeSegelAktion(corrected_snapshot, offset);
        check_status(corrected_offset, SegelErgebnisStatus::Vorschau,
                     "Absolute offset should apply to manually corrected Fock");
        require(close(corrected_offset.lage->translation_m[1],
                      corrected.lage->translation_m[1], 1e-9),
                "Fock cm input must retain perpendicular manual translation");

        SegelAktionAnforderung invalid_plane;
        invalid_plane.aktion = SegelAktion::Translation;
        invalid_plane.element_id = "Fock";
        invalid_plane.ebene = SegelEbene::XY;
        invalid_plane.translation_m = {0., 0., .001};
        check_status(pruefeSegelAktion(persisted, invalid_plane),
                     SegelErgebnisStatus::Fehler,
                     "Translation outside selected model plane must fail");

        SegelAktionAnforderung rotation;
        rotation.aktion = SegelAktion::Rotation;
        rotation.element_id = "Fock";
        rotation.ebene = SegelEbene::XY;
        rotation.bewegte_ecke = SegelEcke::Schothorn;
        rotation.winkel_grad = 23.;
        const auto rotated = pruefeSegelAktion(persisted, rotation);
        check_status(rotated, SegelErgebnisStatus::Vorschau,
                     "Rigid corner rotation should preview");
        require(rotated.lage.has_value(), "Rotation preview must include pose");
        const auto old_tack = transform_point(lage(persisted, "Fock"),
                                              fock_ref.ecken.hals_m);
        const auto new_tack = transform_point(*rotated.lage, fock_ref.ecken.hals_m);
        require(close(old_tack[0], new_tack[0]) && close(old_tack[1], new_tack[1]) &&
                close(old_tack[2], new_tack[2]),
                "Rotation with a non-tack handle must keep the tack pivot fixed");

        SegelAktionAnforderung set_original;
        set_original.aktion = SegelAktion::OriginalBuglage;
        set_original.element_id = "Großsegel";
        set_original.original_buglage = SegelBuglage::Backbord;
        const auto side_saved = bestaetigeSegelAktion(
            catalog, *inspection, persisted, set_original, persisted.revision);
        check_status(side_saved, SegelErgebnisStatus::Gespeichert,
                     "Original bug side must be saved transactionally");
        const auto before_conflict = leseSegelKonfiguration(catalog, *inspection);
#ifdef _WIN32
        const auto config_file_path = root / "modells_information.json";
        const auto bytes_before_write_fault = read_binary_file(config_file_path);
        const auto blocked_temporary = root / "modells_information.json.tmp.0";
        fs::create_directory(blocked_temporary);
        auto prewrite_failure = launch_writer(
            executable, root, before_conflict.revision);
        require(finish_writer(prewrite_failure) == 1,
                "A blocked temporary path must inject a pre-write failure");
        fs::remove(blocked_temporary);
        require(read_binary_file(config_file_path) == bytes_before_write_fault &&
                leseSegelKonfiguration(catalog, *inspection).revision ==
                    before_conflict.revision &&
                !has_temporary_configuration_file(root),
                "Pre-write failure must preserve exact prior bytes/revision and leave no temp");

        const auto held_config = CreateFileW(
            config_file_path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        require(held_config != INVALID_HANDLE_VALUE,
                "Test must open the config without delete-sharing for atomic-replace fault");
        const auto replace_failure = bestaetigeSegelAktion(
            catalog, *inspection, before_conflict, set_original, before_conflict.revision);
        CloseHandle(held_config);
        check_status(replace_failure, SegelErgebnisStatus::Fehler,
                     "A target held without delete-sharing must abort atomic replacement");
        require(read_binary_file(config_file_path) == bytes_before_write_fault &&
                leseSegelKonfiguration(catalog, *inspection).revision ==
                    before_conflict.revision &&
                !has_temporary_configuration_file(root),
                "Failed atomic replacement must retain exact prior bytes/revision and clean temp");

        auto process1 = launch_writer(executable, root, before_conflict.revision);
        auto process2 = launch_writer(executable, root, before_conflict.revision);
        const auto writer1 = finish_writer(process1);
        const auto writer2 = finish_writer(process2);
        require((writer1 == 0 && writer2 == 1) || (writer1 == 1 && writer2 == 0),
                "Separate writer processes must serialize and reject one stale revision");
        const auto lock_baseline = leseSegelKonfiguration(catalog, *inspection);
        const auto lock_handle = CreateFileW(
            (root / "modells_information.lock").c_str(), GENERIC_READ | GENERIC_WRITE, 0,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        require(lock_handle != INVALID_HANDLE_VALUE,
                "Test must be able to hold the model configuration lock");
        const auto denied_write = bestaetigeSegelAktion(
            catalog, *inspection, lock_baseline, set_original, lock_baseline.revision);
        CloseHandle(lock_handle);
        check_status(denied_write, SegelErgebnisStatus::Fehler,
                     "A held process lock must refuse a configuration write");
        const auto after_denied_write = leseSegelKonfiguration(catalog, *inspection);
        require(after_denied_write.revision == lock_baseline.revision,
                "Lock failure must preserve last confirmed configuration");
#else
        std::atomic<int> ready{0};
        std::atomic<bool> start{false};
        std::array<SegelKonfigurationErgebnis, 2> concurrent_results;
        std::array<std::thread, 2> writers{
            std::thread([&] {
                ++ready;
                while (!start.load()) std::this_thread::yield();
                concurrent_results[0] = bestaetigeSegelAktion(
                    catalog, *inspection, before_conflict, set_original,
                    before_conflict.revision);
            }),
            std::thread([&] {
                ++ready;
                while (!start.load()) std::this_thread::yield();
                concurrent_results[1] = bestaetigeSegelAktion(
                    catalog, *inspection, before_conflict, set_original,
                    before_conflict.revision);
            })};
        while (ready.load() != 2) std::this_thread::yield();
        start = true;
        for (auto& writer : writers) writer.join();
        const auto saves = std::count_if(concurrent_results.begin(), concurrent_results.end(),
            [](const SegelKonfigurationErgebnis& result) {
                return result.status == SegelErgebnisStatus::Gespeichert;
            });
        const auto conflicts = std::count_if(concurrent_results.begin(), concurrent_results.end(),
            [](const SegelKonfigurationErgebnis& result) {
                return result.status == SegelErgebnisStatus::Fehler;
            });
        require(saves == 1 && conflicts == 1,
                "Concurrent writers must serialize and reject one stale revision");
#endif

        const auto reread = leseSegelKonfiguration(catalog, *inspection);
        require(reread.status == SegelKonfigurationsStatus::Bereit &&
                lage(reread, "Großsegel").original_buglage == SegelBuglage::Backbord,
                "Original side must survive reload");
        const auto positive = stelleSegelStellwinkelVorschauDar(reread, "Großsegel", 90.);
        const auto negative = stelleSegelStellwinkelVorschauDar(reread, "Großsegel", -90.);
        check_status(positive, SegelErgebnisStatus::Vorschau,
                     "Positive backbord setting should render");
        check_status(negative, SegelErgebnisStatus::Vorschau,
                     "Opposite tack should render by local mirroring");
        require(!positive.meshes.empty() && !negative.meshes.empty(),
                "Both original and mirrored sail representations should contain meshes");
        require(positive.meshes.front().triangles.size() ==
                    negative.meshes.front().triangles.size(),
                "Mirroring must retain contour topology");
        if (!positive.meshes.front().triangles.empty()) {
            const auto& a = positive.meshes.front().triangles.front();
            const auto& b = negative.meshes.front().triangles.front();
            require(a[0] == b[0] && a[1] == b[2] && a[2] == b[1],
                    "Mirroring must reverse triangle orientation");
        }

        auto unknown_snapshot = reread;
        for (auto& value : unknown_snapshot.lagen)
            if (value.element_id == "Fock") value.original_buglage = SegelBuglage::Unbekannt;
        const auto unknown_side = stelleSegelStellwinkelVorschauDar(
            unknown_snapshot, "Fock", 10.);
        check_status(unknown_side, SegelErgebnisStatus::Fehler,
                     "Nonzero setting with unknown original side must diagnose");
        const auto zero_side = stelleSegelStellwinkelVorschauDar(
            unknown_snapshot, "Fock", 0.);
        check_status(zero_side, SegelErgebnisStatus::Vorschau,
                     "Zero setting must preserve unknown original geometry");

        const auto config_path = root / "modells_information.json";
        const auto saved_model_bytes = read_binary_file(config_path);
        auto migrated_model = nlohmann::json::parse(saved_model_bytes);
        migrated_model["konfiguration"]["revision"] = 12;
        {
            std::ofstream output(config_path, std::ios::binary | std::ios::trunc);
            output << migrated_model.dump(2);
        }
        const auto migrated_reload = leseSegelKonfiguration(catalog, *inspection);
        require(migrated_reload.status == SegelKonfigurationsStatus::Bereit &&
                migrated_reload.revision == "12" &&
                lage(migrated_reload, "Fock").rotation_xyzw == lage(reread, "Fock").rotation_xyzw &&
                lage(migrated_reload, "Fock").translation_m == lage(reread, "Fock").translation_m &&
                lage(migrated_reload, "Fock").original_buglage ==
                    lage(reread, "Fock").original_buglage,
                "Migrated revision12 must retain confirmed poses and original side");
        const auto restart_catalog = modellkatalog(root);
        const auto* restart_entry = restart_catalog.eintrag("420");
        require(restart_entry != nullptr && restart_entry->status == "verwendbar",
                "Saved embedded configuration must remain a usable model after restart");
        const auto restart_inspection = readModelInspection(
            restart_catalog, "420", restart_entry->modell_fassung);
        const auto restart_config = leseSegelKonfiguration(restart_catalog, *restart_inspection);
        require(restart_config.status == SegelKonfigurationsStatus::Bereit &&
                restart_config.revision == "12",
                "Fresh physics import and inspection must recover confirmed revision12");
        require(model_element(migrated_model, "Großsegel")["setting"]["unbekannt"] ==
                    main_setting["unbekannt"] &&
                !model_element(migrated_model, "Großsegel")["setting"].contains("intrinsische_quelle_id") &&
                !model_element(migrated_model, "Fock")["setting"].contains("fock_versatz_cm"),
                "Repeated saves of affected sail must retain unknown settings and omit redundant values");
        {
            std::ofstream output(config_path, std::ios::binary | std::ios::trunc);
            output << saved_model_bytes;
        }
        auto invalid_non_sail = nlohmann::json::parse(saved_model_bytes);
        model_element(invalid_non_sail, "Mast")["setting"]["rotation_xyzw"] = {0., 0., 0., 0.};
        {
            std::ofstream output(config_path, std::ios::binary | std::ios::trunc);
            output << invalid_non_sail.dump(2);
        }
        require(leseSegelKonfiguration(catalog, *inspection).status ==
                    SegelKonfigurationsStatus::Fehler,
                "Invalid non-sail pose must be diagnosed, not ignored");
        {
            std::ofstream output(config_path, std::ios::binary | std::ios::trunc);
            output << saved_model_bytes;
        }
        for (const auto& source : {
                "modells/aerodynamics/main/main.json",
                "modells/aerodynamics/main/LookUp/lookup.json",
                "modells/visualisation/mast.obj"}) {
            const auto source_path = root / fs::u8path(source);
            const auto source_bytes = read_binary_file(source_path);
            {
                std::ofstream output(source_path, std::ios::binary | std::ios::app);
                output << '\n';
            }
            const auto rejected = bestaetigeSegelAktion(
                catalog, *inspection, reread, set_original, reread.revision);
            check_status(rejected, SegelErgebnisStatus::Fehler,
                         "Changed intrinsic, lookup or rig source must reject stale save");
            require(read_binary_file(config_path) == saved_model_bytes &&
                    leseSegelKonfiguration(catalog, *inspection).status ==
                        SegelKonfigurationsStatus::Fehler,
                    "Source edits must invalidate config without overwriting confirmed model");
            {
                std::ofstream output(source_path, std::ios::binary | std::ios::trunc);
                output << source_bytes;
            }
            require(leseSegelKonfiguration(catalog, *inspection).status ==
                        SegelKonfigurationsStatus::Bereit,
                    "Restoring exact source bytes must restore configuration validity");
        }
        {
            std::ofstream legacy_sidecar(root / "segelkonfiguration.json", std::ios::binary);
            legacy_sidecar << "{}";
        }
        require(leseSegelKonfiguration(catalog, *inspection).status ==
                    SegelKonfigurationsStatus::Fehler,
                "Simultaneous legacy sidecar and embedded configuration must be rejected");
        fs::remove(root / "segelkonfiguration.json");
        auto stale_base_model = nlohmann::json::parse(saved_model_bytes);
        stale_base_model["unbekannte_modellerweiterung"] = "geaendert";
        {
            std::ofstream output(config_path, std::ios::binary | std::ios::trunc);
            output << stale_base_model.dump(2);
        }
        const auto changed_base_bytes = read_binary_file(config_path);
        const auto stale_base_save = bestaetigeSegelAktion(
            catalog, *inspection, reread, set_original, reread.revision);
        check_status(stale_base_save, SegelErgebnisStatus::Fehler,
                     "Unknown model-content changes must reject save against stale inspection");
        require(read_binary_file(config_path) == changed_base_bytes,
                "Stale-base rejection must preserve all external model edits");
        {
            std::ofstream output(config_path, std::ios::binary | std::ios::trunc);
            output << saved_model_bytes;
        }
        std::ifstream config_input(config_path, std::ios::binary);
        nlohmann::json legacy_config;
        config_input >> legacy_config;
        model_element(legacy_config, "Großsegel")["setting"].erase("original_buglage");
        {
            std::ofstream config_output(config_path, std::ios::binary | std::ios::trunc);
            config_output << legacy_config.dump(2) << '\n';
        }
        const auto legacy_reload = leseSegelKonfiguration(catalog, *inspection);
        require(legacy_reload.status == SegelKonfigurationsStatus::Bereit &&
                lage(legacy_reload, "Großsegel").original_buglage == SegelBuglage::Unbekannt,
                "A prior configuration without original side must preserve pose and report unknown");
        require(close(lage(legacy_reload, "Großsegel").translation_m[0],
                      lage(reread, "Großsegel").translation_m[0]),
                "Missing optional original side must not alter stored position");

        {
            std::ofstream modified_geometry(
                root / "modells/visualisation/grosssegel.obj",
                std::ios::binary | std::ios::app);
            modified_geometry << "\n# source revision conflict\n";
        }
        const auto config_before_source_conflict = read_binary_file(config_path);
        const auto source_conflict_commit = bestaetigeSegelAktion(
            catalog, *inspection, legacy_reload, set_original, legacy_reload.revision);
        check_status(source_conflict_commit, SegelErgebnisStatus::Fehler,
                     "A geometry/source fingerprint conflict must refuse stale commit");
        require(!source_conflict_commit.diagnosen.empty() &&
                source_conflict_commit.diagnosen.front().code == "CORE-SEG-0016" &&
                read_binary_file(config_path) == config_before_source_conflict,
                "Source conflict must diagnose stale geometry and preserve exact config bytes");
        const auto stale_source = leseSegelKonfiguration(catalog, *inspection);
        require(stale_source.status == SegelKonfigurationsStatus::Fehler,
                "Changed geometry-source fingerprints must reject a stale configuration");
        {
            std::ofstream damaged_config(config_path, std::ios::binary | std::ios::trunc);
            legacy_config["konfiguration"] = "invalid";
            damaged_config << legacy_config.dump(2);
        }
        const auto damaged = leseSegelKonfiguration(catalog, *inspection);
        require(damaged.status == SegelKonfigurationsStatus::Fehler &&
                !damaged.diagnosen.empty(),
                "A malformed stored configuration must return a typed diagnostic");

        require(immutable_snapshot_id == inspection->info().snapshot_id,
                "Configuration must not mutate the immutable inspection snapshot");
        const auto meshes_after = getMesh(
            *inspection, "darstellung", std::vector<std::string>{"Großsegel", "Fock"});
        require(meshes_after.size() == immutable_meshes.size(),
                "Inspection geometry must remain unchanged after configuration writes");
        for (std::size_t i = 0; i < meshes_after.size(); ++i)
            require(meshes_after[i].points_m == immutable_meshes[i].points_m &&
                    meshes_after[i].triangles == immutable_meshes[i].triangles,
                    "Inspection mesh contents must remain byte-for-byte equivalent in memory");
    } catch (...) {
        std::error_code ignored;
        fs::remove_all(root, ignored);
        throw;
    }
    std::error_code ignored;
    fs::remove_all(root, ignored);
}

void test_empty_model_configurations(const fs::path& source_model)
{
    std::size_t checked = 0;
    for (const auto& entry : fs::directory_iterator(source_model.parent_path().parent_path())) {
        if (!entry.is_directory() || entry.path() == source_model.parent_path() ||
            !fs::exists(entry.path() / "modells_information.json")) continue;
        const auto root = copy_model(entry.path());
        struct Cleanup {
            fs::path root;
            ~Cleanup() {
                std::error_code error;
                fs::remove_all(root, error);
            }
        } cleanup{root};
        const auto path = root / "modells_information.json";
        const auto fingerprint = modell_fassung(path);
        auto model = nlohmann::json::parse(read_binary_file(path));
        const auto id = model.at("id").get<std::string>();
        model["konfiguration"] = {
            {"schema_version", 1}, {"revision", 0}};
        for (auto& group : model["gruppen"])
            for (auto& element : group["elemente"])
                element["setting"] = {
                    {"translation_m", {0., 0., 0.}},
                    {"rotation_xyzw", {0., 0., 0., 1.}},
                    {"konfiguriert", false}};
        {
            std::ofstream output(path, std::ios::binary | std::ios::trunc);
            output << model.dump(2) << '\n';
        }
        require(modell_fassung(path) == fingerprint,
                "Empty generic configuration must preserve every non-420 model fingerprint");
        const auto catalog = modellkatalog(root);
        const auto* item = catalog.eintrag(id);
        require(item != nullptr && item->status == "verwendbar" &&
                item->modell_fassung == fingerprint,
                "Models without configured sails must remain discoverable");
        const auto inspection = readModelInspection(catalog, id, fingerprint);
        require(inspection->info().model_id == id &&
                inspection->info().model_version == fingerprint,
                "Empty generic configuration must not block non-420 inspection");
        if (id == "quader") {
            const auto original_physics = lade_modell(path);
            const auto original_meshes = getMesh(*inspection, "darstellung");
            auto& setting = model_element(model, "Rumpf")["setting"];
            const double half_angle = std::sqrt(0.5);
            setting = {{"translation_m", {1., 2., 3.}},
                       {"rotation_xyzw", {0., 0., half_angle, half_angle}},
                       {"konfiguriert", true}, {"zukunft", "erhalten"}};
            {
                std::ofstream output(path, std::ios::binary | std::ios::trunc);
                output << model.dump(2) << '\n';
            }
            require(modell_fassung(path) == fingerprint,
                    "Generic mounting pose must preserve base fingerprint");
            const auto mounted_catalog = modellkatalog(root);
            const auto mounted_inspection = readModelInspection(mounted_catalog, id, fingerprint);
            const auto mounted_meshes = getMesh(*mounted_inspection, "darstellung");
            require(mounted_meshes.size() == original_meshes.size() && !mounted_meshes.empty(),
                    "Generic mounting must retain display meshes");
            for (std::size_t i = 0; i < mounted_meshes.size(); ++i)
                for (std::size_t j = 0; j < mounted_meshes[i].points_m.size(); ++j) {
                    const auto& before = original_meshes[i].points_m[j];
                    const Vektor3 expected{1. - before[1], 2. + before[0], 3. + before[2]};
                    require(point_distance(mounted_meshes[i].points_m[j], expected) < 1e-9,
                            "Generic translation and rotation must apply exactly once to display");
                }
            require(getMesh(*inspection, "darstellung").front().points_m ==
                        original_meshes.front().points_m,
                    "Mounting settings must not mutate previous inspection snapshot");
            const auto mounted_physics = lade_modell(path);
            require(original_physics.schwerpunkt_m == mounted_physics.schwerpunkt_m &&
                    original_physics.traegheit_kg_m2 == mounted_physics.traegheit_kg_m2,
                    "Display mounting must not silently move physical mass or inertia");
        }
        ++checked;
    }
    require(checked >= 9, "Expected at least nine reference models without 420 sail configuration");
}

void write_model_document(const fs::path& path, const nlohmann::json& model)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output << model.dump(2) << '\n';
    if (!output) throw std::runtime_error("Cannot write explicit mounting migration");
}

nlohmann::json obj_mounting(const std::vector<unsigned>& tack,
                            const std::vector<unsigned>& clew,
                            const std::vector<unsigned>& head,
                            const std::vector<unsigned>& aft = {})
{
    nlohmann::json result{
        {"schema_version", 1}, {"basis", "mittschiffs"}, {"bezug", "obj"},
        {"hals_vertex_indices", tack}, {"schothorn_vertex_indices", clew},
        {"kopf_vorliek_vertex_indices", head}};
    if (!aft.empty()) result["kopf_achterliek_vertex_indices"] = aft;
    return result;
}

Vektor3 indexed_center(const std::vector<Vektor3>& vertices,
                       const nlohmann::json& indices)
{
    Vektor3 result{};
    for (const auto& index : indices) result = add(result, vertices.at(index.get<unsigned>() - 1));
    return scale(result, 1. / static_cast<double>(indices.size()));
}

void normalize_boat_bases(const fs::path& models)
{
    for (const auto& id : {"420", "folkeboot", "optimist"}) {
        auto model = nlohmann::json::parse(read_binary_file(models / id / "modells_information.json"));
        require(!model_element(model, "Großbaum").contains("montage"),
                "Neutral-base migration already applied; refusing duplicate revision updates");
    }
    for (const auto& id : {"420", "folkeboot", "optimist"}) {
        const auto root = models / id;
        const auto path = root / "modells_information.json";
        auto model = nlohmann::json::parse(read_binary_file(path));
        auto& boom = model_element(model, "Großbaum");
        auto vertices = read_obj_vertices_independently(
            root / boom["darstellung"]["geometrie"].get<std::string>());
        nlohmann::json near_indices, far_indices;
        if (std::string(id) == "420") {
            near_indices = {50};
            far_indices = {49};
        } else if (std::string(id) == "folkeboot") {
            near_indices = {2, 3, 6, 7};
            far_indices = {1, 4, 5, 8};
            auto& main = model_element(model, "Großsegel");
            main["montage"] = obj_mounting({1, 5}, {2, 6}, {4, 8}, {3, 7});
            auto& fock = model_element(model, "Fock");
            fock["montage"] = obj_mounting({2, 5}, {1, 4}, {3, 6});
        } else {
            for (unsigned i = 1; i <= 64; i += 2) near_indices.push_back(i);
            for (unsigned i = 2; i <= 64; i += 2) far_indices.push_back(i);
            auto& main = model_element(model, "Großsegel");
            main["montage"] = obj_mounting({3, 21}, {1, 19}, {6, 24}, {2, 4, 20, 22});
        }
        boom["montage"] = {
            {"schema_version", 1}, {"basis", "mittschiffs"}, {"bezug", "obj"},
            {"anschlag_vertex_indices", near_indices}, {"ende_vertex_indices", far_indices}};
        const auto near_point = indexed_center(vertices, near_indices);
        const auto far_point = indexed_center(vertices, far_indices);
        const auto direction = subtract(far_point, near_point);
        const double angle = std::atan2(direction[1], -direction[0]);
        const QuaternionXyzw rotation{0., 0., std::sin(angle / 2.), std::cos(angle / 2.)};
        auto target = near_point;
        target[1] = 0.;
        const auto translation = subtract(target, rotate(rotation, near_point));
        auto& setting = boom["setting"];
        setting["rotation_xyzw"] = rotation;
        setting["translation_m"] = translation;
        setting["konfiguriert"] = true;
        model["konfiguration"]["revision"] =
            model["konfiguration"].value("revision", std::uint64_t{}) + 1;
        write_model_document(path, model);
        model["konfiguration"]["modell_id"] = id;
        model["konfiguration"]["modell_fassung"] = modell_fassung(path);
        write_model_document(path, model);
        auto catalog = modellkatalog(root);
        auto inspection = readModelInspection(catalog, id, catalog.eintrag(id)->modell_fassung);
        // Existing 420 fingerprints remain valid; other boats are first-time OBJ montage.
        if (std::string(id) != "420") {
            model["konfiguration"] = {{"schema_version", 1}, {"revision", 0}};
            write_model_document(path, model);
            catalog = modellkatalog(root);
            inspection = readModelInspection(catalog, id, catalog.eintrag(id)->modell_fassung);
        }
        auto snapshot = leseSegelKonfiguration(catalog, *inspection);
        require(snapshot.status != SegelKonfigurationsStatus::Fehler,
                "Explicit mounting migration must start from valid source snapshot");
        std::vector<std::string> sail_ids;
        for (const auto& sail : snapshot.referenzen) sail_ids.push_back(sail.element_id);
        for (const auto& sail_id : sail_ids) {
            SegelAktionAnforderung action;
            action.element_id = sail_id;
            const auto result = bestaetigeSegelAktion(
                catalog, *inspection, snapshot, action, snapshot.revision);
            check_status(result, SegelErgebnisStatus::Gespeichert, "Neutral mounting migration");
            snapshot = leseSegelKonfiguration(catalog, *inspection);
        }
        if (std::string(id) == "optimist") {
            model = nlohmann::json::parse(read_binary_file(path));
            const auto& main_setting = model_element(model, "Großsegel")["setting"];
            auto& spriet = model_element(model, "Spriet");
            spriet["setting"]["rotation_xyzw"] = main_setting["rotation_xyzw"];
            spriet["setting"]["translation_m"] = main_setting["translation_m"];
            spriet["setting"]["konfiguriert"] = true;
            spriet["montage"] = {{"schema_version", 1}, {"basis", "mittschiffs"},
                                 {"bezug", "obj"}, {"mitbewegt_mit", "Großsegel"}};
            model["konfiguration"]["revision"] = snapshot.revision.empty() ? 1 :
                std::stoull(snapshot.revision) + 1;
            write_model_document(path, model);
            model["konfiguration"]["modell_fassung"] = modell_fassung(path);
            write_model_document(path, model);
        }
        std::cout << id << " neutral base migrated\n";
    }
}

void test_zero_boat_bases(const fs::path& models)
{
    const auto catalog = modellkatalog(models);
    require(catalog.eintraege().size() == 11, "All eleven active models must load");
    for (const auto& entry : catalog.eintraege())
        require(entry.status == "verwendbar", "Neutral mounting must preserve model usability: " + entry.modell_id);
    for (const auto& id : {"420", "folkeboot", "optimist"}) {
        const auto& entry = *catalog.eintrag(id);
        const auto inspection = readModelInspection(catalog, id, entry.modell_fassung);
        auto snapshot = leseSegelKonfiguration(catalog, *inspection);
        if (snapshot.status != SegelKonfigurationsStatus::Bereit)
            for (const auto& diagnostic : snapshot.diagnosen)
                std::cerr << entry.modell_id << ": " << diagnostic.code << ": "
                          << diagnostic.nachricht << '\n';
        require(snapshot.status == SegelKonfigurationsStatus::Bereit, "Neutral sail snapshot must be ready");
        if (std::string(id) != "420") {
            require(std::any_of(snapshot.diagnosen.begin(), snapshot.diagnosen.end(),
                               [](const SegelDiagnose& diagnostic) {
                                   return diagnostic.code == "CORE-SEG-0030";
                               }),
                    "Ready OBJ mounting must retain intrinsic source incompatibility diagnosis");
            std::cout << id << ": CORE-SEG-0030 retained; mounting ready, no physics release\n";
        }
        const auto root = models / id;
        auto model = nlohmann::json::parse(read_binary_file(root / "modells_information.json"));
        const auto& boom = model_element(model, "Großbaum");
        auto boom_points = read_obj_vertices_independently(
            root / boom["darstellung"]["geometrie"].get<std::string>());
        SegelLage boom_pose;
        boom_pose.rotation_xyzw = boom["setting"]["rotation_xyzw"].get<QuaternionXyzw>();
        boom_pose.translation_m = boom["setting"]["translation_m"].get<Vektor3>();
        const auto before_near = indexed_center(boom_points, boom["montage"]["anschlag_vertex_indices"]);
        const auto before_far = indexed_center(boom_points, boom["montage"]["ende_vertex_indices"]);
        const auto near_point = transform_point(boom_pose, before_near);
        const auto far_point = transform_point(boom_pose, before_far);
        require(std::abs(near_point[1]) < 1e-8 && std::abs(far_point[1] - near_point[1]) < 1e-8 &&
                close(near_point[2], before_near[2], 1e-8) && close(far_point[2], before_far[2], 1e-8),
                "Neutral boom must open zero degrees and preserve attachment heights");
        for (const auto& ref : snapshot.referenzen) {
            const auto& pose = lage(snapshot, ref.element_id);
            const auto tack = transform_point(pose, ref.ecken.hals_m);
            const auto clew = transform_point(pose, ref.ecken.schothorn_m);
            const auto head = transform_point(pose, ref.ecken.kopf_vorliek_m);
            require(std::abs(tack[1]) < 1e-7 && std::abs(clew[1] - tack[1]) < 1e-7 &&
                    std::abs(head[1] - tack[1]) < 1e-7,
                    "Neutral sail semantic reference plane must be amidships");
            if (ref.art == SegelArt::Grosssegel)
                require(point_distance(tack, near_point) < 0.06,
                        "Neutral main sail must retain its measured boom attachment offset");
            SegelAktionAnforderung action;
            action.element_id = ref.element_id;
            const auto automatic = pruefeSegelAktion(snapshot, action);
            check_status(automatic, SegelErgebnisStatus::Vorschau, "Neutral automatic action");
            const auto automatic_tack = point_distance(
                transform_point(*automatic.lage, ref.ecken.hals_m), tack);
            const auto automatic_clew = point_distance(
                transform_point(*automatic.lage, ref.ecken.schothorn_m), clew);
            require(automatic_tack < 1e-7 && automatic_clew < 1e-7,
                    "Automatik must reproduce neutral base for " + ref.element_id +
                    " (tack error " + std::to_string(automatic_tack) +
                    " m, clew error " + std::to_string(automatic_clew) + " m)");
            auto sides = snapshot;
            for (auto& sail : sides.lagen) sail.original_buglage = SegelBuglage::Backbord;
            const auto zero = stelleSegelStellwinkelVorschauDar(sides, ref.element_id, 0.);
            check_status(zero, SegelErgebnisStatus::Vorschau, "Operating delta0 neutral basis");
            const auto positive = stelleSegelStellwinkelVorschauDar(sides, ref.element_id, 10.);
            const auto negative = stelleSegelStellwinkelVorschauDar(sides, ref.element_id, -10.);
            check_status(positive, SegelErgebnisStatus::Vorschau, "Positive operating angle");
            check_status(negative, SegelErgebnisStatus::Vorschau, "Negative operating angle");
            require(!zero.meshes.empty() && !zero.meshes.front().points_m.empty(),
                    "Neutral operating preview must include geometry");
            const auto& zero_points = zero.meshes.front().points_m;
            const auto luff_axis = unit(subtract(head, tack));
            const auto off_axis = [&](const Vektor3& p) {
                const auto relative = subtract(p, tack);
                return point_distance(relative, scale(luff_axis, dot(relative, luff_axis)));
            };
            const auto aft = std::max_element(zero_points.begin(), zero_points.end(),
                [&](const Vektor3& a, const Vektor3& b) { return off_axis(a) < off_axis(b); });
            const auto point_index = static_cast<std::size_t>(aft - zero_points.begin());
            require(positive.meshes.front().points_m[point_index][1] > (*aft)[1] &&
                    negative.meshes.front().points_m[point_index][1] < (*aft)[1],
                    "Operating signs must open toward backbord/steuerbord from neutral basis");
            if (std::string(id) != "420") {
                const auto before_axis = ref.legacy_montage ?
                    unit(ref.rig_vorliek_richtung_modell) :
                    unit(subtract(ref.ecken.kopf_vorliek_m, ref.ecken.hals_m));
                const auto after_axis = unit(subtract(head, tack));
                require(close(before_axis[2], after_axis[2], 1e-9) &&
                        close(point_distance(ref.ecken.schothorn_m, ref.ecken.hals_m),
                              point_distance(clew, tack), 1e-9),
                        "OBJ neutralization must preserve luff inclination and shape dimensions");
            }
            const auto mounted = leseKonfigurierteSegelgeometrie(sides);
            check_status(mounted, SegelErgebnisStatus::Vorschau, "Neutral persisted geometry");
            for (const auto& mesh : zero.meshes) {
                const auto found = std::find_if(mounted.meshes.begin(), mounted.meshes.end(),
                    [&](const InspectionMesh& other) { return mesh.id == other.id; });
                require(found != mounted.meshes.end() && mesh.points_m == found->points_m,
                        "Operating delta0 must equal confirmed neutral mounting");
            }
            if (std::string(id) != "420") {
                const auto display = getMesh(*inspection, "darstellung",
                    std::vector<std::string>{ref.element_id});
                require(display.size() == zero.meshes.size(),
                        "Generic inspection must display neutral OBJ sail");
                for (std::size_t i = 0; i < display.size(); ++i) {
                    require(display[i].points_m.size() == zero.meshes[i].points_m.size(),
                            "Generic and operating neutral geometry cardinalities must match");
                    for (std::size_t p = 0; p < display[i].points_m.size(); ++p)
                        require(point_distance(display[i].points_m[p], zero.meshes[i].points_m[p]) < 1e-8,
                                "Generic display and delta0 must apply mounting exactly once");
                }
            }
            require(entry.modell_fassung == modell_fassung(root / "modells_information.json"),
                    "Neutral settings and delta must leave fingerprint stable");
            const auto foot = subtract(clew, tack);
            const auto opening = std::atan2(foot[1], -foot[0]) * 180. / 3.14159265358979323846;
            std::cout << id << "/" << ref.element_id << " opening_deg=" << opening << " tack_z=" << tack[2]
                      << " boom_tack_gap_m=" << point_distance(tack, near_point) << '\n';
        }
        if (std::string(id) != "420")
            require(!snapshot.diagnosen.empty() && snapshot.diagnosen[0].code == "CORE-SEG-0030",
                    "Intrinsic/OBJ incompatibility must remain visible");
        if (std::string(id) == "optimist") {
            const auto& spriet = model_element(model, "Spriet");
            const auto& main = model_element(model, "Großsegel");
            require(spriet["setting"]["rotation_xyzw"] == main["setting"]["rotation_xyzw"] &&
                    spriet["setting"]["translation_m"] == main["setting"]["translation_m"],
                    "Spriet must follow the same rigid neutralization as the sail");
            const auto points = read_obj_vertices_independently(
                root / spriet["darstellung"]["geometrie"].get<std::string>());
            nlohmann::json far_ring = nlohmann::json::array();
            for (unsigned i = 2; i <= 64; i += 2) far_ring.push_back(i);
            const auto& main_reference = reference(snapshot, "Großsegel");
            const auto before_peak = *main_reference.ecken.kopf_achterliek_m;
            const auto before_spriet = indexed_center(points, far_ring);
            const auto& pose = lage(snapshot, "Großsegel");
            const auto after_gap = point_distance(transform_point(pose, before_peak),
                                                   transform_point(pose, before_spriet));
            if (main_reference.legacy_montage) {
                require(std::any_of(snapshot.diagnosen.begin(), snapshot.diagnosen.end(),
                    [](const SegelDiagnose& diag) { return diag.code == "CORE-SEG-0031"; }),
                    "Changed semantic sail dimensions must retain explicit legacy conflict");
                const auto spriet_display = getMesh(*inspection, "darstellung",
                    std::vector<std::string>{"Spriet"});
                require(spriet_display.size() == 1 &&
                    spriet_display.front().points_m.size() == points.size(),
                    "Contour adaptation must preserve the independent physical spriet");
                const auto stored_pose = Pose(
                    spriet["setting"]["translation_m"].get<Vektor3>(),
                    spriet["setting"]["rotation_xyzw"].get<QuaternionXyzw>());
                for (std::size_t i = 0; i < points.size(); ++i)
                    require(point_distance(spriet_display.front().points_m[i],
                        stored_pose.modellpunkt_nach_welt(points[i])) < 1e-9,
                        "Legacy adaptation must not silently reposition the spriet");
            } else {
                require(after_gap < 0.08 && close(after_gap, point_distance(before_peak, before_spriet), 1e-9),
                    "Neutral Spriet must preserve its measured sail-peak attachment offset");
            }
        }
    }
}

}

int main(int argc, char** argv)
{
    try {
        if (argc == 3 && std::string(argv[1]) == "--normalize-base") {
            normalize_boat_bases(fs::u8path(argv[2]));
            return 0;
        }
        if (argc == 3 && std::string(argv[1]) == "--zero-base-contract") {
            test_zero_boat_bases(fs::u8path(argv[2]));
            return 0;
        }
        if (argc == 3 && std::string(argv[1]) == "--fingerprint") {
            std::cout << segelphysik::modell_fassung(fs::u8path(argv[2])) << '\n';
            return 0;
        }
        if (argc == 4 && std::string(argv[1]) == "--writer")
            return write_from_worker(fs::u8path(argv[2]), argv[3]);
        if (argc != 2) throw std::runtime_error(
            "Usage: segelkonfigurator_tests <420-modells_information.json>");
        test_synthetic_operations();
        fs::path executable;
#ifdef _WIN32
        std::vector<wchar_t> executable_buffer(32768);
        const auto executable_length = GetModuleFileNameW(
            nullptr, executable_buffer.data(),
            static_cast<DWORD>(executable_buffer.size()));
        if (executable_length == 0 ||
            executable_length >= executable_buffer.size())
            throw std::runtime_error("Unable to find test executable path");
        executable = std::wstring(executable_buffer.data(), executable_length);
#else
        executable = fs::absolute(fs::u8path(argv[0]));
#endif
        test_configuration(fs::u8path(argv[1]), executable);
        test_empty_model_configurations(fs::u8path(argv[1]));
        std::cout << "Segelkonfiguration contract tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
