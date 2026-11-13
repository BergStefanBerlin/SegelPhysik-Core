#pragma once

#include "segelphysik/hydrostatik.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace segelphysik::detail {

inline void pruefe_mast(const Modell& modell, const MastReferenz& mast)
{
    if (mast.modell_fassung != modell.fassung)
        throw std::invalid_argument("CORE-SIM-0001: Mastreferenz gehoert zu anderer Modellfassung.");
    if (mast.position_modell_m) {
        if (mast.element_id.empty() || mast.quelle.empty() ||
            !std::all_of(mast.position_modell_m->begin(), mast.position_modell_m->end(),
                [](double value) { return std::isfinite(value); }))
            throw std::invalid_argument("CORE-SIM-0001: Ungueltige Mastreferenz.");
    } else if (mast.diagnose.empty()) {
        throw std::invalid_argument("CORE-SIM-0001: Diagnose fuer fehlende Mastreferenz erforderlich.");
    }
}

inline Vektor3 starre_punktgeschwindigkeit(const Zustand& zustand,
    const Vektor3& punkt_modell_m, const Vektor3& schwerpunkt_modell_m)
{
    const auto point = zustand.pose.modellpunkt_nach_welt(punkt_modell_m);
    const auto cg = zustand.pose.modellpunkt_nach_welt(schwerpunkt_modell_m);
    const auto omega = zustand.pose.modellvektor_nach_welt(zustand.omega_modell_rad_s);
    const Vektor3 r{point[0] - cg[0], point[1] - cg[1], point[2] - cg[2]};
    const Vektor3 velocity{
        zustand.geschwindigkeit_welt_m_s[0] + omega[1]*r[2] - omega[2]*r[1],
        zustand.geschwindigkeit_welt_m_s[1] + omega[2]*r[0] - omega[0]*r[2],
        zustand.geschwindigkeit_welt_m_s[2] + omega[0]*r[1] - omega[1]*r[0]};
    if (!std::all_of(velocity.begin(), velocity.end(), [](double value) { return std::isfinite(value); }))
        throw std::invalid_argument("CORE-SIM-0001: Mastgeschwindigkeit nicht endlich.");
    return velocity;
}

}
