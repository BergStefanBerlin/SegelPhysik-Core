#include "segelphysik/segelkonfigurator.hpp"
#include "segelphysik/defaults.hpp"
#include "modell_intern.hpp"
#include "modellformat_03_intern.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <limits>
#include <map>
#include <sstream>
#include <set>
#include <stdexcept>
#include <string>
#include <system_error>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif

namespace segelphysik {
namespace {

using Json = nlohmann::json;
constexpr double pi = 3.141592653589793238462643383279502884;
constexpr double matrix_tolerance = 1e-10;

struct Sha256 {
    std::array<std::uint32_t, 8> state{
        0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
        0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U};
    std::array<unsigned char, 64> block{};
    std::uint64_t bit_count{};
    std::size_t used{};

    static std::uint32_t rotate(std::uint32_t value, unsigned amount)
    {
        return (value >> amount) | (value << (32U - amount));
    }

    void transform()
    {
        static constexpr std::array<std::uint32_t, 64> constants{
            0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U,
            0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
            0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
            0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
            0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
            0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
            0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
            0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
            0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
            0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
            0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U,
            0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
            0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U,
            0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
            0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
            0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U};
        std::array<std::uint32_t, 64> words{};
        for (std::size_t i = 0; i < 16; ++i) {
            const auto offset = i * 4;
            words[i] = (static_cast<std::uint32_t>(block[offset]) << 24U) |
                (static_cast<std::uint32_t>(block[offset + 1]) << 16U) |
                (static_cast<std::uint32_t>(block[offset + 2]) << 8U) |
                static_cast<std::uint32_t>(block[offset + 3]);
        }
        for (std::size_t i = 16; i < words.size(); ++i) {
            const auto s0 = rotate(words[i - 15], 7) ^
                rotate(words[i - 15], 18) ^ (words[i - 15] >> 3U);
            const auto s1 = rotate(words[i - 2], 17) ^
                rotate(words[i - 2], 19) ^ (words[i - 2] >> 10U);
            words[i] = words[i - 16] + s0 + words[i - 7] + s1;
        }
        auto a = state[0];
        auto b = state[1];
        auto c = state[2];
        auto d = state[3];
        auto e = state[4];
        auto f = state[5];
        auto g = state[6];
        auto h = state[7];
        for (std::size_t i = 0; i < words.size(); ++i) {
            const auto sum1 = rotate(e, 6) ^ rotate(e, 11) ^ rotate(e, 25);
            const auto choice = (e & f) ^ (~e & g);
            const auto temp1 = h + sum1 + choice + constants[i] + words[i];
            const auto sum0 = rotate(a, 2) ^ rotate(a, 13) ^ rotate(a, 22);
            const auto majority = (a & b) ^ (a & c) ^ (b & c);
            const auto temp2 = sum0 + majority;
            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }
        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        state[4] += e;
        state[5] += f;
        state[6] += g;
        state[7] += h;
    }

    void update(const unsigned char* bytes, std::size_t count)
    {
        bit_count += static_cast<std::uint64_t>(count) * 8U;
        for (std::size_t i = 0; i < count; ++i) {
            block[used++] = bytes[i];
            if (used == block.size()) {
                transform();
                used = 0;
            }
        }
    }

