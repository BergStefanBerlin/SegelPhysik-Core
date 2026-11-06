#include "segelphysik/segelkonfigurator.hpp"
#include "modell_intern.hpp"

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
constexpr double angle_tolerance = 1e-6;
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

struct SourceFiles {
    std::string source_path;
    std::string display_path;
    std::string lookup_path;
};

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

Json read_json(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Datei nicht lesbar: " + path.filename().u8string());
    Json value;
    input >> value;
    if (!input.good() && !input.eof()) throw std::runtime_error("JSON-Lesefehler");
    return value;
}

std::vector<Vektor3> read_obj_vertices(const std::filesystem::path& path)
{
    std::ifstream input(path);
    if (!input) throw std::runtime_error("Riggeometrie nicht lesbar: " + path.filename().u8string());
    std::vector<Vektor3> vertices;
    std::string line;
    while (std::getline(input, line)) {
        std::istringstream row(line);
        std::string kind;
        row >> kind;
        if (kind != "v") continue;
        Vektor3 vertex{};
        if (!(row >> vertex[0] >> vertex[1] >> vertex[2]) || !finite(vertex))
            throw std::runtime_error("Ungueltiger Vertex in Riggeometrie: " +
                                     path.filename().u8string());
        vertices.push_back(vertex);
    }
    if (input.bad()) throw std::runtime_error("Lesefehler in Riggeometrie");
    return vertices;
}

Vektor3 vertex_section_center(const std::vector<Vektor3>& vertices,
                              std::size_t first, std::size_t count)
{
    if (count == 0 || first > vertices.size() || count > vertices.size() - first)
        throw std::runtime_error("Semantischer Rigg-Querschnitt fehlt");
    Vektor3 center{};
    for (std::size_t i = first; i < first + count; ++i)
        center = add(center, vertices[i]);
    return scale(center, 1. / static_cast<double>(count));
}

struct Mastachse {
    Vektor3 punkt_m;
    Vektor3 richtung;
};

std::vector<Vektor3> horizontal_section_centers(const std::vector<Vektor3>& vertices)
{
    std::map<double, std::vector<Vektor3>> sections;
    for (const auto& vertex : vertices) sections[vertex[2]].push_back(vertex);
    std::vector<Vektor3> centers;
    for (const auto& section : sections) {
        if (section.second.size() < 3)
            throw std::runtime_error("Keine belegten horizontalen Riggquerschnitte");
        const auto center = vertex_section_center(section.second, 0, section.second.size());
        double radius = 0.;
        for (const auto& point : section.second)
            radius = std::max(radius, norm(subtract(point, center)));
        if (!(radius > 1e-8))
            throw std::runtime_error("Entarteter Riggquerschnitt");
        centers.push_back(center);
    }
    if (centers.size() < 2) throw std::runtime_error("Riggachse hat zu wenige Querschnitte");
    return centers;
}

Mastachse mastachse_bei_z(const std::vector<Vektor3>& vertices, double z)
{
    const auto centers = horizontal_section_centers(vertices);
    for (std::size_t i = 0; i + 1 < centers.size(); ++i) {
        const double lower = centers[i][2];
        const double upper = centers[i + 1][2];
        if (z < lower || z > upper) continue;
        const double span = upper - lower;
        if (!(span > 1e-9)) throw std::runtime_error("Mastquerschnitte nicht aufsteigend");
        const double fraction = (z - lower) / span;
        return {add(centers[i], scale(subtract(centers[i + 1], centers[i]), fraction)),
                subtract(centers[i + 1], centers[i])};
    }
    throw std::runtime_error("Mastachse reicht nicht bis zur Baumanschlaghoehe");
}

struct Riggmetadaten {
    Vektor3 grosssegel_hals_m;
    Vektor3 mastachse_modell;
    Vektor3 baumrichtung_modell;
    Vektor3 fock_hals_m;
    Vektor3 vorstagachse_modell;
};

const Json& find_model_element(const Json& model, const std::string& id);
std::string json_string(const Json& object, const char* field);
Vektor3 obj_reference_point(const std::vector<Vektor3>& points, const Json& indices);

