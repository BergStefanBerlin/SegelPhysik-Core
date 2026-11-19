#pragma once

#include "segelphysik/pose.hpp"

#include <optional>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace segelphysik {

struct Lufteigenschaften {
    double dichte_kg_m3 = 1.225;
    double kinematische_viskositaet_m2_s = 1.5e-5;
};

struct LuftGueltigkeitsbereich {
    double dichte_min_kg_m3{};
    double dichte_max_kg_m3{};
    double viskositaet_min_m2_s{};
    double viskositaet_max_m2_s{};
};

void pruefe_lufteigenschaften(const Lufteigenschaften& luft,
    const LuftGueltigkeitsbereich& bereich);

struct SailforceSdpSummen {
    double querkraft_n{};
    double einzelbetragssumme_n{};
    double erstes_moment_x_nm{};
    double erstes_moment_z_nm{};
};

enum class SailforceSdpStatus { gueltig, nicht_bestimmbar, inaktiv };

struct SailforceSdp {
    SailforceSdpSummen summen;
    SailforceSdpStatus status = SailforceSdpStatus::nicht_bestimmbar;
    std::optional<Vektor3> position_modell_m;
};

SailforceSdp sailforce_sdp(const SailforceSdpSummen& summen);
// Only active sails belong in this list; preserve producer sums before division.
SailforceSdp sailforce_sdp_gesamt(const std::vector<SailforceSdpSummen>& aktive_segel);

struct SailforceStreifenlast {
    Vektor3 abfragepunkt_segel_m{};
    Vektor3 kraft_segel_n{};
    Vektor3 moment_abfragepunkt_segel_nm{};
};

struct SailforceModelllast {
    Vektor3 darstellungsbezug_modell_m{};
    Vektor3 kraft_modell_n{};
    Vektor3 moment_s_modell_nm{};
};

SailforceModelllast sailforce_modelllast(const SailforceStreifenlast& last,
    const Pose& segel_nach_modell, const Vektor3& schwerpunkt_modell_m);

struct Modell;
struct Zustand;
struct Kraefte;
struct SailforceVorbereitung;

struct SailforceSegelwahl {
    std::string element_id;
    bool aktiv = true;
    double stellwinkel_grad{};
};

struct SailforceKonfiguration {
    Lufteigenschaften luft;
    std::vector<SailforceSegelwahl> segel;
};

struct SailforceKontext {
    std::shared_ptr<const SailforceVorbereitung> vorbereitung;
    SailforceKonfiguration konfiguration;
    std::string modell_fassung;
    std::string quellen_fassung;
    std::size_t revision{};
};

struct SailforceStreifenErgebnis {
    std::size_t id{};
    SailforceModelllast last;
};

struct SailforceGrenzlinie {
    std::size_t id{};
    std::size_t grenze_id{};
    std::optional<std::size_t> streifen_unten_id;
    std::optional<std::size_t> streifen_oben_id;
    std::vector<Vektor3> punkte_segel_m;
};

struct SailforceSegelErgebnis {
    std::string element_id;
    std::string quellfassung;
    bool aktiv{};
    double stellwinkel_grad{};
    bool form_gespiegelt{};
    std::vector<SailforceStreifenErgebnis> streifen;
    std::shared_ptr<const std::vector<SailforceGrenzlinie>> grenzlinien_segel_m;
    Pose segel_nach_modell{{}, {0., 0., 0., 1.}};
    Vektor3 kraft_modell_n{};
    Vektor3 moment_s_modell_nm{};
    Vektor3 darstellungsbezug_modell_m{};
    SailforceSdp sdp;
};

struct SailforceAuswertung {
    std::size_t revision{};
    std::vector<SailforceSegelErgebnis> segel;
    Vektor3 kraft_modell_n{};
    Vektor3 moment_s_modell_nm{};
    Vektor3 darstellungsbezug_modell_m{};
    SailforceSdp sdp_gesamt;
};

Zustand sailforce_start(const Modell&, const Zustand&, const std::filesystem::path& modellpfad,
    const SailforceKonfiguration& konfiguration = {});
Zustand sailforce_stellwinkel(const Zustand&, const std::string& element_id, double winkel_grad);
SailforceAuswertung sailforce_auswertung(const Modell&, const Zustand&,
    const Vektor3& schwerpunkt_modell_m);
std::string sailforce_json(const SailforceAuswertung&, const Lufteigenschaften&);
void sailforce_addieren(Kraefte&, const Modell&, const Zustand&);

}