    std::string finish()
    {
        const auto original_bits = bit_count;
        const unsigned char marker = 0x80U;
        update(&marker, 1);
        const unsigned char zero = 0;
        while (used != 56) update(&zero, 1);
        std::array<unsigned char, 8> length{};
        for (std::size_t i = 0; i < length.size(); ++i)
            length[7 - i] = static_cast<unsigned char>(original_bits >> (i * 8U));
        update(length.data(), length.size());
        std::ostringstream result;
        result << std::hex << std::setfill('0');
        for (const auto word : state) result << std::setw(8) << word;
        return result.str();
    }
};

std::string sha256(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Quelldatei nicht lesbar: " + path.filename().u8string());
    Sha256 digest;
    std::array<unsigned char, 64 * 1024> buffer{};
    while (input) {
        input.read(reinterpret_cast<char*>(buffer.data()),
                   static_cast<std::streamsize>(buffer.size()));
        const auto count = input.gcount();
        if (count > 0) digest.update(buffer.data(), static_cast<std::size_t>(count));
    }
    if (input.bad()) throw std::runtime_error("Lesefehler: " + path.filename().u8string());
    return digest.finish();
}

using Matrix = std::array<double, 9>;

Vektor3 add(const Vektor3& a, const Vektor3& b)
{
    return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}

Vektor3 subtract(const Vektor3& a, const Vektor3& b)
{
    return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

Vektor3 scale(const Vektor3& a, double factor)
{
    return {a[0] * factor, a[1] * factor, a[2] * factor};
}

double dot(const Vektor3& a, const Vektor3& b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

Vektor3 cross(const Vektor3& a, const Vektor3& b)
{
    return {a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0]};
}

double norm(const Vektor3& value)
{
    return std::sqrt(dot(value, value));
}

bool finite(const Vektor3& value)
{
    return std::isfinite(value[0]) && std::isfinite(value[1]) && std::isfinite(value[2]);
}

Vektor3 normalized(const Vektor3& value)
{
    const auto length = norm(value);
    if (!(length > 1e-12) || !std::isfinite(length))
        throw std::runtime_error("Referenzrichtung ist nicht normierbar");
    return scale(value, 1. / length);
}

Matrix matrix_from_quaternion(const QuaternionXyzw& q)
{
    const double x = q[0], y = q[1], z = q[2], w = q[3];
    return {
        1. - 2. * (y * y + z * z), 2. * (x * y - z * w), 2. * (x * z + y * w),
        2. * (x * y + z * w), 1. - 2. * (x * x + z * z), 2. * (y * z - x * w),
        2. * (x * z - y * w), 2. * (y * z + x * w), 1. - 2. * (x * x + y * y)};
}

Vektor3 rotate(const Matrix& matrix, const Vektor3& value)
{
    return {matrix[0] * value[0] + matrix[1] * value[1] + matrix[2] * value[2],
            matrix[3] * value[0] + matrix[4] * value[1] + matrix[5] * value[2],
            matrix[6] * value[0] + matrix[7] * value[1] + matrix[8] * value[2]};
}

Matrix multiply(const Matrix& a, const Matrix& b)
{
    Matrix result{};
    for (std::size_t r = 0; r < 3; ++r)
        for (std::size_t c = 0; c < 3; ++c)
            for (std::size_t k = 0; k < 3; ++k)
                result[r * 3 + c] += a[r * 3 + k] * b[k * 3 + c];
    return result;
}

QuaternionXyzw quaternion_from_matrix(const Matrix& m)
{
    QuaternionXyzw q{};
    const double trace = m[0] + m[4] + m[8];
    if (trace > 0.) {
        const double s = std::sqrt(trace + 1.) * 2.;
        q = {(m[7] - m[5]) / s, (m[2] - m[6]) / s,
             (m[3] - m[1]) / s, .25 * s};
    } else if (m[0] > m[4] && m[0] > m[8]) {
        const double s = std::sqrt(1. + m[0] - m[4] - m[8]) * 2.;
        q = {.25 * s, (m[1] + m[3]) / s, (m[2] + m[6]) / s,
             (m[7] - m[5]) / s};
    } else if (m[4] > m[8]) {
        const double s = std::sqrt(1. + m[4] - m[0] - m[8]) * 2.;
        q = {(m[1] + m[3]) / s, .25 * s, (m[5] + m[7]) / s,
             (m[2] - m[6]) / s};
    } else {
        const double s = std::sqrt(1. + m[8] - m[0] - m[4]) * 2.;
        q = {(m[2] + m[6]) / s, (m[5] + m[7]) / s, .25 * s,
             (m[3] - m[1]) / s};
    }
    const double length = std::sqrt(q[0] * q[0] + q[1] * q[1] +
                                    q[2] * q[2] + q[3] * q[3]);
    for (auto& value : q) value /= length;
    return q;
}

QuaternionXyzw quaternion_multiply(const QuaternionXyzw& a, const QuaternionXyzw& b)
{
    return {
        a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1],
        a[3] * b[1] - a[0] * b[2] + a[1] * b[3] + a[2] * b[0],
        a[3] * b[2] + a[0] * b[1] - a[1] * b[0] + a[2] * b[3],
        a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2]};
}

QuaternionXyzw axis_angle(const Vektor3& axis, double radians)
{
    const auto direction = normalized(axis);
    const double half = radians / 2.;
    const double sine = std::sin(half);
    return {direction[0] * sine, direction[1] * sine, direction[2] * sine,
            std::cos(half)};
}

bool valid_rotation(const QuaternionXyzw& q)
{
    for (const auto value : q) if (!std::isfinite(value)) return false;
    const double length = std::sqrt(q[0] * q[0] + q[1] * q[1] +
                                    q[2] * q[2] + q[3] * q[3]);
    if (std::abs(length - 1.) > matrix_tolerance) return false;
    const auto matrix = matrix_from_quaternion(q);
    const auto product = multiply(matrix, Matrix{
        matrix[0], matrix[3], matrix[6], matrix[1], matrix[4], matrix[7],
        matrix[2], matrix[5], matrix[8]});
    for (std::size_t i = 0; i < product.size(); ++i) {
        const double expected = (i % 4 == 0) ? 1. : 0.;
        if (std::abs(product[i] - expected) > matrix_tolerance) return false;
    }
    const double determinant =
        matrix[0] * (matrix[4] * matrix[8] - matrix[5] * matrix[7]) -
        matrix[1] * (matrix[3] * matrix[8] - matrix[5] * matrix[6]) +
        matrix[2] * (matrix[3] * matrix[7] - matrix[4] * matrix[6]);
    return std::abs(determinant - 1.) <= matrix_tolerance;
}

std::string buglage_name(SegelBuglage side)
{
    switch (side) {
    case SegelBuglage::Unbekannt: return "unbekannt";
    case SegelBuglage::Backbord: return "backbord";
    case SegelBuglage::Steuerbord: return "steuerbord";
    }
    throw std::runtime_error("Ungueltige Original-Buglage");
}

SegelBuglage parse_buglage(const std::string& value)
{
    if (value == "unbekannt") return SegelBuglage::Unbekannt;
    if (value == "backbord") return SegelBuglage::Backbord;
    if (value == "steuerbord") return SegelBuglage::Steuerbord;
    throw std::runtime_error("Ungueltige Original-Buglage");
}

Vektor3 json_vector(const Json& value)
{
    if (!value.is_array() || value.size() != 3)
        throw std::runtime_error("Ungueltiger 3D-Punkt in Segelquelle");
    Vektor3 result{};
    for (std::size_t i = 0; i < 3; ++i) {
        if (!value[i].is_number()) throw std::runtime_error("Nichtnumerischer Segelpunkt");
        result[i] = value[i].get<double>();
        if (!std::isfinite(result[i])) throw std::runtime_error("Nichtendlicher Segelpunkt");
    }
    return result;
}

Json read_model_json(const std::filesystem::path& path)
{
    if (!detail::ist_modellformat03_datei(path))
        throw std::runtime_error("CORE-MOD-0002: Nur Modellformat 0.3 wird unterstuetzt");
    const auto model_folder = path.parent_path();
    const auto validation = detail::pruefe_modellformat03(
        model_folder, schemaordner_fuer_modellordner(model_folder), false);
    if (!validation.schemas_gueltig || !validation.fehler.empty())
        throw std::runtime_error(validation.fehler.empty()
            ? "CORE-MOD-0002: Modellformat-0.3-Schema-Pruefung fehlgeschlagen"
            : validation.fehler.front());
    return detail::normalisiere_modellformat03(validation.daten, model_folder);
}

std::string whole_source_fingerprint(const std::vector<SegelReferenz>& references)
{
    std::string combined;
    for (const auto& reference : references)
        combined += reference.element_id + ":" + reference.quellen_fingerprint + "\n";
    Sha256 digest;
    digest.update(reinterpret_cast<const unsigned char*>(combined.data()), combined.size());
    return digest.finish();
}

std::string json_string(const Json& object, const char* field)
{
    if (!object.contains(field) || !object[field].is_string())
        throw std::runtime_error(std::string("Ungueltige Referenz: ") + field);
    return object[field].get<std::string>();
}

const Json& find_model_element(const Json& model, const std::string& id)
{
    std::size_t count = 0;
    const Json* found = nullptr;
    if (!model.contains("gruppen") || !model["gruppen"].is_object())
        throw std::runtime_error("Modellgruppen fehlen");
    for (const auto& group : model["gruppen"].items()) {
        if (!group.value().contains("elemente") || !group.value()["elemente"].is_array()) continue;
        for (const auto& element : group.value()["elemente"]) {
            if (element.value("id", std::string{}) == id) {
                ++count;
                found = &element;
            }
        }
    }
    if (count != 1 || found == nullptr)
        throw std::runtime_error(count == 0 ? "Segel-Element fehlt: " + id :
                                 "Mehrdeutige Segel-Element-ID: " + id);
    return *found;
}

std::string display_file_reference(const Json&, const Json& element)
{
    return json_string(element.at("Darstellung"), "datei");
}

Matrix corner_basis(const SegelEcken& corners)
{
    const auto z = normalized(subtract(corners.kopf_vorliek_m, corners.hals_m));
    const auto foot = subtract(corners.schothorn_m, corners.hals_m);
    const auto x = scale(normalized(subtract(foot, scale(z, dot(foot, z)))), -1.);
    const auto y = normalized(cross(z, x));
    return {x[0], y[0], z[0], x[1], y[1], z[1], x[2], y[2], z[2]};
}

Matrix transpose(const Matrix& m)
{
    return {m[0], m[3], m[6], m[1], m[4], m[7], m[2], m[5], m[8]};
}

QuaternionXyzw rig_rotation(const SegelReferenz& reference, bool main_sail);

std::optional<Vektor3> montagepunkt03(const Json& model, const Json& mounting,
                                      const char* role, std::string& error)
{
    if (!mounting.contains(role)) {
        error = std::string("Montagebezug fehlt: ") + role;
        return std::nullopt;
    }
    const auto& reference = mounting.at(role);
    const auto id = reference.at("element").get<std::string>();
    const auto point = reference.at("point").get<std::string>();
    const Json* found = nullptr;
    for (const auto& group : model.at("gruppen"))
        for (const auto& element : group.at("elemente")) {
            if (element.value("id", std::string{}) != id) continue;
            if (found) {
                error = "Montageelement ist nicht eindeutig: " + id;
                return std::nullopt;
            }
            found = &element;
        }
    if (!found || !found->contains("mountingPoints") ||
        !found->at("mountingPoints").contains(point)) {
        error = "Montagepunkt fehlt: " + id + "." + point;
        return std::nullopt;
    }
    const auto local = json_vector(found->at("mountingPoints").at(point));
    const auto* setting = element_setting(model, id);
    return setting ? setting_pose(*setting).modellpunkt_nach_welt(local) : local;
}

void apply_montage03(const Json& model, const Json& element, SegelReferenz& result,
                     std::vector<SegelDiagnose>& diagnostics)
{
    std::string error;
    std::optional<Vektor3> tack, upper, boom_start, boom_end;
    if (!element.contains("mounting"))
        error = "Montagehilfe fehlt";
    else {
        const auto& mounting = element.at("mounting");
        tack = montagepunkt03(model, mounting, "tack", error);
        if (tack) upper = montagepunkt03(model, mounting, "luffUpper", error);
        if (upper && result.art == SegelArt::Grosssegel) {
            boom_start = montagepunkt03(model, mounting, "boomStart", error);
            if (boom_start) boom_end = montagepunkt03(model, mounting, "boomEnd", error);
        }
        if (upper && norm(subtract(*upper, *tack)) <= 1e-12)
            error = "Vorliek-Montagepunkte sind identisch";
        if (boom_start && boom_end && norm(subtract(*boom_end, *boom_start)) <= 1e-12)
            error = "Baum-Montagepunkte sind identisch";
        if (error.empty() && upper && tack) {
            const auto luff = subtract(*upper, *tack);
            const auto foot = boom_start && boom_end
                ? subtract(*boom_end, *boom_start) : Vektor3{-1., 0., 0.};
            if (norm(cross(luff, foot)) <= 1e-12 * norm(luff) * norm(foot))
                error = "Montageachsen sind parallel";
        }
    }
    if (!error.empty()) {
        diagnostics.push_back({"CORE-SEG-0033",
            error + "; bitte Segel manuell platzieren.", result.element_id});
        return;
    }
    result.rig_hals_anschlag_m = *tack;
    result.rig_vorliek_richtung_modell = subtract(*upper, *tack);
    if (boom_start && boom_end)
        result.rig_unterliek_richtung_modell = subtract(*boom_end, *boom_start);
    result.fock_grundhals_m = *tack;
    result.fock_versatzrichtung_modell = result.rig_vorliek_richtung_modell;
    result.montage_basis_vorgegeben = false;
}

SegelReferenz sail_reference03(const std::filesystem::path& root, const Json& model,
                                const Json& element, SegelArt art,
                                std::vector<SegelDiagnose>& diagnostics)
{
    SegelReferenz result;
    result.element_id = json_string(element, "id");
    result.art = art;
    const auto display = display_file_reference(model, element);
    result.darstellungs_mesh_id = display;
    result.intrinsische_quelle_id = display;
    if (!element.contains("mountingPoints"))
        throw std::runtime_error("CORE-SEG-0033: Segel-Eckpunkte fehlen: " + result.element_id);
    const auto& points = element.at("mountingPoints");
    for (const auto* name : {"tack", "clew", "luffUpper"})
        if (!points.contains(name))
            throw std::runtime_error(std::string("CORE-SEG-0033: Segel-Eckpunkt fehlt: ") +
                                     result.element_id + "." + name);
    result.ecken.hals_m = json_vector(points.at("tack"));
    result.ecken.schothorn_m = json_vector(points.at("clew"));
    result.ecken.kopf_vorliek_m = json_vector(points.at("luffUpper"));
    if (points.contains("leechUpper"))
        result.ecken.kopf_achterliek_m = json_vector(points.at("leechUpper"));
    (void)corner_basis(result.ecken);
    result.vorliek_richtung_lokal = subtract(result.ecken.kopf_vorliek_m, result.ecken.hals_m);
    result.unterliek_richtung_lokal = subtract(result.ecken.schothorn_m, result.ecken.hals_m);
    const auto pose = setting_pose(element.at("setting"));
    result.montage.element_id = result.element_id;
    result.montage.rotation_xyzw = pose.orientierung_xyzw();
    result.montage.translation_m = pose.position_m();
    result.montage.bezugspunkt_lokal_m = result.ecken.hals_m;
    result.montage.laengsrichtung_lokal = result.vorliek_richtung_lokal;
    result.montage.querrichtung_lokal = result.unterliek_richtung_lokal;
    result.montage.herkunft = "Benannte Segelpunkte im Visualisierungsmesh";
    result.quellen_fingerprint = sha256_bytes(
        display + ":" + sha256(root / std::filesystem::u8path(display)) + "\n" +
        points.dump() + "\n" + (element.contains("mounting") ? element.at("mounting").dump() : ""));
    result.montage.quellen_fingerprint = result.quellen_fingerprint;
    result.rig_hals_anschlag_m = pose.modellpunkt_nach_welt(result.ecken.hals_m);
    result.rig_vorliek_richtung_modell = pose.modellvektor_nach_welt(result.vorliek_richtung_lokal);
    result.rig_unterliek_richtung_modell = pose.modellvektor_nach_welt(result.unterliek_richtung_lokal);
    result.fock_grundhals_m = result.rig_hals_anschlag_m;
    result.fock_versatzrichtung_modell = result.rig_vorliek_richtung_modell;
    result.montage_basis_vorgegeben = true;
    apply_montage03(model, element, result, diagnostics);
    if (!result.montage_basis_vorgegeben) {
        result.montage.rotation_xyzw = quaternion_from_matrix(multiply(
            matrix_from_quaternion(rig_rotation(result, art == SegelArt::Grosssegel)),
            transpose(corner_basis(result.ecken))));
        result.montage.translation_m = subtract(result.rig_hals_anschlag_m,
            rotate(matrix_from_quaternion(result.montage.rotation_xyzw), result.ecken.hals_m));
    }
    return result;
}

std::filesystem::path model_root(const Modellkatalog& catalog,
                                 const ModelInspection& inspection)
{
    const auto& info = inspection.info();
    return katalog_datei(catalog, info.model_id, info.model_version).parent_path();
}

std::vector<InspectionMesh> copy_meshes(const SegelKonfigurationSnapshot& snapshot,
                                        const std::string& element_id)
{
    std::vector<InspectionMesh> result;
    for (const auto& mesh : snapshot.grundmeshes)
        if (mesh.element_id == element_id) result.push_back(mesh);
    return result;
}

const SegelReferenz* find_reference(const SegelKonfigurationSnapshot& snapshot,
                                    const std::string& element_id)
{
    const SegelReferenz* result = nullptr;
    for (const auto& reference : snapshot.referenzen) {
        if (reference.element_id != element_id) continue;
        if (result != nullptr) return nullptr;
        result = &reference;
    }
    return result;
}

SegelLage* find_lage(SegelKonfigurationSnapshot& snapshot, const std::string& element_id)
{
    SegelLage* result = nullptr;
    for (auto& lage : snapshot.lagen) {
        if (lage.element_id != element_id) continue;
        if (result != nullptr) return nullptr;
        result = &lage;
    }
    return result;
}

const SegelLage* find_lage(const SegelKonfigurationSnapshot& snapshot,
                           const std::string& element_id)
{
    const SegelLage* result = nullptr;
    for (const auto& lage : snapshot.lagen) {
        if (lage.element_id != element_id) continue;
        if (result != nullptr) return nullptr;
        result = &lage;
    }
    return result;
}

SegelKonfigurationErgebnis error_result(const std::string& code, const std::string& text,
                                        const std::string& element_id = {})
{
    SegelKonfigurationErgebnis result;
    result.status = SegelErgebnisStatus::Fehler;
    result.diagnosen.push_back({code, text, element_id});
    return result;
}

SegelKonfigurationSnapshot snapshot_error(const std::string& code, const std::string& text)
{
    SegelKonfigurationSnapshot result;
    result.status = SegelKonfigurationsStatus::Fehler;
    result.diagnosen.push_back({code, text, {}});
    return result;
}

Vektor3 corner(const SegelReferenz& reference, SegelEcke which)
{
    switch (which) {
    case SegelEcke::Hals: return reference.ecken.hals_m;
    case SegelEcke::Schothorn: return reference.ecken.schothorn_m;
    case SegelEcke::KopfVorliek: return reference.ecken.kopf_vorliek_m;
    case SegelEcke::KopfAchterliek:
        if (reference.ecken.kopf_achterliek_m) return *reference.ecken.kopf_achterliek_m;
        throw std::runtime_error("Das Segel hat keine hintere Kopfecke");
    }
    throw std::runtime_error("Ungueltiger semantischer Segelpunkt");
}

Vektor3 plane_normal(SegelEbene plane)
{
    switch (plane) {
    case SegelEbene::XY: return {0., 0., 1.};
    case SegelEbene::XZ: return {0., 1., 0.};
    case SegelEbene::YZ: return {1., 0., 0.};
    }
    throw std::runtime_error("Ungueltige Modellachsenebene");
}

bool translation_in_plane(const Vektor3& value, SegelEbene plane)
{
    const auto normal = plane_normal(plane);
    return std::abs(dot(value, normal)) <= 1e-12;
}

QuaternionXyzw rig_rotation(const SegelReferenz& reference, bool main_sail)
{
    const auto z = normalized(reference.rig_vorliek_richtung_modell);
    Vektor3 under = reference.rig_unterliek_richtung_modell;
    if (!main_sail) {
        const Vektor3 aft{-1., 0., 0.};
        under = subtract(aft, scale(z, dot(aft, z)));
    }
    const auto under_direction = normalized(subtract(under, scale(z, dot(under, z))));
    const auto x = scale(under_direction, -1.);
    const auto y = normalized(cross(z, x));
    const auto x_corrected = normalized(cross(y, z));
    const Matrix rotation{
        x_corrected[0], y[0], z[0],
        x_corrected[1], y[1], z[1],
        x_corrected[2], y[2], z[2]};
    const auto q = quaternion_from_matrix(rotation);
    if (!valid_rotation(q)) throw std::runtime_error("Automatik erzeugt keine starre Drehung");
    return q;
}

Vektor3 transform_point(const SegelLage& lage, const Vektor3& point)
{
    return add(rotate(matrix_from_quaternion(lage.rotation_xyzw), point),
               lage.translation_m);
}

bool should_mirror(SegelBuglage original, double delta, std::string& error)
{
    try {
        (void)buglage_name(original);
    } catch (const std::exception& exception) {
        error = exception.what();
        return false;
    }
    if (std::abs(delta) <= 1e-12) return false;
    if (original == SegelBuglage::Unbekannt) {
        error = "Original-Buglage unbekannt; benoetigte Formspiegelung nicht bestimmbar";
        return false;
    }
    const auto target = delta > 0. ? SegelBuglage::Backbord : SegelBuglage::Steuerbord;
    return original != target;
}

std::vector<InspectionMesh> render_meshes(const SegelKonfigurationSnapshot& snapshot,
                                          const SegelReferenz& reference,
                                          const SegelLage& lage, double delta)
{
    std::string mirror_error;
    const bool mirror = should_mirror(lage.original_buglage, delta, mirror_error);
    if (!mirror_error.empty()) throw std::runtime_error(mirror_error);
    auto meshes = copy_meshes(snapshot, reference.element_id);
    if (meshes.empty()) throw std::runtime_error("Segel-Darstellungsmesh fehlt");

    const auto pivot = reference.ecken.hals_m;
    const auto luff = subtract(reference.ecken.kopf_vorliek_m, pivot);
    const auto foot = subtract(reference.ecken.schothorn_m, pivot);
    const auto axis = scale(normalized(luff), -1.);
    const auto q_stell = axis_angle(axis, delta * pi / 180.);
    const auto q_total = quaternion_multiply(lage.rotation_xyzw, q_stell);
    const auto total_matrix = matrix_from_quaternion(q_total);
    for (auto& mesh : meshes) {
        mesh.montage = reference.montage;
        mesh.montage->rotation_xyzw = lage.rotation_xyzw;
        mesh.montage->translation_m = lage.translation_m;
        for (auto& point : mesh.points_m) {
            point = subtract(point, pivot);
            if (mirror) {
                const auto x = normalized(subtract(foot,
                    scale(normalized(axis), dot(foot, normalized(axis)))));
                const auto normal = normalized(cross(axis, x));
                point = subtract(point, scale(normal, 2. * dot(point, normal)));
            }
            point = add(rotate(total_matrix, point), transform_point(lage, pivot));
            if (!finite(point)) throw std::runtime_error("Transformation erzeugt ungueltige Geometrie");
        }
        if (mirror)
            for (auto& triangle : mesh.triangles) std::swap(triangle[1], triangle[2]);
    }
    return meshes;
}

SegelKonfigurationSnapshot load_snapshot(const Json& model, const std::string& model_id,
                                         const std::string& model_version,
                                         std::vector<SegelReferenz> references,
                                         std::vector<InspectionMesh> meshes)
{
    SegelKonfigurationSnapshot result;
    result.modell_id = model_id;
    result.modell_fassung = model_version;
    result.revision = model_version;
    result.referenzen = std::move(references);
    result.grundmeshes = std::move(meshes);
    result.quellen_fingerprint = whole_source_fingerprint(result.referenzen);
    validiere_element_settings(model);
    for (const auto& reference : result.referenzen) {
        SegelLage lage;
        lage.element_id = reference.element_id;
        lage.original_buglage = reference.original_buglage;
        const auto* setting = element_setting(model, reference.element_id);
        if (setting) {
            lage.rotation_xyzw = setting->at("rotation_xyzw").get<QuaternionXyzw>();
            lage.translation_m = setting->at("translation_m").get<Vektor3>();
            lage.konfiguriert = true;
            if (setting->contains("original_buglage"))
                lage.original_buglage =
                    parse_buglage(setting->at("original_buglage").get<std::string>());
        }
        if (reference.art == SegelArt::Fock)
            lage.fock_versatz_cm = 100. * dot(
                subtract(transform_point(lage, reference.ecken.hals_m),
                    reference.fock_grundhals_m),
                normalized(reference.fock_versatzrichtung_modell));
        result.lagen.push_back(lage);
    }
    result.status = SegelKonfigurationsStatus::NichtKonfiguriert;
    return result;
}

SegelKonfigurationErgebnis preview_from(SegelKonfigurationSnapshot snapshot,
                                        const SegelAktionAnforderung& request)
{
    if (snapshot.status == SegelKonfigurationsStatus::Fehler)
        return error_result("CORE-SEG-0001", "Konfigurationssnapshot ist ungueltig");
    const auto* reference = find_reference(snapshot, request.element_id);
    auto* lage = find_lage(snapshot, request.element_id);
    if (reference == nullptr || lage == nullptr)
        return error_result("CORE-SEG-0002",
                            "Unbekanntes oder mehrdeutiges Segel-Element", request.element_id);
    if (request.aktion == SegelAktion::Automatisch ||
        request.aktion == SegelAktion::FockVersatzCm) {
        const auto missing = std::find_if(snapshot.diagnosen.begin(), snapshot.diagnosen.end(),
            [&](const SegelDiagnose& diagnostic) {
                return diagnostic.code == "CORE-SEG-0033" &&
                    diagnostic.element_id == request.element_id;
            });
        if (missing != snapshot.diagnosen.end())
            return error_result(missing->code, missing->nachricht, request.element_id);
    }

    try {
        if (!finite(request.translation_m) || !std::isfinite(request.winkel_grad) ||
            !std::isfinite(request.fock_versatz_cm))
            return error_result("CORE-SEG-0003", "Anforderung enthaelt NaN oder Inf",
                                request.element_id);
        switch (request.aktion) {
        case SegelAktion::Automatisch: {
            if (reference->montage_basis_vorgegeben) {
                lage->rotation_xyzw = reference->montage.rotation_xyzw;
                lage->translation_m = reference->montage.translation_m;
                lage->fock_versatz_cm = 0.;
                lage->konfiguriert = true;
                break;
            }
            const bool is_main = reference->art == SegelArt::Grosssegel;
            lage->rotation_xyzw = quaternion_from_matrix(multiply(
                matrix_from_quaternion(rig_rotation(*reference, is_main)),
                transpose(corner_basis(reference->ecken))));
            const auto local_tack = rotate(matrix_from_quaternion(lage->rotation_xyzw),
                                           reference->ecken.hals_m);
            lage->translation_m = subtract(reference->rig_hals_anschlag_m, local_tack);
            lage->fock_versatz_cm = 0.;
            lage->konfiguriert = true;
            break;
        }
        case SegelAktion::Translation:
            if (!translation_in_plane(request.translation_m, request.ebene))
                return error_result("CORE-SEG-0004",
                                    "Translation hat Anteil ausserhalb der Modellachsenebene",
                                    request.element_id);
            lage->translation_m = add(lage->translation_m, request.translation_m);
            lage->konfiguriert = true;
            break;
        case SegelAktion::Rotation: {
            (void)corner(*reference, request.bewegte_ecke);
            const auto pivot_local = request.bewegte_ecke == SegelEcke::Hals ?
                reference->ecken.kopf_vorliek_m : reference->ecken.hals_m;
            const auto pivot_model = transform_point(*lage, pivot_local);
            const auto q_delta = axis_angle(plane_normal(request.ebene),
                                            request.winkel_grad * pi / 180.);
            const auto q_new = quaternion_multiply(q_delta, lage->rotation_xyzw);
            const auto local_fixed = rotate(matrix_from_quaternion(q_new), pivot_local);
            lage->rotation_xyzw = q_new;
            lage->translation_m = subtract(pivot_model, local_fixed);
            lage->konfiguriert = true;
            break;
        }
        case SegelAktion::FockVersatzCm: {
            if (reference->art != SegelArt::Fock)
                return error_result("CORE-SEG-0005",
                                    "Absoluter cm-Versatz ist nur fuer die Fock zulaessig",
                                    request.element_id);
            const auto direction = normalized(reference->fock_versatzrichtung_modell);
            const auto hals = transform_point(*lage, reference->ecken.hals_m);
            const auto current = dot(subtract(hals, reference->fock_grundhals_m), direction);
            lage->translation_m = add(lage->translation_m,
                scale(direction, request.fock_versatz_cm / 100. - current));
            lage->fock_versatz_cm = request.fock_versatz_cm;
            lage->konfiguriert = true;
            break;
        }
        case SegelAktion::OriginalBuglage:
            if (!request.original_buglage)
                return error_result("CORE-SEG-0006",
                                    "Original-Buglage muss explizit angegeben werden",
                                    request.element_id);
            (void)buglage_name(*request.original_buglage);
            lage->original_buglage = *request.original_buglage;
            lage->konfiguriert = true;
            break;
        case SegelAktion::AbsoluteLage:
            if (!request.absolute_lage || request.absolute_lage->element_id != request.element_id)
                return error_result("CORE-SEG-0002",
                                    "Absolute Lage hat keine passende Elementidentitaet",
                                    request.element_id);
            (void)buglage_name(request.absolute_lage->original_buglage);
            lage->rotation_xyzw = request.absolute_lage->rotation_xyzw;
            lage->translation_m = request.absolute_lage->translation_m;
            lage->original_buglage = request.absolute_lage->original_buglage;
            lage->konfiguriert = true;
            break;
        default:
            return error_result("CORE-SEG-0009", "Unbekannte Segelaktion",
                                request.element_id);
        }
        if (!valid_rotation(lage->rotation_xyzw) || !finite(lage->translation_m))
            return error_result("CORE-SEG-0007", "Aktion erzeugt keine gueltige starre Lage",
                                request.element_id);
        if (reference->art == SegelArt::Fock) {
            const auto direction = normalized(reference->fock_versatzrichtung_modell);
            const auto hals = transform_point(*lage, reference->ecken.hals_m);
            lage->fock_versatz_cm = 100. * dot(
                subtract(hals, reference->fock_grundhals_m), direction);
        }
        SegelKonfigurationErgebnis result;
        result.status = SegelErgebnisStatus::Vorschau;
        result.revision = snapshot.revision;
        result.lage = *lage;
        result.meshes = render_meshes(snapshot, *reference, *lage, 0.);
        return result;
    } catch (const std::exception& error) {
        return error_result("CORE-SEG-0008", error.what(), request.element_id);
    }
}


}