Riggmetadaten riggreferenzen(const std::filesystem::path& root, const Json& model)
{
    const auto rig_mesh_path = [&](const std::string& element_id) {
        const auto& element = find_model_element(model, element_id);
        if (!element.contains("Darstellung") || !element["Darstellung"].is_object())
            throw std::runtime_error("Rigg-Darstellungsreferenz fehlt: " + element_id);
        const auto relative = std::filesystem::u8path(
            json_string(element["Darstellung"], "datei"));
        return root / relative;
    };
    const auto mounted_vertices = [&](const std::string& id) {
        auto points = read_obj_vertices(rig_mesh_path(id));
        const auto* setting = element_setting(model, id);
        if (setting && setting->at("konfiguriert").get<bool>()) {
            const auto pose = setting_pose(*setting);
            for (auto& point : points) point = pose.modellpunkt_nach_welt(point);
        }
        return points;
    };
    const auto mast = read_obj_vertices(rig_mesh_path("Mast"));
    const auto* mast_setting = element_setting(model, "Mast");
    const auto mast_pose = mast_setting && mast_setting->at("konfiguriert").get<bool>() ?
        setting_pose(*mast_setting) : Pose({0., 0., 0.}, {0., 0., 0., 1.});
    const auto boom = mounted_vertices("Großbaum");
    const auto stay = mounted_vertices("Vorstag");
    const auto& boom_mount = find_model_element(model, "Großbaum").at("Montage");
    const auto boom_cap_a = obj_reference_point(boom, boom_mount.at("anschlag_vertex_indices"));
    const auto boom_cap_b = obj_reference_point(boom, boom_mount.at("ende_vertex_indices"));
    const auto boom_midpoint = scale(add(boom_cap_a, boom_cap_b), 0.5);
    const auto local_midpoint = mast_pose.weltpunkt_nach_modell(boom_midpoint);
    const auto local_mast_axis = mastachse_bei_z(mast, local_midpoint[2]);
    const Mastachse mast_axis{
        mast_pose.modellpunkt_nach_welt(local_mast_axis.punkt_m),
        mast_pose.modellvektor_nach_welt(local_mast_axis.richtung)};
    const auto boom_near = norm(subtract(boom_cap_a, mast_axis.punkt_m)) <
        norm(subtract(boom_cap_b, mast_axis.punkt_m)) ? boom_cap_a : boom_cap_b;
    const auto boom_far = boom_near == boom_cap_a ? boom_cap_b : boom_cap_a;
    const auto stay_centers = horizontal_section_centers(stay);
    const auto deck_center = stay_centers.front();
    const auto next_stay_center = stay_centers[1];
    return {mast_axis.punkt_m, mast_axis.richtung, subtract(boom_far, boom_near),
            deck_center, subtract(next_stay_center, deck_center)};
}

