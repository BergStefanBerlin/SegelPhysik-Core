#include "segelphysik/segelkonfigurator.hpp"
#include "segelphysik/modell.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>

namespace {
using namespace segelphysik;
namespace fs = std::filesystem;
using Json = nlohmann::json;

void require(bool value, const std::string& message)
{
    if (!value) throw std::runtime_error(message);
}

double distance(const Vektor3& a, const Vektor3& b)
{
    double sum = 0.;
    for (std::size_t i = 0; i < 3; ++i) sum += (a[i] - b[i]) * (a[i] - b[i]);
    return std::sqrt(sum);
}

std::string bytes(const fs::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    require(static_cast<bool>(stream), "Cannot read model fixture");
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

Json& element(Json& model, const std::string& id)
{
    for (auto& group : model["gruppen"])
        for (auto& item : group["elemente"])
            if (item.at("id") == id) return item;
    throw std::runtime_error("Missing fixture element");
}

void save(const fs::path& path, const Json& value)
{
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream << value.dump(2) << '\n';
}

void require_ready(const SegelKonfigurationSnapshot& snapshot)
{
    if (snapshot.status != SegelKonfigurationsStatus::Fehler) return;
    throw std::runtime_error(snapshot.diagnosen.empty() ? "Invalid snapshot" :
        snapshot.diagnosen.front().nachricht);
}

void require_preview(const SegelKonfigurationErgebnis& result)
{
    if (result.status != SegelErgebnisStatus::Fehler) return;
    throw std::runtime_error(result.diagnosen.empty() ? "Invalid result" :
        result.diagnosen.front().nachricht);
}

void rigid(const std::vector<InspectionMesh>& raw, const std::vector<InspectionMesh>& moved)
{
    require(raw.size() == moved.size(), "Mounting changes mesh cardinality");
    for (std::size_t m = 0; m < raw.size(); ++m) {
        require(raw[m].points_m.size() == moved[m].points_m.size() &&
            raw[m].triangles == moved[m].triangles, "Mounting changes physical topology");
        const auto& a = raw[m].points_m;
        const auto& b = moved[m].points_m;
        for (std::size_t i = 0; i < a.size(); i += std::max<std::size_t>(1, a.size() / 29))
            require(std::abs(distance(a.front(), a[i]) - distance(b.front(), b[i])) < 1e-9,
                "Mounting deforms source geometry");
    }
}

void inspect_all(const fs::path& source)
{
    const auto catalog = modellkatalog(source);
    std::size_t models = 0, objects = 0, point_objects = 0;
    for (const auto& item : catalog.eintraege()) {
        require(item.status == "verwendbar", "Current model is not inspectable");
        const auto inspection = readModelInspection(catalog, item.modell_id, item.modell_fassung);
        const auto meshes = getMesh(*inspection, "darstellung");
        std::set<std::string> ids;
        for (const auto& mesh : meshes) {
            require(ids.insert(mesh.element_id).second, "Duplicate current object display");
            require(mesh.montage && mesh.montage->element_id == mesh.element_id,
                "Object has no common mounting reference");
            if (mesh.origin == "punktmasse_referenz") {
                require(mesh.topology == "punkte" && mesh.points_m.size() == 1 &&
                    mesh.triangles.empty(), "Point mass has guessed replacement geometry");
                const Pose pose(mesh.montage->translation_m, mesh.montage->rotation_xyzw);
                require(distance(mesh.points_m.front(), pose.modellpunkt_nach_welt(
                    mesh.montage->bezugspunkt_lokal_m)) < 1e-9,
                    "Point mass display differs from its declared object reference");
                ++point_objects;
            }
            ++objects;
        }
        for (const auto& node : getObjectTree(*inspection, "objekte"))
            if (node.kind == "element")
                require(node.mesh_ids.size() == 1, "Canonical object needs exactly one current reference");
        require_ready(leseSegelKonfiguration(catalog, *inspection));
        ++models;
    }
    require(models == 11 && objects == 38 && point_objects == 3,
        "Expected all eleven models and all 38 existing objects including three point masses");
    std::cout << "INSPECT " << models << " models, " << objects << " unique current references\n";
}

void boats(const fs::path& source, const fs::path& work)
{
    const auto original_catalog = modellkatalog(source);
    std::size_t boat_count = 0, sail_count = 0, action_count = 0;
    double max_tack_error = 0., max_luff_error = 0.;
    for (const auto& item : original_catalog.eintraege()) {
        const auto original_inspection = readModelInspection(original_catalog,
            item.modell_id, item.modell_fassung);
        const auto original_snapshot = leseSegelKonfiguration(original_catalog, *original_inspection);
        require_ready(original_snapshot);
        if (original_snapshot.referenzen.empty()) continue;
        const auto original_model = source / fs::u8path(item.modell_id) / "modells_information.json";
        const auto original_bytes = bytes(original_model);
        const auto root = work / fs::u8path(item.modell_id);
        fs::copy(original_model.parent_path(), root, fs::copy_options::recursive);
        const auto path = root / "modells_information.json";
        const auto base = modell_fassung(path);
        const auto catalog = modellkatalog(root);
        const auto inspection = readModelInspection(catalog, item.modell_id, base);
        auto snapshot = leseSegelKonfiguration(catalog, *inspection);
        require_ready(snapshot);
        const auto original_physics = lade_modell(path);
        const auto identities = getObjectTree(*inspection, "objekte");
        for (const auto& reference : original_snapshot.referenzen) {
            SegelAktionAnforderung request;
            request.element_id = reference.element_id;
            const std::vector<SegelAktion> actions{SegelAktion::Automatisch,
                SegelAktion::Translation, SegelAktion::Rotation,
                SegelAktion::Automatisch};
            for (const auto action : actions) {
                request.aktion = action;
                request.ebene = SegelEbene::XZ;
                request.translation_m = {0.13, 0., -0.07};
                request.winkel_grad = 17.;
                request.bewegte_ecke = SegelEcke::Schothorn;
                const auto before_file = bytes(path);
                const auto preview = pruefeSegelAktion(snapshot, request);
                require_preview(preview);
                require(before_file == bytes(path), "Preview writes source model");
                std::vector<InspectionMesh> raw;
                for (const auto& mesh : snapshot.grundmeshes)
                    if (mesh.element_id == request.element_id) raw.push_back(mesh);
                rigid(raw, preview.meshes);
                require(preview.lage.has_value(), "Preview has no rigid pose");
                for (const auto& mesh : preview.meshes)
                    require(mesh.montage &&
                        mesh.montage->rotation_xyzw == preview.lage->rotation_xyzw &&
                        mesh.montage->translation_m == preview.lage->translation_m,
                        "Preview common mounting reference is stale");
                const Pose pose(preview.lage->translation_m, preview.lage->rotation_xyzw);
                if (action == SegelAktion::Automatisch) {
                    const auto tack = pose.modellpunkt_nach_welt(reference.ecken.hals_m);
                    const auto head = pose.modellpunkt_nach_welt(reference.ecken.kopf_vorliek_m);
                    const auto clew = pose.modellpunkt_nach_welt(reference.ecken.schothorn_m);
                    const auto tack_error = distance(tack, reference.rig_hals_anschlag_m);
                    Vektor3 luff{}, target = reference.rig_vorliek_richtung_modell;
                    double luff_length = 0., target_length = 0.;
                    for (std::size_t i = 0; i < 3; ++i) {
                        luff[i] = head[i] - tack[i];
                        luff_length += luff[i] * luff[i];
                        target_length += target[i] * target[i];
                    }
                    for (std::size_t i = 0; i < 3; ++i) {
                        luff[i] /= std::sqrt(luff_length);
                        target[i] /= std::sqrt(target_length);
                    }
                    const auto luff_error = distance(luff, target);
                    max_tack_error = std::max(max_tack_error, tack_error);
                    max_luff_error = std::max(max_luff_error, luff_error);
                    require(tack_error < 1e-6 && luff_error < 1e-6 &&
                        std::abs(tack[1]) < 1e-7 && std::abs(head[1]) < 1e-7 &&
                        std::abs(clew[1]) < 1e-7, "Auto does not realize neutral semantic mounting");
                    auto repeated_snapshot = snapshot;
                    for (auto& lage : repeated_snapshot.lagen)
                        if (lage.element_id == request.element_id) lage = *preview.lage;
                    const auto repeated = pruefeSegelAktion(repeated_snapshot, request);
                    require_preview(repeated);
                    require(repeated.meshes.front().points_m == preview.meshes.front().points_m,
                        "Automatic mounting is not idempotent");
                } else {
                    const auto old = std::find_if(snapshot.lagen.begin(), snapshot.lagen.end(),
                        [&](const SegelLage& lage) { return lage.element_id == request.element_id; });
                    require(old != snapshot.lagen.end(), "Missing manual starting pose");
                    const Pose previous(old->translation_m, old->rotation_xyzw);
                    if (action == SegelAktion::Translation) {
                        for (const auto& mesh : raw)
                            for (const auto& point : mesh.points_m) {
                                auto expected = previous.modellpunkt_nach_welt(point);
                                for (std::size_t i = 0; i < 3; ++i)
                                    expected[i] += request.translation_m[i];
                                require(distance(expected, pose.modellpunkt_nach_welt(point)) < 1e-9,
                                    "Manual translation does not match requested metric offset");
                            }
                    } else {
                        const auto pivot = previous.modellpunkt_nach_welt(reference.ecken.hals_m);
                        require(distance(pivot, pose.modellpunkt_nach_welt(reference.ecken.hals_m)) < 1e-9,
                            "Rigid corner rotation moves fixed tack pivot");
                        const auto clew = previous.modellpunkt_nach_welt(reference.ecken.schothorn_m);
                        const double angle = request.winkel_grad * 3.14159265358979323846 / 180.;
                        const Vektor3 expected{
                            pivot[0] + std::cos(angle) * (clew[0] - pivot[0]) +
                                std::sin(angle) * (clew[2] - pivot[2]),
                            clew[1],
                            pivot[2] - std::sin(angle) * (clew[0] - pivot[0]) +
                                std::cos(angle) * (clew[2] - pivot[2])};
                        require(distance(expected, pose.modellpunkt_nach_welt(
                            reference.ecken.schothorn_m)) < 1e-9,
                            "Manual corner rotation does not realize requested angle");
                    }
                }
                const auto confirmed = bestaetigeSegelAktion(catalog, *inspection,
                    snapshot, request, snapshot.revision);
                require_preview(confirmed);
                require(confirmed.status == SegelErgebnisStatus::Gespeichert,
                    "Confirmed mounting was not persisted");
                require(modell_fassung(path) == base, "Setting changes immutable base fingerprint");
                snapshot = leseSegelKonfiguration(catalog, *inspection);
                require_ready(snapshot);
                ++action_count;
            }
            if (reference.art == SegelArt::Fock) {
                request.aktion = SegelAktion::FockVersatzCm;
                request.fock_versatz_cm = 10.;
                const auto confirmed = bestaetigeSegelAktion(catalog, *inspection,
                    snapshot, request, snapshot.revision);
                require_preview(confirmed);
                snapshot = leseSegelKonfiguration(catalog, *inspection);
                require_ready(snapshot);
                const auto pose = std::find_if(snapshot.lagen.begin(), snapshot.lagen.end(),
                    [&](const SegelLage& lage) { return lage.element_id == request.element_id; });
                require(pose != snapshot.lagen.end() &&
                    std::abs(pose->fock_versatz_cm - 10.) < 1e-7, "Fock offset persistence mismatch");
                ++action_count;
            }
            ++sail_count;
        }
        const auto reload_catalog = modellkatalog(root);
        const auto reload_inspection = readModelInspection(reload_catalog, item.modell_id, base);
        const auto reload = leseSegelKonfiguration(reload_catalog, *reload_inspection);
        require_ready(reload);
        require(reload.revision == snapshot.revision, "Reload lost configuration revision");
        const auto displayed = getMesh(*reload_inspection, "darstellung");
        const auto configured = leseKonfigurierteSegelgeometrie(reload);
        require_preview(configured);
        for (const auto& mesh : configured.meshes) {
            const auto current = std::find_if(displayed.begin(), displayed.end(),
                [&](const InspectionMesh& display) { return display.element_id == mesh.element_id; });
            require(current != displayed.end() && current->points_m == mesh.points_m,
                "Inspection current geometry differs from persisted mounting");
        }
        const auto final_physics = lade_modell(path);
        require(original_physics.masse_kg == final_physics.masse_kg &&
            original_physics.schwerpunkt_m == final_physics.schwerpunkt_m &&
            original_physics.traegheit_kg_m2 == final_physics.traegheit_kg_m2,
            "Mounting changed physical mass properties");
        require(identities.size() == getObjectTree(*reload_inspection, "objekte").size(),
            "Mounting created new object identities");
        require(bytes(original_model) == original_bytes, "Original source model modified");

        auto changed = Json::parse(bytes(path));
        const auto id = snapshot.referenzen.front().element_id;
        element(changed, id)["setting"]["montage_referenz"]["obj_sha256"] = "stale";
        save(path, changed);
        const auto stale = leseSegelKonfiguration(catalog, *inspection);
        require(stale.status == SegelKonfigurationsStatus::Fehler &&
            stale.diagnosen.front().code == "CORE-SEG-0031" &&
            stale.diagnosen.front().nachricht.find("Montagequelle") != std::string::npos,
            "Stale confirmed reference must be explicitly rejected");
        ++boat_count;
    }
    require(boat_count == 3 && sail_count == 5, "Expected all three current boats and five sails");
    std::cout << "MOUNT " << boat_count << " boats, " << sail_count << " sails, "
        << action_count << " rigid persisted actions; max tack error " << max_tack_error
        << " m, max directed luff error " << max_luff_error << "; stale checks PASS\n";
}
}

int main(int argc, char** argv)
{
    try {
        require(argc == 3, "Usage: montage_tests <models-root> <build-root>");
        const auto source = fs::absolute(fs::u8path(argv[1]));
        const auto work = fs::absolute(fs::u8path(argv[2])) /
            ("montage-fixtures-" + std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()));
        fs::create_directories(work);
        struct Cleanup {
            fs::path path;
            ~Cleanup() { std::error_code ignored; fs::remove_all(path, ignored); }
        } cleanup{work};
        inspect_all(source);
        boats(source, work);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