std::string sha256_bytes(const std::string& value)
{
    Sha256 digest;
    digest.update(reinterpret_cast<const unsigned char*>(value.data()), value.size());
    return digest.finish();
}

SegelKonfigurationSnapshot leseSegelKonfiguration(
    const Modellkatalog& katalog, const ModelInspection& inspection)
{
    try {
        const auto& info = inspection.info();
        const auto model_file = katalog_datei(katalog, info.model_id, info.model_version);
        const auto root = model_file.parent_path();
        const auto model = read_model_json(model_file);
        std::vector<SegelReferenz> references;
        std::vector<SegelDiagnose> diagnostics;
        for (const auto& group : model.at("gruppen").items())
            for (const auto& element : group.value().at("elemente")) {
                const auto id = element.value("id", std::string{});
                if (group.key() == "mainsail" && id == "Mainsail")
                    references.push_back(sail_reference03(
                        root, model, element, SegelArt::Grosssegel, diagnostics));
                else if (group.key() == "jib" && id == "Jib")
                    references.push_back(sail_reference03(
                        root, model, element, SegelArt::Fock, diagnostics));
            }
        std::vector<std::string> ids;
        for (const auto& reference : references) ids.push_back(reference.element_id);
        std::vector<InspectionMesh> meshes;
        for (auto mesh : getMesh(inspection, "darstellung", ids)) {
            const auto reference = std::find_if(references.begin(), references.end(),
                [&](const SegelReferenz& ref) { return ref.element_id == mesh.element_id; });
            if (reference != references.end()) reference->darstellungs_mesh_id = mesh.id;
            const auto& element = find_model_element(model, mesh.element_id);
            const auto raw = lade_quellmesh(root / std::filesystem::u8path(
                json_string(element.at("Darstellung"), "datei")), std::nullopt, true);
            mesh.points_m = raw.punkte;
            mesh.triangles = raw.dreiecke;
            meshes.push_back(std::move(mesh));
        }
        if (!references.empty() && meshes.empty())
            return snapshot_error("CORE-SEG-0011", "Keine Segelmeshes vorhanden");
        auto snapshot = load_snapshot(model,
                             info.model_id, info.model_version,
                             std::move(references), std::move(meshes));
        snapshot.diagnosen.insert(snapshot.diagnosen.begin(), diagnostics.begin(), diagnostics.end());
        return snapshot;
    } catch (const std::exception& error) {
        const std::string text = error.what();
        if (text.rfind("CORE-SEG-", 0) == 0 && text.size() > 15 && text.substr(13, 2) == ": ")
            return snapshot_error(text.substr(0, 13), text.substr(15));
        return snapshot_error("CORE-SEG-0012", text);
    }
}

