#pragma once

#include "segelphysik/katalog.hpp"

namespace segelphysik {

Darstellungsgeometrie darstellungsgeometrie_aus_dateien(
    const Modellkatalog& katalog, const std::string& modell_id,
    const std::string& erwartete_fassung,
    const std::optional<std::string>& element_id);

Darstellungsgeometrie darstellungsgeometrie_aus_beschreibung(
    const std::string& modell_id, const std::string& erwartete_fassung,
    const std::filesystem::path& modelldatei, const ModellBeschreibung& beschreibung,
    const std::optional<std::string>& element_id);

Modellstruktur modellstruktur_aus_dateien(
    const Modellkatalog& katalog, const std::string& modell_id,
    const std::string& erwartete_fassung);

}
