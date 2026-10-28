#pragma once

#include "segelphysik/modell.hpp"
#include <atomic>
#include <optional>
#include <string_view>
#include <memory>

namespace segelphysik {

struct VolumenWerte {
    double volumen_m3{};
    std::optional<Vektor3> schwerpunkt_m;
    bool geflutet{};
};

// Contract version 2 metadata is stored as enums to keep the per-evaluation
// cost of the solver hot path unchanged; names appear only when serialised.
enum class Quelle { schwerkraft, hydrostatik, hydrodynamik, summe, nicht_zugeordnet, testantrieb };
enum class Rolle { einzelbeitrag, gruppensumme, gesamtsumme };
enum class Punktart { angriffspunkt, bezugspunkt };
enum class Datenzustand { gueltig, null, deaktiviert, nicht_berechnet, ungueltig };

const char* name(Quelle wert);
const char* name(Rolle wert);
const char* name(Punktart wert);
const char* name(Datenzustand wert);

struct Beitrag {
    std::string id;
    std::string element_id;
    std::string art;
    Vektor3 punkt_m{};
    Vektor3 vektor{};
    Quelle quelle = Quelle::nicht_zugeordnet;
    std::string quellname;  // original name for Quelle::nicht_zugeordnet
    std::string_view gruppe = "gewicht_auftrieb";
    Rolle rolle = Rolle::einzelbeitrag;
    Punktart punktart = Punktart::angriffspunkt;
    Datenzustand zustand = Datenzustand::gueltig;
    std::string fehler_id;
    Vektor3 moment_s_nm{};  // moment about the current total centre of mass S
    bool punkt_bekannt = true;
};

struct Kraefte {
    Vektor3 kraft_welt_n{};
    Vektor3 moment_modell_nm{};
    double volumen_m3{};
    std::vector<Beitrag> beitraege;
    std::vector<std::pair<std::string, VolumenWerte>> elementvolumina;
};

struct HydrodynamikKonfiguration {
    double rho_kg_m3 = 1000.;
    double nu_m2_s = 1e-6;
    Vektor3 fluidgeschwindigkeit_welt_m_s{};
};
struct HydrodynamikKontext {
    HydrodynamikKonfiguration konfiguration;
    std::string modell_fassung;
    std::size_t auswertung_schritt{};
    double auswertung_zeit_s{};
};

VolumenWerte wasseranteil(const Mesh& mesh, const Pose& pose);
// M_P = (A - P) x F + M_A: moment of a force applied at A (with own moment M_A) about P.
Vektor3 moment_um(const Vektor3& angriffspunkt, const Vektor3& bezugspunkt,
                  const Vektor3& kraft, const Vektor3& eigenmoment = {});
// geflutet: sorted IDs of flooded elements; they keep their weight but give
// zero buoyancy (A-C-00047). All other elements are unaffected.
Kraefte hydrostatik(const Modell& modell, const Pose& pose,
                   double rho = 1000., double g = 9.81,
                   const std::vector<std::string>& geflutet = {});
// Returns bisher plus every floodable element with a rim point below the
// water plane (world z < 0) in this pose. Sorted, without duplicates;
// flooding is permanent and never removed.
std::vector<std::string> flutung(const Modell& modell, const Pose& pose,
                                 std::vector<std::string> bisher);

struct TestantriebKonfiguration {
    double kraft_n = 200.;
    double grenze_m_s = 463./90.;
};
struct TestantriebEreignis {
    std::size_t kennung{};
    bool aktiv{};
};
struct TestantriebBegrenzung {
    bool wirksam{};
    Vektor3 vorher_welt_m_s{};
    Vektor3 nachher_welt_m_s{};
    Vektor3 korrekturimpuls_ns{};
    double energieaenderung_j{};
};
struct TestantriebUebergang {
    std::size_t von_schritt{};
    std::size_t nach_schritt{};
    double startzeit_s{};
    double dt_s{};
    std::vector<TestantriebEreignis> ereignisse;
    TestantriebBegrenzung begrenzung;
};
struct TestantriebKontext {
    TestantriebKonfiguration konfiguration;
    bool aktiv{};
    std::optional<Vektor3> zugpunkt_modell_m;
    std::string geometriequelle = "physische_massen_volumennetze_punktmassen";
    std::string modell_fassung;
    std::size_t letzte_ereigniskennung{};
    std::optional<TestantriebUebergang> uebergang;
    std::size_t auswertung_schritt{};
    double auswertung_zeit_s{};
};

struct Zustand {
    Pose pose{{}, {0., 0., 0., 1.}};
    Vektor3 geschwindigkeit_welt_m_s{};
    Vektor3 omega_modell_rad_s{};
    double zeit_s{};
    std::size_t schritt{};
    std::vector<std::string> geflutet;
    std::shared_ptr<const TestantriebKontext> testantrieb;
    bool testantrieb_erforderlich = false;
    std::shared_ptr<const HydrodynamikKontext> hydrodynamik;
    bool hydrodynamik_erforderlich = false;
};

// Returned states own an immutable historical context; no global live flag.
Zustand testantrieb_start(const Modell&, const Zustand&,
    const TestantriebKonfiguration&, bool aktiv);
Zustand hydrodynamik_start(const Modell&, const Zustand&,
    const HydrodynamikKonfiguration&);
Kraefte kraefte(const Modell&, const Pose&, const TestantriebKontext*,
    double rho = 1000., double g = 9.81,
    const std::vector<std::string>& geflutet = {},
    const HydrodynamikKontext* hydrodynamik = nullptr,
    const Vektor3& geschwindigkeit_welt_m_s = {});

struct Gleichgewichtsangaben {
    double krafttoleranz_n{};
    double momenttoleranz_nm{};
    double residuum_kraft_n{};
    double residuum_moment_nm{};
    std::vector<std::string> freiheitsgrade;
    std::size_t iterationen{};
};

struct SolverErgebnis {
    Zustand zustand;
    std::string status;
    std::string diagnose;
    std::size_t iterationen{};
    std::optional<Gleichgewichtsangaben> gleichgewicht;
};

struct Summe {
    std::string gruppe;
    Vektor3 kraft_modell_n{};
    Vektor3 moment_s_nm{};
    std::vector<std::string> quellen;
};

struct Bilanz {
    std::string status;  // vollstaendig, teilsumme or ungeklaert
    std::vector<std::string> erwartete_beitraege;
    std::vector<std::string> fehlende_beitraege;
    Summe gesamt;
    std::vector<Summe> gruppen;
};

// Marks non-finite contributions as ungueltig (CORE-BIL-0001) and derives the
// group/total sums from the individual contributions with zustand gueltig/null.
// The expected scope is weight per element plus buoyancy per element with a
// displacement volume.
Bilanz bilanz(const Modell& modell, std::vector<Beitrag>& beitraege,
    bool hydrodynamik_aktiv = false);

// Static equilibrium (A-C-00047): first solved with the flooding state of
// start; if a floodable rim is below water in the result, that element is
// flooded permanently and the search is repeated. The final state carries
// the flooded IDs in zustand.geflutet.
SolverErgebnis gleichgewicht(const Modell& modell, const Zustand& start,
    double krafttoleranz_n = .001, double momenttoleranz_nm = .001,
    std::size_t budget = 200, const std::atomic_bool* abbruch = nullptr);
// Updates the flooding state from the start and end pose of the step.
Zustand dynamikschritt(const Modell& modell, const Zustand& zustand,
    double dt_s, double rho = 1000., double g = 9.81);
Zustand dynamikschritt(const Modell& modell, const Zustand& zustand,
    double dt_s, const std::vector<TestantriebEreignis>& ereignisse,
    double rho = 1000., double g = 9.81);

}
