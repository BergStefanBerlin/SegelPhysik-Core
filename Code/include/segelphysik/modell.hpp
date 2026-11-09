#pragma once

#include "segelphysik/pose.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <cstddef>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace segelphysik {

struct Mesh {
    std::vector<Vektor3> punkte;
    std::vector<std::array<std::size_t, 3>> dreiecke;
};

enum class HydrodynamikRolle { rumpf, kiel, ruder };
enum class Bewegungsart { drehen, heben };

struct Bewegungsdefinition {
    Bewegungsart art = Bewegungsart::drehen;
    bool ruderwinkel{};
    Vektor3 achse_modell{};
    Vektor3 drehpunkt_modell_m{};
    Vektor3 richtung_modell{};
    std::vector<std::string> mitbewegte_elemente;
    double hubweg_m{};
};

struct HydrodynamikParameter {
    double wert{};
    std::string einheit;
    std::string status;
    std::string quelle;
    std::string gueltigkeit;
    std::string unsicherheit;
};

struct HydrodynamikKennfeldpunkt {
    double anstellwinkel_rad{};
    double reynolds_zahl{};
    double cl{};
    double cd{};
    std::string status;
    std::string quelle;
    std::string gueltigkeit;
    std::string unsicherheit;
};

struct HydrodynamikKraftmodell {
    std::string methode;
    std::map<std::string, HydrodynamikParameter> parameter;
    std::vector<HydrodynamikKennfeldpunkt> kennfeld;
    std::vector<std::string> widerstandsanteile;
    std::string gueltigkeit;
    std::string quelle;
};

struct ElementHydrodynamikDaten {
    std::map<std::string, HydrodynamikKraftmodell> modelle;
    std::optional<std::string> materialreferenz;
};

struct Element {
    std::string id;
    double masse_kg = 0.;
    Vektor3 schwerpunkt_m{};
    std::array<double, 9> traegheit_kg_m2{};
    std::optional<Mesh> volumen;
    std::optional<Mesh> massen_mesh;
    std::string massenart;
    std::optional<double> dichte_kg_pro_m3;
    std::optional<double> wanddicke_m;
    std::optional<HydrodynamikRolle> hydrodynamik_rolle;
    std::optional<ElementHydrodynamikDaten> hydrodynamik_daten;
    std::optional<Bewegungsdefinition> bewegung;
    // Open-edge vertices of a shell mass mesh (e.g. the gunwale of a hull
    // without deck). Only kept when the element also has a displacement
    // volume; non-empty means the element can flood (A-C-00047).
    std::vector<Vektor3> flutrand_m;
    // Imported physical mass geometry, including the location of a point mass.
    std::vector<Vektor3> massenpunkte_m;
};

// Optionaler Modellblock "Hydrodynamische_Traegheit" (A-C-00087): ueberschreibt
// berechnete Zusatzmassen (a11..a66, kg bzw. kg m^2 im Modellrahmen um den
// Gesamtschwerpunkt) und Daempfungsparameter (zeta_rollen, zeta_stampfen,
// zeta_tauchen, gieren_linear [1/s], gieren_quadratisch [kg m^2]).
struct HydrodynamischeTraegheitModell {
    std::map<std::string, double> zusatzmasse;
    std::map<std::string, double> daempfung;
};

struct Modell {
    std::string id;
    std::vector<Element> elemente;
    double masse_kg = 0.;
    Vektor3 schwerpunkt_m{};
    std::array<double, 9> traegheit_kg_m2{};
    std::string fassung;
    std::optional<HydrodynamischeTraegheitModell> hydrodynamische_traegheit;
};

