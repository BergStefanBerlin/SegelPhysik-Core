#include "aufbereitung_intern.hpp"
#include <cmath>
#include <stdexcept>

namespace segelphysik {
namespace detail {
thread_local Aufbereitungsdaten* aktive_aufbereitung = nullptr;
}
Modellaufbereitung::Modellaufbereitung(const Modell& modell)
{
    SEGEL_MESSUNG("modellbasis_aufbau");
    detail::zaehle_meshkopien(modell);
    daten_=std::make_unique<detail::Aufbereitungsdaten>(modell);
}
Modellaufbereitung::~Modellaufbereitung() = default;
const Modell& Modellaufbereitung::modell() const noexcept { return daten_->basis; }
Zustand Modellaufbereitung::schritt(const Zustand& s, double dt,
    const std::vector<TestantriebEreignis>& testereignisse,
    const std::vector<Bewegungsereignis>& bewegungsereignisse, double rho, double g)
{
    detail::Aufbereitungsbereich bereich(*this);
    bereich.zustand(s);
    return dynamikschritt(modell(),s,dt,testereignisse,bewegungsereignisse,rho,g);
}
Kraefte Modellaufbereitung::auswertung(const Zustand& s,double rho,double g)
{
    detail::Aufbereitungsbereich bereich(*this);
    bereich.zustand(s);
    detail::pruefe_auswertungszustand(modell(),s);
    return kraefte(modell(),s.pose,s.testantrieb.get(),rho,g,s.geflutet,
        s.hydrodynamik.get(),s.geschwindigkeit_welt_m_s,s.omega_modell_rad_s,
        s.bewegungsstellungen,s.dynamische_fremdkraft.get());
}
std::string Modellaufbereitung::json(const Zustand& s,const std::string& status,
    const std::string& diagnose,const Gleichgewichtsangaben* gg,
    const StationaeresGleichgewichtErgebnis* stationaer,ErgebnisJsonFormat format)
{
    detail::Aufbereitungsbereich bereich(*this);
    bereich.zustand(s);
    return zustand_json(modell(),s,status,diagnose,gg,stationaer,format);
}
}
