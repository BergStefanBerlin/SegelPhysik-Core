#pragma once

#include "segelphysik/simulation.hpp"

namespace segelphysik {
namespace detail { struct Aufbereitungsdaten; class Aufbereitungsbereich; }

// Owns a frozen model snapshot. Create a new context after changing model data
// or derived geometry; historical states retain their own movement/settings.
class Modellaufbereitung {
public:
    explicit Modellaufbereitung(const Modell& modell);
    ~Modellaufbereitung();
    Modellaufbereitung(const Modellaufbereitung&) = delete;
    Modellaufbereitung& operator=(const Modellaufbereitung&) = delete;
    const Modell& modell() const noexcept;
    Zustand schritt(const Zustand&, double dt_s,
        const std::vector<TestantriebEreignis>& testereignisse = {},
        const std::vector<Bewegungsereignis>& bewegungsereignisse = {},
        double rho = 1000., double g = 9.81);
    Kraefte auswertung(const Zustand&, double rho = 1000., double g = 9.81);
    std::string json(const Zustand&, const std::string& status,
        const std::string& diagnose = {},
        const Gleichgewichtsangaben* gleichgewicht = nullptr,
        const StationaeresGleichgewichtErgebnis* stationaer = nullptr,
        ErgebnisJsonFormat format = ErgebnisJsonFormat::V6);
private:
    std::unique_ptr<detail::Aufbereitungsdaten> daten_;
    friend class detail::Aufbereitungsbereich;
};
}
