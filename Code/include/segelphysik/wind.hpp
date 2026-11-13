#pragma once

#include "segelphysik/hydrostatik.hpp"

namespace segelphysik {

enum class WindModus { weltfest, bootmitgefuehrt };

struct WindKonfiguration {
    double herkunft_welt_grad{};
    double referenzgeschwindigkeit_m_s = 6.;
    double referenzhoehe_m = 10.;
    WindModus modus = WindModus::weltfest;
};

struct WindKontext {
    WindKonfiguration konfiguration;
    double referenzkurs_grad{};
    std::string modell_fassung;
};

struct WindVektor {
    Vektor3 stroemung_welt_m_s{};
    double horizontal_m_s{};
    double raeumlich_m_s{};
    std::optional<double> herkunft_welt_grad;
    bool kompass_sichtbar{};
    bool pfeil_sichtbar{};
};

struct WindAuswertung {
    double zeit_s{};
    std::size_t schritt{};
    std::optional<Vektor3> mastspitze_welt_m;
    std::optional<Vektor3> mastgeschwindigkeit_welt_m_s;
    std::optional<WindVektor> wahr;
    std::optional<WindVektor> scheinbar;
    std::string diagnose;
};

Zustand wind_start(const Modell& modell, const Zustand& zustand,
    const WindKonfiguration& konfiguration, std::optional<Vektor3> mastspitze_modell_m,
    const std::string& mast_element_id, const std::string& mast_diagnose);
Zustand wind_aendern(const Zustand& zustand, std::optional<double> herkunft_welt_grad,
    std::optional<double> referenzgeschwindigkeit_m_s, std::optional<WindModus> modus);
double windprofil(const WindKonfiguration& konfiguration, double hoehe_m);
WindAuswertung wind_auswertung(const Modell& modell, const Zustand& zustand,
    const Vektor3& schwerpunkt_modell_m);

}
