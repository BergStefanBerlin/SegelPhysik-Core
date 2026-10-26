#pragma once

#include "segelphysik/modell.hpp"
#include <atomic>
#include <optional>

namespace segelphysik {

struct VolumenWerte {
    double volumen_m3{};
    std::optional<Vektor3> schwerpunkt_m;
};

struct Beitrag {
    std::string id;
    std::string element_id;
    std::string art;
    Vektor3 punkt_m{};
    Vektor3 vektor{};
};

struct Kraefte {
    Vektor3 kraft_welt_n{};
    Vektor3 moment_modell_nm{};
    double volumen_m3{};
    std::vector<Beitrag> beitraege;
    std::vector<std::pair<std::string, VolumenWerte>> elementvolumina;
};

VolumenWerte wasseranteil(const Mesh& mesh, const Pose& pose);
Kraefte hydrostatik(const Modell& modell, const Pose& pose,
                   double rho = 1000., double g = 9.81);

struct Zustand {
    Pose pose{{}, {0., 0., 0., 1.}};
    Vektor3 geschwindigkeit_welt_m_s{};
    Vektor3 omega_modell_rad_s{};
    double zeit_s{};
    std::size_t schritt{};
};

struct SolverErgebnis {
    Zustand zustand;
    std::string status;
    std::string diagnose;
    std::size_t iterationen{};
};

SolverErgebnis gleichgewicht(const Modell& modell, const Zustand& start,
    double krafttoleranz_n = .001, double momenttoleranz_nm = .001,
    std::size_t budget = 200, const std::atomic_bool* abbruch = nullptr);
Zustand dynamikschritt(const Modell& modell, const Zustand& zustand,
    double dt_s, double rho = 1000., double g = 9.81);

}
