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

}
