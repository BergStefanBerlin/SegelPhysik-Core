#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace segelphysik {

enum class ModellFingerprintBereich {
    visualisation,
    mass,
    hydrostatics,
    aerodynamics,
    modelData
};

struct ModellFingerprintWert {
    ModellFingerprintBereich bereich;
    std::string hash;
    std::string modified_at;
};

struct ModellFingerprintErgebnis {
    std::vector<ModellFingerprintWert> werte;
    bool validierungsdatei_aktualisiert{};
};

ModellFingerprintErgebnis aktualisiere_modell_fingerprints(
    const std::filesystem::path& modellordner,
    const std::vector<ModellFingerprintBereich>& bereiche);

}