SegelKonfigurationErgebnis pruefeSegelAktion(
    const SegelKonfigurationSnapshot& snapshot,
    const SegelAktionAnforderung& anforderung)
{
    return preview_from(snapshot, anforderung);
}

SegelKonfigurationErgebnis leseKonfigurierteSegelgeometrie(
    const SegelKonfigurationSnapshot& snapshot)
{
    if (snapshot.status == SegelKonfigurationsStatus::Fehler)
        return error_result("CORE-SEG-0018", "Konfigurationssnapshot ist ungueltig");
    try {
        SegelKonfigurationErgebnis result;
        result.status = SegelErgebnisStatus::Vorschau;
        result.revision = snapshot.revision;
        for (const auto& reference : snapshot.referenzen) {
            const auto* lage = find_lage(snapshot, reference.element_id);
            if (lage == nullptr)
                return error_result("CORE-SEG-0002",
                                    "Unbekanntes oder mehrdeutiges Segel-Element",
                                    reference.element_id);
            auto meshes = render_meshes(snapshot, reference, *lage, 0.);
            result.meshes.insert(result.meshes.end(),
                                 std::make_move_iterator(meshes.begin()),
                                 std::make_move_iterator(meshes.end()));
        }
        return result;
    } catch (const std::exception& error) {
        return error_result("CORE-SEG-0008", error.what());
    }
}