// JSON "2"/"3": geometrie.{datei,laengeneinheit:"m"} selects one OBJ containing
// uniquely named o objects. Darstellung, Hydrostatik.masse (except point
// masses) and Hydrostatik.volumen reference these with mesh:"o-ID".
// Schema 2 allows source variants per element; schema 3 requires one shared
// source object per element. No auto repair.
// Element ID, mesh ID and explicit Hydrodynamik.rolle are independent.
// No role is inferred in versions 2 or 3. All source objects use triangle faces.
// JSON 0.1-entwurf remains supported: id, gruppen.{name}.elemente; elements contain id and
// Darstellung, Hydrostatik, and optionally Hydrodynamik and Bewegung. Hydrostatik requires
// masse and optionally volumen. Hydrodynamik.rolle requires a physical volume
// mesh and is one of rumpf, kiel or ruder; optional model data carries
// coefficients, provenance and two-axis foil polars. Unknown structural/physics fields are rejected. name, status and
// entwurf are metadata only, not a physical validation or simulation approval.
// The eight canonical simple model IDs (bleistift, kegel, kugel, platte,
// pyramide, quader, rolle, unregelmaessig) require exactly one physical
// volume element and receive the rumpf role in the Core importer.
// Paths are relative to the JSON file; Darstellung is never used for physics
// (legacy display-only files are not opened by lade_modell).
// Darstellung.farbe is an optional "#rrggbb" colour,
// Darstellung.oberflaeche an optional "matt", "glaenzend", "metall", "chrom", "lack", "glas",
// "leuchtend", "schillernd", "facettiert", "drahtgitter", "perlmutt", "unsichtbar" (not drawn,
// e.g. air bodies), "stoff", "holz" or "metall_gebuerstet". Surface names are
// presentation metadata, not physical materials; no UV coordinates are required.
// OBJ faces must be triangles; negative indices and
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
// fassung is the same non-cryptographic FNV-1a fingerprint as modell_fassung:
// canonical JSON excluding top-level konfiguration and settings on named
// elements, plus all explicit geometry/sail/lookup file references.
Modell lade_modell(const std::filesystem::path& datei);

// Tolerant description reader for catalogue and display purposes. It does not
// validate physics; unusable JSON is rejected. Readable source geometry is
// retained in quellmeshes; absent/invalid sources remain display diagnostics.
// Element order matches lade_modell (groups by name).
struct ElementBeschreibung {
    std::string id;
    std::string gruppe;
    std::optional<std::filesystem::path> darstellung;
    std::optional<std::filesystem::path> volumen;
    std::optional<std::array<double, 3>> farbe_rgb;  // Darstellung.farbe "#rrggbb", 0..1
    std::optional<std::string> oberflaeche;          // Darstellung.oberflaeche, siehe oben
    std::optional<Bewegungsdefinition> bewegung;
    std::optional<std::string> darstellung_mesh;
    std::optional<std::string> volumen_mesh;
    std::optional<std::string> masse_mesh;
};
struct QuellMesh {
    std::string id;
    std::string ursprung;
    Mesh mesh;
};
struct ModellBeschreibung {
    std::string id;
    std::string name;
    std::optional<std::string> beschreibung;
    std::vector<std::string> gruppen;
    std::vector<ElementBeschreibung> elemente;
    nlohmann::json dokument;
    std::vector<QuellMesh> quellmeshes;
};
ModellBeschreibung lies_beschreibung(const std::filesystem::path& datei);

// Hashes canonical nlohmann JSON without top-level konfiguration and settings
// on gruppen.*.elemente[] with string IDs, then sorted unique datei,
// quelle_datei and lookup_datei references with their bytes. Missing files
// have an explicit marker; read
// failures throw. For an importable model it equals lade_modell(datei).fassung.
std::string modell_fassung(const std::filesystem::path& datei);

// Creates a new directory containing modells_information.json and boot.obj. Never overwrites
// an existing destination; originals are read only. Uses the regular importer.
// Schema-3 derivations become explicit source meshes in schema 2.
void konvertiere_modell_v2(const std::filesystem::path& quelle,
                          const std::filesystem::path& zielordner);
// Explicit opt-in permits validated source-derived surface selections/plane
// cuts. The default still rejects competing geometry without selecting it.
void konvertiere_modell_v3(const std::filesystem::path& quelle,
                          const std::filesystem::path& zielordner,
                          bool ableitungen_zulassen = false);

}
