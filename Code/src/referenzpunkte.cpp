#include "segelphysik/referenzpunkte.hpp"
#include "referenzpunkte_intern.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace segelphysik {
namespace {

Referenzpunkt punkt(const Zustand& zustand, std::string id, std::string art,
    std::string element, std::string quelle, std::optional<Vektor3> position,
    std::optional<Vektor3> geschwindigkeit, std::string diagnose = {})
{
    Referenzpunkt result;
    result.id = std::move(id);
    result.art = std::move(art);
    result.element_id = std::move(element);
    result.quelle = std::move(quelle);
    result.diagnose = std::move(diagnose);
    result.positionsstatus = position ? "gueltig" : "nicht_verfuegbar";
    result.geschwindigkeitsstatus = geschwindigkeit ? "gueltig" : "nicht_verfuegbar";
    if (position) {
        if (!std::all_of(position->begin(), position->end(),
                         [](double value) { return std::isfinite(value); }))
            throw std::invalid_argument("CORE-SIM-0001: Referenzpunkt nicht endlich.");
        result.position_modell_m = position;
        result.position_welt_m = zustand.pose.modellpunkt_nach_welt(*position);
    }
    if (geschwindigkeit) {
        if (!std::all_of(geschwindigkeit->begin(), geschwindigkeit->end(),
                         [](double value) { return std::isfinite(value); }))
            throw std::invalid_argument("CORE-SIM-0001: Punktgeschwindigkeit nicht endlich.");
        result.geschwindigkeit_welt_m_s = geschwindigkeit;
        result.geschwindigkeit_modell_m_s = zustand.pose.weltvektor_nach_modell(*geschwindigkeit);
    }
    return result;
}

}

std::vector<Referenzpunkt> referenzpunkte(const Modell& modell, const Zustand& zustand,
    const Kraefte& kraefte, const Lateraldruckpunkt& lateral,
    const StationaeresGleichgewichtErgebnis* stationaer)
{
    std::vector<Referenzpunkt> result;
    if (zustand.mast) detail::pruefe_mast(modell, *zustand.mast);
    const auto mast_position = zustand.mast ? zustand.mast->position_modell_m : std::nullopt;
    const auto mast_velocity = mast_position
        ? std::optional<Vektor3>(detail::starre_punktgeschwindigkeit(
              zustand, *mast_position, kraefte.schwerpunkt_modell_m))
        : std::nullopt;
    result.push_back(punkt(zustand, "mastspitze", "mastspitze",
        zustand.mast ? zustand.mast->element_id : "",
        zustand.mast ? zustand.mast->quelle : "",
        mast_position, mast_velocity,
        zustand.mast ? zustand.mast->diagnose : "Keine Mastdaten im Ergebnisstand"));
    result.push_back(punkt(zustand, "schwerpunkt", "schwerpunkt", "", "gesamtmasse",
        kraefte.schwerpunkt_modell_m, zustand.geschwindigkeit_welt_m_s));
    for (const auto& beitrag : kraefte.beitraege) {
        if (beitrag.rolle != Rolle::einzelbeitrag || beitrag.art != "kraft" ||
            beitrag.punktart != Punktart::angriffspunkt) continue;
        const bool available = beitrag.punkt_bekannt &&
            (beitrag.zustand == Datenzustand::gueltig || beitrag.zustand == Datenzustand::null);
        result.push_back(punkt(zustand, "beitrag:" + beitrag.id,
            beitrag.quelle == Quelle::hydrostatik ? "auftrieb" : "kraftangriffspunkt",
            beitrag.element_id, beitrag.id,
            available ? std::optional<Vektor3>(beitrag.punkt_m) : std::nullopt,
            {}, available ? "" : name(beitrag.zustand)));
        result.back().fachstatus = name(beitrag.zustand);
    }
    if (lateral.vorhanden) {
        result.push_back(punkt(zustand, "lateraldruckpunkt", "lateraldruckpunkt", "",
            "lateraldruckpunkt", lateral.position_modell_m, {},
            lateral.position_modell_m ? "" : lateral.status));
        result.back().fachstatus = lateral.status;
    }
    if (stationaer || zustand.dynamische_fremdkraft) {
        const auto& force = stationaer ? stationaer->konfiguration.kraft_welt_n
            : zustand.dynamische_fremdkraft->konfiguration.kraft_modellfest_n;
        const auto& position = stationaer ? stationaer->konfiguration.angriffspunkt_modell_m
            : zustand.dynamische_fremdkraft->konfiguration.angriffspunkt_modell_m;
        const bool active = stationaer ? stationaer->konfiguration.aktiv
            : zustand.dynamische_fremdkraft->konfiguration.aktiv;
        const bool force_null = std::all_of(force.begin(), force.end(),
            [](double value) { return value == 0.; });
        const bool available = active && !force_null;
        result.push_back(punkt(zustand, "ersatz_segel", "ersatz_sdp", "", "fremdkraft",
            available ? std::optional<Vektor3>(position) : std::nullopt, {},
            !active ? "inaktiv" : force_null ? "kraft_null" : ""));
        result.back().fachstatus = !active ? "inaktiv" : force_null ? "kraft_null" : "gueltig";
    }
    return result;
}

}
