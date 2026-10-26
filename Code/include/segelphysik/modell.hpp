#pragma once

#include "segelphysik/pose.hpp"

#include <array>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace segelphysik {

struct Mesh {
    std::vector<Vektor3> punkte;
    std::vector<std::array<std::size_t, 3>> dreiecke;
};

struct Element {
    std::string id;
    double masse_kg = 0.;
    Vektor3 schwerpunkt_m{};
    std::array<double, 9> traegheit_kg_m2{};
    std::optional<Mesh> volumen;
    // Open-edge vertices of a shell mass mesh (e.g. the gunwale of a hull
    // without deck). Only kept when the element also has a displacement
    // volume; non-empty means the element can flood (A-C-00047).
    std::vector<Vektor3> flutrand_m;
};

struct Modell {
    std::string id;
    std::vector<Element> elemente;
    double masse_kg = 0.;
    Vektor3 schwerpunkt_m{};
    std::array<double, 9> traegheit_kg_m2{};
    std::string fassung;
};

// JSON 0.1-entwurf: id, gruppen.{name}.elemente; elements contain id and
// Darstellung and/or Hydrostatik. Hydrostatik requires masse and optionally
// volumen. Unknown structural/physics fields are rejected. name, status and
// entwurf are metadata only, not a physical validation or simulation approval.
// Paths are relative to the JSON file; Darstellung is never used for physics
// (its file is not opened). OBJ faces must be triangles; negative indices and
// v/vt/vn references are supported. Coordinates are model-fixed metres.
//
// Mass variants: punktmasse(masse_kg, position_m), vollkoerper
// (dichte_kg_pro_m3, datei), schale(dichte_kg_pro_m3, wanddicke_m, datei).
// A shell is one material midsurface, integrated as density * thickness * area,
// with no through-thickness inertia or automatic closed-shell reinterpretation.
// Duplicate faces are rejected; arbitrary near-coincident/double-wall material
// cannot be identified automatically and remains the author's responsibility.
//
// Inertia is row-major, about the respective centre of mass, in model axes.
// Drawing-only elements have zero mass/inertia and no displacement. A model
// without positive physical total mass is rejected with CORE-MOD-0004.
// Flooding (A-C-00047): a shell mass mesh with open edges whose element also
// has a displacement volume is floodable; its open-edge vertices form the rim.
// Volume/solid meshes must be closed, consistently outward oriented manifolds.
// Every disconnected closed component must have positive signed volume.
// Component AABBs with positive interior overlap are conservatively rejected;
// no mesh repair. Mere AABB boundary contact is permitted.
// Displacement AABBs with positive interior overlap cause CORE-MOD-0007, even
// when actual volumes are disjoint: no Boolean union/exact volume-overlap test.
// The surface-intersection helper deliberately also detects coplanar material
// contact; it cannot classify positive-volume overlap versus boundary contact.
// No ray-parity/containment heuristic is used to override conservative rejection.
// Triangle intersection checks use relative floating-point tolerances, not
// exact predicates; validation is not a proof against every geometric defect.
// Face-pair testing uses an AABB sweep, worst-case quadratic when boxes overlap.
// Throws std::runtime_error with CORE-MOD-0001..0008 and file/element context.
// fassung is a non-cryptographic FNV-1a fingerprint of JSON and the actually
// imported physics OBJ contents, including boundaries; not of display files.
Modell lade_modell(const std::filesystem::path& datei);

// Tolerant description reader for catalogue and display purposes. It does not
// validate physics and throws (CORE-MOD-0001/0002/0005) only if the JSON
// itself is unusable. Element order matches lade_modell (groups by name).
struct ElementBeschreibung {
    std::string id;
    std::string gruppe;
    std::optional<std::filesystem::path> darstellung;
    std::optional<std::filesystem::path> volumen;
};
struct ModellBeschreibung {
    std::string id;
    std::string name;
    std::optional<std::string> beschreibung;
    std::vector<std::string> gruppen;
    std::vector<ElementBeschreibung> elemente;
};
ModellBeschreibung lies_beschreibung(const std::filesystem::path& datei);

// Fingerprint over the JSON and all readable physics OBJ files in import
// order. For an importable model it equals lade_modell(datei).fassung.
std::string modell_fassung(const std::filesystem::path& datei);

}
