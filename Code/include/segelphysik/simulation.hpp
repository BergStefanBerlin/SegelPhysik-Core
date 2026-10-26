#pragma once
#include "segelphysik/hydrostatik.hpp"
#include "segelphysik/messung.hpp"
#include <filesystem>

namespace segelphysik {
struct Laufoptionen {
    bool dynamisch = false;
    Zustand start;
    double dauer_s = 1.;
    double dt_s = .01;
    std::size_t ausgabe_intervall = 1;
    bool jsonl = true;
    const std::atomic_bool* abbruch = nullptr;
};
struct Laufbericht {
    std::string status;
    std::string diagnose;
    double gesamtlaufzeit_s{};
    std::size_t zustaende{};
    Messwerte messungen;
};
Laufbericht simulieren(const std::filesystem::path& modell,
    const std::filesystem::path& ausgabe, const Laufoptionen& optionen);
// Contract version 2: all version-1 fields keep their meaning; added are the
// contribution metadata, sums, balance status and (static only) equilibrium data.
std::string zustand_json(const Modell& modell,const Zustand& zustand,
    const std::string& status, const std::string& diagnose = {},
    const Gleichgewichtsangaben* gleichgewicht = nullptr);
std::string pruefung_json(const std::filesystem::path& modell, bool& gueltig);
}
