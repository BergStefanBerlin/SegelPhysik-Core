#pragma once

#include "segelphysik/modell.hpp"

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace segelphysik {

// Model catalogue interface (catalogue version 1). The GUI selects the folder
// and holds the catalogue object; the core keeps no global state. Answers
// address models by model ID and never contain file paths or internal
// physics data. Errors are thrown as std::runtime_error with a CORE-KAT-
// prefix (or the CORE-MOD- prefix of the model import for run requests).

struct KatalogDiagnose {
    std::string fehler_id;
    std::string text;
    std::string fundstelle;  // file name relative to the model folder plus context
};

struct KatalogEintrag {
    std::string modell_id;
    std::string name;
    std::optional<std::string> beschreibung;
    std::string modell_fassung;
    std::string status;  // verwendbar or fehlerhaft
    std::size_t fehleranzahl{};
};

class Modellkatalog {
public:
    const std::vector<KatalogEintrag>& eintraege() const { return eintraege_; }
    const KatalogEintrag* eintrag(const std::string& modell_id) const;

private:
    friend Modellkatalog modellkatalog(const std::filesystem::path& ordner);
    friend const std::filesystem::path& katalog_datei(const Modellkatalog&, const std::string&,
                                                      const std::string&);
    friend std::vector<KatalogDiagnose> modelldiagnose(const Modellkatalog&, const std::string&,
                                                       const std::string&);
    std::vector<KatalogEintrag> eintraege_;
    std::vector<std::filesystem::path> dateien_;
    std::vector<std::vector<KatalogDiagnose>> diagnosen_;
};

// Searches ordner/modell.json and ordner/*/modell.json, sorted by folder name.
// CORE-KAT-0001 if the folder is not readable.
Modellkatalog modellkatalog(const std::filesystem::path& ordner);

// Resolves a catalogue entry and checks the expected model version against
// the current files (CORE-KAT-0003 unknown ID, CORE-KAT-0004 outdated version).
const std::filesystem::path& katalog_datei(const Modellkatalog& katalog,
    const std::string& modell_id, const std::string& erwartete_fassung);

struct StrukturElement {
    std::string id;
    std::string name;
    bool darstellung = false;
    bool hydrostatik_volumen = false;
    std::optional<Bewegungsdefinition> bewegung;
};
struct StrukturGruppe {
    std::string id;
    std::string name;
    std::vector<StrukturElement> elemente;
};
struct Modellstruktur {
    std::string modell_id;
    std::string modell_fassung;
    std::vector<StrukturGruppe> gruppen;
    std::optional<double> gesamtmasse_kg;
    std::optional<std::array<Vektor3, 2>> huellquader_m;  // min, max of display geometry
};
Modellstruktur modellstruktur(const Modellkatalog& katalog, const std::string& modell_id,
                              const std::string& erwartete_fassung);

struct Darstellungsteil {
    std::string element_id;
    std::string gruppe;
    std::string herkunft;  // darstellung or volumen_ersatz
    std::vector<Vektor3> punkte_m;
    std::vector<std::array<std::size_t, 3>> dreiecke;
    std::optional<std::array<double, 3>> farbe_rgb;
    std::optional<std::string> oberflaeche;  // Werte siehe modell.hpp (Darstellung.oberflaeche)
};
struct Darstellungsgeometrie {
    std::string modell_id;
    std::string modell_fassung;
    std::vector<Darstellungsteil> teile;
    std::vector<KatalogDiagnose> diagnosen;  // unreadable display parts
};
// Without element_id all elements; CORE-KAT-0006 for an unknown element.
Darstellungsgeometrie darstellungsgeometrie(const Modellkatalog& katalog,
    const std::string& modell_id, const std::string& erwartete_fassung,
    const std::optional<std::string>& element_id = std::nullopt);

std::vector<KatalogDiagnose> modelldiagnose(const Modellkatalog& katalog,
    const std::string& modell_id, const std::string& erwartete_fassung);

// Loads the model of a catalogue entry for checking or simulation.
Modell katalogmodell(const Modellkatalog& katalog, const std::string& modell_id,
                     const std::string& erwartete_fassung);

// JSON answers with "katalogversion":"1".
std::string katalog_json(const Modellkatalog& katalog);
std::string modellstruktur_json(const Modellstruktur& struktur);
std::string darstellungsgeometrie_json(const Darstellungsgeometrie& geometrie);
std::string modelldiagnose_json(const std::string& modell_id, const std::string& modell_fassung,
                                const std::vector<KatalogDiagnose>& diagnosen);
std::string katalogfehler_json(const std::string& diagnose);

}
