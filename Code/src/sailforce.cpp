#include "segelphysik/sailforce.hpp"
#include "segelphysik/hydrostatik.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace segelphysik {
namespace {

bool endlich(const Vektor3& v)
{
    return std::all_of(v.begin(), v.end(),
        [](double value) { return std::isfinite(value); });
}

void pruefe_summen(const SailforceSdpSummen& s)
{
    if (!std::isfinite(s.querkraft_n) ||
        !std::isfinite(s.einzelbetragssumme_n) ||
        !std::isfinite(s.erstes_moment_x_nm) ||
        !std::isfinite(s.erstes_moment_z_nm) ||
        s.einzelbetragssumme_n < 0.)
        throw std::invalid_argument("CORE-SIM-0001: Ungueltige sailforce-SDP-Rohsummen.");
    const double scale = std::max(std::abs(s.querkraft_n), s.einzelbetragssumme_n);
    if (std::abs(s.querkraft_n) - s.einzelbetragssumme_n >
        32. * std::numeric_limits<double>::epsilon() * scale ||
        (s.einzelbetragssumme_n == 0. &&
            (s.erstes_moment_x_nm != 0. || s.erstes_moment_z_nm != 0.)))
        throw std::invalid_argument("CORE-SIM-0001: Widerspruechliche sailforce-SDP-Rohsummen.");
}

}

void pruefe_lufteigenschaften(const Lufteigenschaften& luft,
    const LuftGueltigkeitsbereich& bereich)
{
    if (!std::isfinite(bereich.dichte_min_kg_m3) ||
        !std::isfinite(bereich.dichte_max_kg_m3) ||
        !std::isfinite(bereich.viskositaet_min_m2_s) ||
        !std::isfinite(bereich.viskositaet_max_m2_s) ||
        bereich.dichte_min_kg_m3 <= 0. ||
        bereich.viskositaet_min_m2_s <= 0. ||
        bereich.dichte_min_kg_m3 > bereich.dichte_max_kg_m3 ||
        bereich.viskositaet_min_m2_s > bereich.viskositaet_max_m2_s)
        throw std::invalid_argument("CORE-SIM-0001: Ungueltiger sailforce-Luftdatenbereich.");
    if (!std::isfinite(luft.dichte_kg_m3) ||
        !std::isfinite(luft.kinematische_viskositaet_m2_s) ||
        luft.dichte_kg_m3 < bereich.dichte_min_kg_m3 ||
        luft.dichte_kg_m3 > bereich.dichte_max_kg_m3 ||
        luft.kinematische_viskositaet_m2_s < bereich.viskositaet_min_m2_s ||
        luft.kinematische_viskositaet_m2_s > bereich.viskositaet_max_m2_s)
        throw std::invalid_argument(
            "CORE-SIM-0001: Lufteigenschaften ausserhalb des sailforce-Modellbereichs.");
}

SailforceSdp sailforce_sdp(const SailforceSdpSummen& summen)
{
    pruefe_summen(summen);
    SailforceSdp result;
    result.summen = summen;
    const double q = summen.querkraft_n;
    if (std::abs(q) < 1. || std::abs(q) < .05 * summen.einzelbetragssumme_n)
        return result;
    const Vektor3 position{summen.erstes_moment_x_nm / q, 0.,
        summen.erstes_moment_z_nm / q};
    if (!endlich(position))
        throw std::invalid_argument("CORE-SIM-0001: sailforce-SDP nicht endlich darstellbar.");
    result.position_modell_m = position;
    result.status = SailforceSdpStatus::gueltig;
    return result;
}

SailforceSdp sailforce_sdp_gesamt(const std::vector<SailforceSdpSummen>& aktive_segel)
{
    if (aktive_segel.empty()) {
        SailforceSdp result;
        result.status = SailforceSdpStatus::inaktiv;
        return result;
    }
    SailforceSdpSummen summe;
    for (const auto& segel : aktive_segel) {
        pruefe_summen(segel);
        summe.querkraft_n += segel.querkraft_n;
        summe.einzelbetragssumme_n += segel.einzelbetragssumme_n;
        summe.erstes_moment_x_nm += segel.erstes_moment_x_nm;
        summe.erstes_moment_z_nm += segel.erstes_moment_z_nm;
        pruefe_summen(summe);
    }
    return sailforce_sdp(summe);
}

SailforceModelllast sailforce_modelllast(const SailforceStreifenlast& last,
    const Pose& segel_nach_modell, const Vektor3& schwerpunkt_modell_m)
{
    if (!endlich(last.abfragepunkt_segel_m) || !endlich(last.kraft_segel_n) ||
        !endlich(last.moment_abfragepunkt_segel_nm) || !endlich(schwerpunkt_modell_m))
        throw std::invalid_argument("CORE-SIM-0001: Nicht endliche sailforce-Streifenlast.");
    SailforceModelllast result;
    result.darstellungsbezug_modell_m =
        segel_nach_modell.modellpunkt_nach_welt(last.abfragepunkt_segel_m);
    result.kraft_modell_n =
        segel_nach_modell.modellvektor_nach_welt(last.kraft_segel_n);
    const auto eigenmoment =
        segel_nach_modell.modellvektor_nach_welt(last.moment_abfragepunkt_segel_nm);
    result.moment_s_modell_nm = moment_um(result.darstellungsbezug_modell_m,
        schwerpunkt_modell_m, result.kraft_modell_n, eigenmoment);
    if (!endlich(result.darstellungsbezug_modell_m) || !endlich(result.kraft_modell_n) ||
        !endlich(result.moment_s_modell_nm))
        throw std::invalid_argument("CORE-SIM-0001: sailforce-Modelllast nicht endlich darstellbar.");
    return result;
}

}