SegelKonfigurationErgebnis bestaetigeSegelAktion(
    const Modellkatalog& katalog, const ModelInspection& inspection,
    const SegelKonfigurationSnapshot& snapshot,
    const SegelAktionAnforderung& anforderung,
    const std::string& erwartete_revision)
{
    if (snapshot.status == SegelKonfigurationsStatus::Fehler)
        return error_result("CORE-SEG-0013", "Konfigurationssnapshot ist ungueltig");
    if (erwartete_revision.empty() || erwartete_revision != snapshot.revision)
        return error_result("CORE-SEG-0014", "Uebergebene Revision ist nicht die Snapshotrevision");
    const auto preview = preview_from(snapshot, anforderung);
    if (preview.status != SegelErgebnisStatus::Vorschau) return preview;
    try {
        const auto root = model_root(katalog, inspection);
        {
            auto result = preview;
            if (result.status != SegelErgebnisStatus::Vorschau || !result.lage)
                return result;
            const auto& lage = *result.lage;
            std::string tack;
            switch (lage.original_buglage) {
            case SegelBuglage::Unbekannt: tack = "unknown"; break;
            case SegelBuglage::Backbord: tack = "port"; break;
            case SegelBuglage::Steuerbord: tack = "starboard"; break;
            default: throw std::runtime_error("Ungueltige Original-Buglage");
            }
            const Json placement = {{"translation_m", lage.translation_m},
                {"rotation_xyzw", lage.rotation_xyzw}, {"originalTack", tack}};
            result.revision = detail::speichere_segel_placement03(
                root, anforderung.element_id, placement, snapshot.modell_fassung);
            result.status = SegelErgebnisStatus::Gespeichert;
            return result;
        }
    } catch (const std::exception& error) {
        if (std::string(error.what()).rfind("CORE-KAT-0004", 0) == 0)
            return error_result("CORE-SEG-0016", error.what(), anforderung.element_id);
        return error_result("CORE-SEG-0017", error.what(), anforderung.element_id);
    }
}

SegelKonfigurationErgebnis stelleSegelStellwinkelVorschauDar(
    const SegelKonfigurationSnapshot& snapshot, const std::string& element_id,
    double stellwinkel_grad)
{
    if (snapshot.status == SegelKonfigurationsStatus::Fehler)
        return error_result("CORE-SEG-0018", "Konfigurationssnapshot ist ungueltig");
    if (!std::isfinite(stellwinkel_grad))
        return error_result("CORE-SEG-0003", "Stellwinkel ist nicht endlich", element_id);
    const auto* reference = find_reference(snapshot, element_id);
    const auto* lage = find_lage(snapshot, element_id);
    if (reference == nullptr || lage == nullptr)
        return error_result("CORE-SEG-0002", "Unbekanntes oder mehrdeutiges Segel-Element",
                            element_id);
    try {
        SegelKonfigurationErgebnis result;
        result.status = SegelErgebnisStatus::Vorschau;
        result.revision = snapshot.revision;
        result.lage = *lage;
        result.meshes = render_meshes(snapshot, *reference, *lage, stellwinkel_grad);
        return result;
    } catch (const std::exception& error) {
        return error_result("CORE-SEG-0008", error.what(), element_id);
    }
}

}