std::string combine_fingerprints(const std::filesystem::path& root,
                                 const SourceFiles& entry, const Json& model)
{
    std::ostringstream content;
    for (const auto& relative : {entry.source_path, entry.display_path, entry.lookup_path,
                                 json_string(find_model_element(model, "Mast").at("Darstellung"), "datei"),
                                 json_string(find_model_element(model, "Großbaum").at("Darstellung"), "datei"),
                                 json_string(find_model_element(model, "Vorstag").at("Darstellung"), "datei")}) {
        content << relative << ':' << sha256(root / std::filesystem::u8path(relative)) << '\n';
    }
    Sha256 digest;
    const auto text = content.str();
    digest.update(reinterpret_cast<const unsigned char*>(text.data()), text.size());
    return digest.finish();
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

Vektor3 obj_reference_point(const std::vector<Vektor3>& points, const Json& indices)
{
    if (!indices.is_array() || indices.empty())
        throw std::runtime_error("OBJ-Montagereferenz braucht benannte Vertex-Indizes");
    Vektor3 result{};
    for (const auto& index : indices) {
        if (!index.is_number_unsigned() || index.get<std::size_t>() == 0 ||
            index.get<std::size_t>() > points.size())
            throw std::runtime_error("OBJ-Montagereferenz liegt ausserhalb der Quelldatei");
        result = add(result, points.at(index.get<std::size_t>() - 1));
    }
    return scale(result, 1. / static_cast<double>(indices.size()));
}

SegelLage neutral_obj_lage(const SegelReferenz& reference)
{
    const auto foot = subtract(reference.ecken.schothorn_m, reference.ecken.hals_m);
    const auto z = normalized(subtract(reference.ecken.kopf_vorliek_m, reference.ecken.hals_m));
    const auto x = scale(normalized(subtract(foot, scale(z, dot(foot, z)))), -1.);
    const auto y = normalized(cross(z, x));
    const auto tilt = std::hypot(z[0], z[1]);
    const auto target_z = normalized(Vektor3{z[0] < 0. ? -tilt : tilt, 0., z[2]});
    const Vektor3 forward{1., 0., 0.};
    const auto target_x = normalized(subtract(forward, scale(target_z, dot(forward, target_z))));
    const auto target_y = normalized(cross(target_z, target_x));
    const Matrix source_transpose{x[0], x[1], x[2], y[0], y[1], y[2], z[0], z[1], z[2]};
    const Matrix target_basis{target_x[0], target_y[0], target_z[0],
                        target_x[1], target_y[1], target_z[1],
                        target_x[2], target_y[2], target_z[2]};
    SegelLage result;
    result.element_id = reference.element_id;
    result.rotation_xyzw = quaternion_from_matrix(multiply(target_basis, source_transpose));
    auto target = reference.ecken.hals_m;
    target[1] = 0.;
    result.translation_m = subtract(target,
        rotate(matrix_from_quaternion(result.rotation_xyzw), reference.ecken.hals_m));
    result.konfiguriert = true;
    return result;
}

Vektor3 find_contour_point(const Json& source, const std::string& point_id)
{
    const auto& points = source.at("grundkontur").at("punkte");
    if (!points.is_array()) throw std::runtime_error("Segelkontur fehlt");
    std::optional<Vektor3> found;
    for (const auto& point : points) {
        if (point.value("id", std::string{}) == point_id) {
            if (found) throw std::runtime_error("Doppelter semantischer Segelpunkt: " + point_id);
            found = json_vector(point.at("position_m"));
        }
    }
    if (!found) throw std::runtime_error("Semantischer Segelpunkt fehlt: " + point_id);
    return *found;
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

SegelEcken indexed_corners(const std::vector<Vektor3>& vertices, const Json& mounting)
{
    SegelEcken corners;
    corners.hals_m = obj_reference_point(vertices, mounting.at("hals_vertex_indices"));
    corners.schothorn_m = obj_reference_point(vertices, mounting.at("schothorn_vertex_indices"));
    corners.kopf_vorliek_m = obj_reference_point(vertices, mounting.at("kopf_vorliek_vertex_indices"));
    if (mounting.contains("kopf_achterliek_vertex_indices"))
        corners.kopf_achterliek_m = obj_reference_point(vertices, mounting.at("kopf_achterliek_vertex_indices"));
    (void)corner_basis(corners);
    return corners;
}

SegelEcken semantic_corners(const Json& source)
{
    SegelEcken corners;
    corners.hals_m = find_contour_point(source, "hals");
    corners.schothorn_m = find_contour_point(source, "schothorn");
    bool found = false;
    for (const auto& edge : source.at("grundkontur").at("rand").at("kanten")) {
        if (edge.value("name", std::string{}) != "vorliek") continue;
        if (found) throw std::runtime_error("Mehrdeutige semantische Vorliekreferenz");
        const auto ids = edge.at("punkt_ids").get<std::vector<std::string>>();
        if (ids.size() < 2 || (ids.front() != "hals" && ids.back() != "hals"))
            throw std::runtime_error("Vorliek muss am semantischen Hals enden");
        corners.kopf_vorliek_m = find_contour_point(source,
            ids.front() == "hals" ? ids.back() : ids.front());
        found = true;
    }
    if (!found) throw std::runtime_error("Semantische Vorliekreferenz fehlt");
    for (const auto& edge : source.at("grundkontur").at("rand").at("kanten")) {
        if (edge.value("name", std::string{}) != "kopf") continue;
        const auto ids = edge.at("punkt_ids").get<std::vector<std::string>>();
        if (ids.size() != 2) throw std::runtime_error("Mehrdeutige semantische Kopfreferenz");
        for (const auto& id : ids) {
            const auto point = find_contour_point(source, id);
            if (norm(subtract(point, corners.kopf_vorliek_m)) > 1e-9)
                corners.kopf_achterliek_m = point;
        }
    }
    (void)corner_basis(corners);
    return corners;
}

QuaternionXyzw rig_rotation(const SegelReferenz& reference, bool main_sail);

SegelReferenz sail_reference(const std::filesystem::path& root, const Json& model,
                             const Json& element, std::vector<SegelDiagnose>& diagnostics)
{
    const auto id = json_string(element, "id");
    const auto display = json_string(element.at("Darstellung"), "datei");
    const auto source_path = json_string(element.at("Segel"), "quelle_datei");
    const auto source = read_json(root / std::filesystem::u8path(source_path));
    const auto vertices = read_obj_vertices(root / std::filesystem::u8path(display));
    if (element.contains("setting") && element.at("setting").contains("montage_referenz")) {
        const auto& stored_reference = element.at("setting").at("montage_referenz");
        if (stored_reference.value("obj_sha256", std::string{}) !=
                sha256(root / std::filesystem::u8path(display)) ||
            stored_reference.value("kontur_sha256", std::string{}) !=
                sha256(root / std::filesystem::u8path(source_path)))
            throw std::runtime_error("CORE-SEG-0031: Bestaetigte Montagequelle geaendert; explizite Neuzuordnung erforderlich: " + id);
    }
    SegelReferenz result;
    result.element_id = id;
    result.darstellungs_mesh_id = display;
    result.intrinsische_quelle_id = json_string(source.at("segel"), "id");
    const auto type = json_string(source.at("segel"), "typ");
    if (type != "grosssegel" && type != "fock")
        throw std::runtime_error("Unbekannter fachlicher Segeltyp: " + type);
    result.art = type == "fock" ? SegelArt::Fock : SegelArt::Grosssegel;
    if (!element.at("Segel").contains("lookup_datei"))
        diagnostics.push_back({"CORE-SEG-0030",
            "Darstellung ohne Lookup; intrinsische Rekonstruktion ersetzt keine physische "
            "Bestandsquelle und erteilt keine Physikfreigabe.", id});
    result.ecken = semantic_corners(source);
    const auto point_present = [&](const Vektor3& point) {
        return std::any_of(vertices.begin(), vertices.end(),
            [&](const Vektor3& vertex) { return norm(subtract(vertex, point)) <= 1e-7; });
    };
    std::ifstream obj(root / std::filesystem::u8path(display));
    std::string first_line;
    std::getline(obj, first_line);
    const auto declaration = "# Source: " + std::filesystem::u8path(source_path).filename().u8string() + ";";
    const bool semantic_obj = first_line.rfind(declaration, 0) == 0 &&
        point_present(result.ecken.hals_m) && point_present(result.ecken.kopf_vorliek_m);
    std::string fingerprint_text = display + ":" + sha256(root / std::filesystem::u8path(display)) +
        "\n" + source_path + ":" + sha256(root / std::filesystem::u8path(source_path)) + "\n";
    bool direct_montage = false;

    if (element.contains("Montage")) {
        const auto& mounting = element.at("Montage");
        if (mounting.value("schema_version", 0) != 1 ||
            mounting.value("basis", std::string{}) != "mittschiffs")
            throw std::runtime_error("Ungueltige Montagebasis: " + id);
        fingerprint_text += mounting.dump();
        if (mounting.value("bezug", std::string{}) == "kontur") {
            if (!semantic_obj) throw std::runtime_error("Kontur-/OBJ-Quellbezug nicht belegt: " + id);
            direct_montage = true;
        } else if (mounting.value("bezug", std::string{}) == "obj") {
            const auto current_hash = sha256(root / std::filesystem::u8path(display));
            const bool checked = mounting.value("quellen_sha256", std::string{}) == current_hash;
            if (checked) {
                result.ecken = indexed_corners(vertices, mounting);
                direct_montage = true;
            }
            else {
                if (!semantic_obj)
                    throw std::runtime_error("CORE-SEG-0031: Veraltete oder ungepruefte OBJ-Montage; semantischer Quellbezug fehlt: " + id);
                if (mounting.contains("quellen_sha256"))
                    throw std::runtime_error("CORE-SEG-0031: OBJ-Quellpruefsumme stimmt nicht; explizite Neuzuordnung erforderlich: " + id);
                const auto mass_path = element.contains("Hydrostatik") &&
                    element.at("Hydrostatik").contains("masse") &&
                    element.at("Hydrostatik").at("masse").contains("datei") ?
                    json_string(element.at("Hydrostatik").at("masse"), "datei") :
                    (std::filesystem::u8path(display).parent_path().parent_path() /
                        "masse" / std::filesystem::u8path(display).filename()).generic_u8string();
                if (!std::filesystem::is_regular_file(root / std::filesystem::u8path(mass_path)))
                    throw std::runtime_error("CORE-SEG-0031: Legacy-Montage ohne belegte Originalgeometrie: " + id);
                const auto legacy = indexed_corners(
                    read_obj_vertices(root / std::filesystem::u8path(mass_path)), mounting);
                MontageReferenz adaptation;
                adaptation.element_id = id;
                const auto rotation = multiply(corner_basis(legacy), transpose(corner_basis(result.ecken)));
                adaptation.rotation_xyzw = quaternion_from_matrix(rotation);
                adaptation.translation_m = subtract(legacy.hals_m, rotate(rotation, result.ecken.hals_m));
                adaptation.herkunft = "Legacy-OBJ-Montage aus referenzierter Massenquelle; Kontur ohne Skalierung";
                result.legacy_montage = adaptation;
                fingerprint_text += "\n" + mass_path + ":" + sha256(root / std::filesystem::u8path(mass_path));
                auto legacy_reference = result;
                legacy_reference.ecken = legacy;
                const auto neutral = neutral_obj_lage(legacy_reference);
                const auto mounted = multiply(matrix_from_quaternion(neutral.rotation_xyzw), rotation);
                result.montage.rotation_xyzw = quaternion_from_matrix(mounted);
                result.montage.translation_m = add(
                    rotate(matrix_from_quaternion(neutral.rotation_xyzw), adaptation.translation_m),
                    neutral.translation_m);
                diagnostics.push_back({"CORE-SEG-0031",
                    "Veraltete ungepruefte Vertexreferenzen verworfen; semantische aktuelle Kontur "
                    "starr am belegten Legacy-Hals/Vorliek montiert. Physische Altgeometrie unveraendert.", id});
            }
        } else throw std::runtime_error("Unbekannter Montagebezug: " + id);
    } else if (!semantic_obj) {
        throw std::runtime_error("Semantischer Kontur-/OBJ-Bezug fehlt: " + id);
    }
    result.vorliek_richtung_lokal = subtract(result.ecken.kopf_vorliek_m, result.ecken.hals_m);
    result.unterliek_richtung_lokal = subtract(result.ecken.schothorn_m, result.ecken.hals_m);
    if (element.at("Segel").contains("lookup_datei")) {
        const auto lookup_path = json_string(element.at("Segel"), "lookup_datei");
        const auto lookup = read_json(root / std::filesystem::u8path(lookup_path));
        result.lookup_id = json_string(lookup, "segel_id");
        if (result.lookup_id != result.intrinsische_quelle_id ||
            lookup.value("status", std::string{}) != "erfolgreich")
            throw std::runtime_error("Lookup-/Segelidentitaet stimmt nicht: " + id);
        fingerprint_text += "\n" + lookup_path + ":" + sha256(root / std::filesystem::u8path(lookup_path));
    }
    if (!result.legacy_montage && !direct_montage && element.at("Segel").contains("lookup_datei")) {
        const auto rig = riggreferenzen(root, model);
        result.rig_hals_anschlag_m = result.art == SegelArt::Grosssegel ?
            rig.grosssegel_hals_m : rig.fock_hals_m;
        result.rig_vorliek_richtung_modell = result.art == SegelArt::Grosssegel ?
            rig.mastachse_modell : rig.vorstagachse_modell;
        result.rig_unterliek_richtung_modell = rig.baumrichtung_modell;
        SourceFiles mapping{};
        mapping.source_path = source_path;
        mapping.display_path = display;
        mapping.lookup_path = json_string(element.at("Segel"), "lookup_datei");
        result.quellen_fingerprint = combine_fingerprints(root, mapping, model);
    } else {
        if (!result.legacy_montage) {
            const auto neutral = neutral_obj_lage(result);
            result.montage.rotation_xyzw = neutral.rotation_xyzw;
            result.montage.translation_m = neutral.translation_m;
            if (element.contains("Montage") && element.at("Montage").contains("translation_m")) {
                result.montage.translation_m = json_vector(element.at("Montage").at("translation_m"));
                result.montage.rotation_xyzw = element.at("Montage").at("rotation_xyzw").get<QuaternionXyzw>();
                if (!valid_rotation(result.montage.rotation_xyzw))
                    throw std::runtime_error("Ungueltige deklarierte Montagepose: " + id);
            }
        }
        result.rig_hals_anschlag_m = add(
            rotate(matrix_from_quaternion(result.montage.rotation_xyzw), result.ecken.hals_m),
            result.montage.translation_m);
        result.rig_vorliek_richtung_modell = rotate(
            matrix_from_quaternion(result.montage.rotation_xyzw), result.vorliek_richtung_lokal);
        result.rig_unterliek_richtung_modell = rotate(
            matrix_from_quaternion(result.montage.rotation_xyzw), result.unterliek_richtung_lokal);
        Sha256 hash;
        hash.update(reinterpret_cast<const unsigned char*>(fingerprint_text.data()), fingerprint_text.size());
        result.quellen_fingerprint = hash.finish();
    }
    result.fock_grundhals_m = result.rig_hals_anschlag_m;
    result.fock_versatzrichtung_modell = result.rig_vorliek_richtung_modell;
    result.montage.element_id = id;
    result.montage.bezugspunkt_lokal_m = result.ecken.hals_m;
    result.montage.laengsrichtung_lokal = result.vorliek_richtung_lokal;
    result.montage.querrichtung_lokal = result.unterliek_richtung_lokal;
    result.montage.herkunft = semantic_obj ? "Semantische Segelkontur mit OBJ-Quelldeklaration" : "SHA256-gepruefte OBJ-Vertexreferenzen";
    result.montage.quellen_fingerprint = result.quellen_fingerprint;
    result.montage_basis_vorgegeben = result.legacy_montage.has_value() ||
        direct_montage || result.lookup_id.empty();
    if (!result.montage_basis_vorgegeben) {
        result.montage.rotation_xyzw = quaternion_from_matrix(multiply(
            matrix_from_quaternion(rig_rotation(result, result.art == SegelArt::Grosssegel)),
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
    // Montagebasis is always amidships, independent of an opened raw boom.
    const double horizontal = std::hypot(under[0], under[1]);
    if (!(horizontal > 1e-12)) throw std::runtime_error("Unterliek hat keine Mittschiffsrichtung");
    under = {-horizontal, 0., under[2]};
    const auto under_direction = normalized(under);
    if (std::abs(dot(z, under_direction)) > angle_tolerance)
        throw std::runtime_error("Vorliek und Unterliek koennen nicht starr ausgerichtet werden");
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

Json lage_json(const SegelLage& lage, const SegelReferenz& reference)
{
    return Json{
        {"rotation_xyzw", lage.rotation_xyzw},
        {"translation_m", lage.translation_m},
        {"quellen_fingerprint", reference.quellen_fingerprint},
        {"original_buglage", buglage_name(lage.original_buglage)},
        {"konfiguriert", lage.konfiguriert}};
}

Json config_json(const SegelKonfigurationSnapshot& snapshot, std::uint64_t revision,
                 Json config)
{
    if (config.is_null()) config = Json::object();
    config["schema_version"] = 1;
    config["modell_id"] = snapshot.modell_id;
    config["modell_fassung"] = snapshot.modell_fassung;
    config["quellen_fingerprint"] = snapshot.quellen_fingerprint;
    config["revision"] = revision;
    return config;
}

std::uint64_t parse_revision(const std::string& value)
{
    if (value.empty() || !std::all_of(value.begin(), value.end(),
                                      [](unsigned char ch) { return ch >= '0' && ch <= '9'; }))
        throw std::runtime_error("Ungueltige Konfigurationsrevision");
    return std::stoull(value);
}

class ConfigurationLock {
public:
    explicit ConfigurationLock(const std::filesystem::path& path)
    {
#ifdef _WIN32
        handle_ = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                              OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (handle_ == INVALID_HANDLE_VALUE)
            throw std::runtime_error("Konfiguration ist durch einen anderen Prozess gesperrt");
#else
        descriptor_ = ::open(path.c_str(), O_CREAT | O_RDWR, 0600);
        if (descriptor_ < 0 || ::flock(descriptor_, LOCK_EX | LOCK_NB) != 0) {
            if (descriptor_ >= 0) ::close(descriptor_);
            descriptor_ = -1;
            throw std::runtime_error("Konfiguration ist durch einen anderen Prozess gesperrt");
        }
#endif
    }

    ~ConfigurationLock()
    {
#ifdef _WIN32
        if (handle_ != INVALID_HANDLE_VALUE) CloseHandle(handle_);
#else
        if (descriptor_ >= 0) {
            ::flock(descriptor_, LOCK_UN);
            ::close(descriptor_);
        }
#endif
    }

    ConfigurationLock(const ConfigurationLock&) = delete;
    ConfigurationLock& operator=(const ConfigurationLock&) = delete;

private:
#ifdef _WIN32
    HANDLE handle_ = INVALID_HANDLE_VALUE;
#else
    int descriptor_ = -1;
#endif
};

void atomic_write(const std::filesystem::path& target, const std::string& contents)
{
    static std::atomic<std::uint64_t> sequence{0};
    const auto temporary = target.parent_path() /
        (target.filename().u8string() + ".tmp." +
         std::to_string(sequence.fetch_add(1, std::memory_order_relaxed)));
    try {
        {
            std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
            if (!output) throw std::runtime_error("Temporare Konfigurationsdatei nicht erstellbar");
            output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
            output.flush();
            if (!output) throw std::runtime_error("Konfigurationsdatei nicht vollstaendig schreibbar");
        }
#ifdef _WIN32
        const auto temporary_handle = CreateFileW(
            temporary.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (temporary_handle == INVALID_HANDLE_VALUE)
            throw std::runtime_error("Temporare Konfigurationsdatei nicht erneut oeffnbar");
        const bool flushed = FlushFileBuffers(temporary_handle) != 0;
        CloseHandle(temporary_handle);
        if (!flushed) throw std::runtime_error("Konfigurationsdaten nicht dauerhaft schreibbar");
        if (!MoveFileExW(temporary.c_str(), target.c_str(),
                         MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            throw std::runtime_error("Atomare Konfigurationsdatei-Ersetzung fehlgeschlagen");
#else
        const int file_descriptor = ::open(temporary.c_str(), O_RDWR);
        if (file_descriptor < 0 || ::fsync(file_descriptor) != 0) {
            if (file_descriptor >= 0) ::close(file_descriptor);
            throw std::runtime_error("Konfigurationsdaten nicht dauerhaft schreibbar");
        }
        ::close(file_descriptor);
        std::error_code error;
        std::filesystem::rename(temporary, target, error);
        if (error) throw std::runtime_error("Atomare Konfigurationsdatei-Ersetzung fehlgeschlagen");
        const int directory_descriptor = ::open(target.parent_path().c_str(), O_RDONLY);
        if (directory_descriptor >= 0) {
            ::fsync(directory_descriptor);
            ::close(directory_descriptor);
        }
#endif
    } catch (const std::exception&) {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        throw;
    }
}

SegelKonfigurationSnapshot load_snapshot(const std::filesystem::path& root,
                                         const std::string& model_id,
                                         const std::string& model_version,
                                         std::vector<SegelReferenz> references,
                                         std::vector<InspectionMesh> meshes)
{
    SegelKonfigurationSnapshot result;
    result.modell_id = model_id;
    result.modell_fassung = model_version;
    result.referenzen = std::move(references);
    result.grundmeshes = std::move(meshes);
    result.quellen_fingerprint = whole_source_fingerprint(result.referenzen);
    for (const auto& reference : result.referenzen) {
        SegelLage lage;
        lage.element_id = reference.element_id;
        lage.original_buglage = reference.original_buglage;
        if (reference.art == SegelArt::Fock) {
            const auto direction = normalized(reference.fock_versatzrichtung_modell);
            lage.fock_versatz_cm = 100. * dot(
                subtract(reference.ecken.hals_m, reference.fock_grundhals_m), direction);
        }
        result.lagen.push_back(lage);
    }

    const auto model = read_json(root / "bootsmodell.json");
    validiere_element_settings(model);
    if (std::filesystem::exists(root / "segelkonfiguration.json"))
        throw std::runtime_error("Legacy-Segelkonfiguration vorhanden; explizite Migration erforderlich");
    if (!model.contains("konfiguration")) {
        for (const auto& reference : result.referenzen) {
            const auto* setting = element_setting(model, reference.element_id);
            if (setting && setting->at("konfiguriert").get<bool>())
                throw std::runtime_error("Bestaetigte Segel-setting ohne Konfigurationsmetadaten");
        }
        result.status = SegelKonfigurationsStatus::NichtKonfiguriert;
        return result;
    }

    const auto& config = model.at("konfiguration");
    if (!config.is_object() || config.value("schema_version", 0) != 1 ||
        !config.contains("revision") || !config["revision"].is_number_unsigned() ||
        config.contains("elemente"))
        throw std::runtime_error("Konfigurationsschema ungueltig");
    result.revision = std::to_string(config["revision"].get<std::uint64_t>());
    bool confirmed = false;
    for (const auto& reference : result.referenzen) {
        const auto* setting = element_setting(model, reference.element_id);
        if (setting && setting->at("konfiguriert").get<bool>()) confirmed = true;
    }
    const bool unbound_empty = !confirmed && result.revision == "0" &&
        !config.contains("modell_id") && !config.contains("modell_fassung") &&
        !config.contains("quellen_fingerprint");
    const bool legacy_adaptation = std::any_of(result.referenzen.begin(), result.referenzen.end(),
        [&](const SegelReferenz& ref) {
            const auto* setting = element_setting(model, ref.element_id);
            return ref.legacy_montage && setting && !setting->contains("montage_referenz") &&
                setting->at("konfiguriert").get<bool>() &&
                setting->value("quellen_fingerprint", std::string{}) != ref.quellen_fingerprint;
        });
    if (!unbound_empty && config.value("modell_id", std::string{}) != model_id)
        throw std::runtime_error("Konfiguration gehoert zu anderem Modell");
    if (!unbound_empty && (
        config.value("modell_fassung", std::string{}) != model_version ||
        config.value("quellen_fingerprint", std::string{}) != result.quellen_fingerprint) &&
        !legacy_adaptation)
        throw std::runtime_error("Konfiguration gehoert zu anderer Modell-/Quellenfassung");

    for (const auto& ref : result.referenzen) {
        const auto id = ref.element_id;
        const auto* setting = element_setting(model, id);
        if (!setting) continue;
        const auto& stored = *setting;
        if (!stored.at("konfiguriert").get<bool>()) continue;
        if (!stored.is_object() || !stored.contains("konfiguriert") ||
            !stored["konfiguriert"].is_boolean())
            throw std::runtime_error("Ungueltige Elementkonfiguration");
        const auto rotation = stored.at("rotation_xyzw");
        const auto translation = stored.at("translation_m");
        if (!rotation.is_array() || rotation.size() != 4 ||
            !translation.is_array() || translation.size() != 3)
            throw std::runtime_error("Ungueltige starre Elementlage");
        const auto q = rotation.get<QuaternionXyzw>();
        const auto t = translation.get<Vektor3>();
        if (!valid_rotation(q) || !finite(t))
            throw std::runtime_error("Ungueltige starre Elementlage");
        const auto* reference = find_reference(result, id);
        auto* lage = find_lage(result, id);
        if (reference == nullptr || lage == nullptr)
            throw std::runtime_error("Fremde oder mehrdeutige Segel-ID in Konfiguration");
        const bool adapt_legacy = reference->legacy_montage &&
            !stored.contains("montage_referenz") &&
            stored.value("quellen_fingerprint", std::string{}) != reference->quellen_fingerprint;
        if (stored.at("konfiguriert").get<bool>() &&
            stored.value("quellen_fingerprint", std::string{}) != reference->quellen_fingerprint &&
            !adapt_legacy)
            throw std::runtime_error("Veraltete Quellzuordnung in Konfiguration");
        if (!rotation.is_array() || rotation.size() != 4 ||
            !translation.is_array() || translation.size() != 3)
            throw std::runtime_error("Ungueltige starre Segellage");
        for (std::size_t i = 0; i < 4; ++i) lage->rotation_xyzw[i] = rotation.at(i).get<double>();
        for (std::size_t i = 0; i < 3; ++i) lage->translation_m[i] = translation.at(i).get<double>();
        if (!valid_rotation(lage->rotation_xyzw) || !finite(lage->translation_m))
            throw std::runtime_error("Ungueltige starre Segellage");
        lage->original_buglage = stored.contains("original_buglage") ?
            parse_buglage(stored.at("original_buglage").get<std::string>()) :
            SegelBuglage::Unbekannt;
        lage->konfiguriert = stored.value("konfiguriert", true);
        if (adapt_legacy) {
            const auto& adaptation = *reference->legacy_montage;
            lage->translation_m = add(rotate(matrix_from_quaternion(lage->rotation_xyzw),
                adaptation.translation_m), lage->translation_m);
            lage->rotation_xyzw = quaternion_multiply(lage->rotation_xyzw,
                adaptation.rotation_xyzw);
            result.diagnosen.push_back({"CORE-SEG-0032",
                "Bestehende Lage starr auf belegte aktuelle Konturbasis umgerechnet; "
                "Halsposition und gerichtete Vorliekachse bleiben erhalten, keine Ruecksetzung.", id});
        }
        if (reference->art == SegelArt::Fock) {
            const auto direction = normalized(reference->fock_versatzrichtung_modell);
            const auto hals = transform_point(*lage, reference->ecken.hals_m);
            const double derived = 100. * dot(
                subtract(hals, reference->fock_grundhals_m), direction);
            lage->fock_versatz_cm = derived;
        } else {
            lage->fock_versatz_cm = 0.;
        }
    }
    result.status = confirmed ? SegelKonfigurationsStatus::Bereit :
        SegelKonfigurationsStatus::NichtKonfiguriert;
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

Json read_config_json(const std::filesystem::path& path)
{
    const auto model = read_json(path);
    return model.contains("konfiguration") ? model.at("konfiguration") : Json();
}

}

SegelKonfigurationSnapshot leseSegelKonfiguration(
    const Modellkatalog& katalog, const ModelInspection& inspection)
{
    try {
        const auto& info = inspection.info();
        const auto root = model_root(katalog, inspection);
        const auto model = read_json(root / "bootsmodell.json");
        if (modell_fassung(root / "bootsmodell.json") != info.model_version)
            throw std::runtime_error("Modell-/Quellenstand passt nicht zum Inspektionssnapshot");
        std::vector<SegelReferenz> references;
        std::vector<SegelDiagnose> diagnostics;
        for (const auto& group : model.at("gruppen"))
            for (const auto& element : group.at("elemente"))
                if (element.contains("Segel"))
                    references.push_back(sail_reference(root, model, element, diagnostics));
        std::vector<std::string> ids;
        for (const auto& reference : references) ids.push_back(reference.element_id);
        std::vector<InspectionMesh> meshes;
        for (auto mesh : getMesh(inspection, "darstellung", ids)) {
            const auto reference = std::find_if(references.begin(), references.end(),
                [&](const SegelReferenz& ref) { return ref.element_id == mesh.element_id; });
            if (reference != references.end()) reference->darstellungs_mesh_id = mesh.id;
            const auto& element = find_model_element(model, mesh.element_id);
            const auto raw = lade_quellmesh(
                root / std::filesystem::u8path(json_string(element.at("Darstellung"), "datei")),
                std::nullopt, true);
            mesh.points_m = raw.punkte;
            mesh.triangles = raw.dreiecke;
            meshes.push_back(std::move(mesh));
        }
        if (!references.empty() && meshes.empty())
            return snapshot_error("CORE-SEG-0011", "Keine Segelmeshes vorhanden");
        auto snapshot = load_snapshot(root, info.model_id, info.model_version,
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
        const auto path = root / "bootsmodell.json";
        ConfigurationLock lock(root / "bootsmodell.lock");
        const auto current = read_config_json(path);
        const std::string current_revision = current.is_null() ? "0" :
            std::to_string(current.at("revision").get<std::uint64_t>());
        if (current_revision != erwartete_revision)
            return error_result("CORE-SEG-0015",
                                "Konfiguration wurde von einem anderen Prozess geaendert",
                                anforderung.element_id);
        auto latest = leseSegelKonfiguration(katalog, inspection);
        if (latest.status == SegelKonfigurationsStatus::Fehler ||
            latest.quellen_fingerprint != snapshot.quellen_fingerprint ||
            latest.modell_fassung != snapshot.modell_fassung)
            return error_result("CORE-SEG-0016",
                                "Modell-/Quellenstand hat sich seit der Vorschau geaendert",
                                anforderung.element_id);
        const auto latest_preview = preview_from(latest, anforderung);
        if (latest_preview.status != SegelErgebnisStatus::Vorschau) return latest_preview;
        auto updated = latest;
        auto* changed = find_lage(updated, anforderung.element_id);
        if (changed == nullptr)
            return error_result("CORE-SEG-0002",
                                "Unbekanntes oder mehrdeutiges Segel-Element",
                                anforderung.element_id);
        *changed = *latest_preview.lage;
        const auto revision = parse_revision(current_revision);
        if (revision == std::numeric_limits<std::uint64_t>::max())
            throw std::runtime_error("Konfigurationsrevision erschoepft");
        const auto next_revision = revision + 1;
        auto model = read_json(path);
        auto& element = const_cast<Json&>(find_model_element(model, anforderung.element_id));
        if (!element.contains("setting")) element["setting"] = Json::object();
        element["setting"].update(lage_json(*changed,
            *find_reference(updated, anforderung.element_id)));
        const auto display = json_string(element.at("Darstellung"), "datei");
        const auto source = json_string(element.at("Segel"), "quelle_datei");
        element["setting"]["montage_referenz"] = {
            {"schema_version", 1}, {"bezug", "kontur"},
            {"obj_sha256", sha256(root / std::filesystem::u8path(display))},
            {"kontur_sha256", sha256(root / std::filesystem::u8path(source))}};
        model["konfiguration"] = config_json(updated, next_revision, current);
        const auto content = model.dump(2) + "\n";
        atomic_write(path, content);
        auto result = latest_preview;
        result.status = SegelErgebnisStatus::Gespeichert;
        result.revision = std::to_string(next_revision);
        result.lage = *changed;
        return result;
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
