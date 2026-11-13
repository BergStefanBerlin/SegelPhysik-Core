#pragma once

#include "segelphysik/hydrostatik.hpp"

namespace segelphysik {

struct Referenzpunkt {
    std::string id;
    std::string art;
    std::string element_id;
    std::string quelle;
    std::string fachstatus;
    std::string positionsstatus;
    std::string geschwindigkeitsstatus;
    std::string diagnose;
    std::optional<Vektor3> position_modell_m;
    std::optional<Vektor3> position_welt_m;
    std::optional<Vektor3> geschwindigkeit_modell_m_s;
    std::optional<Vektor3> geschwindigkeit_welt_m_s;
};

std::vector<Referenzpunkt> referenzpunkte(const Modell& modell, const Zustand& zustand,
    const Kraefte& kraefte, const Lateraldruckpunkt& lateral,
    const StationaeresGleichgewichtErgebnis* stationaer = nullptr);

}
