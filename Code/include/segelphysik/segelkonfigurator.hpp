#pragma once

#include "segelphysik/inspektion.hpp"

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace segelphysik {

enum class SegelArt { Grosssegel, Fock };
enum class SegelBuglage { Unbekannt, Backbord, Steuerbord };
enum class SegelEbene { XY, XZ, YZ };
enum class SegelEcke { Hals, Schothorn, KopfVorliek, KopfAchterliek };
enum class SegelAktion {
    Automatisch,
    Translation,
    Rotation,
    FockVersatzCm,
    OriginalBuglage
};
enum class SegelKonfigurationsStatus { NichtKonfiguriert, Bereit, Fehler };
enum class SegelErgebnisStatus { Vorschau, Gespeichert, Fehler };

struct SegelEcken {
    Vektor3 hals_m{};
    Vektor3 schothorn_m{};
    Vektor3 kopf_vorliek_m{};
    std::optional<Vektor3> kopf_achterliek_m;
};

struct SegelReferenz {
    std::string element_id;
    std::string intrinsische_quelle_id;
    std::string darstellungs_mesh_id;
    std::string lookup_id;
    SegelArt art = SegelArt::Grosssegel;
    SegelEcken ecken;
    Vektor3 vorliek_richtung_lokal{};
    Vektor3 unterliek_richtung_lokal{};
    Vektor3 rig_hals_anschlag_m{};
    Vektor3 rig_vorliek_richtung_modell{};
    Vektor3 rig_unterliek_richtung_modell{};
    Vektor3 fock_grundhals_m{};
    Vektor3 fock_versatzrichtung_modell{};
    SegelBuglage original_buglage = SegelBuglage::Unbekannt;
    std::string quellen_fingerprint;
    MontageReferenz montage;
    std::optional<MontageReferenz> legacy_montage;
    bool montage_basis_vorgegeben{};
};

struct SegelLage {
    std::string element_id;
    QuaternionXyzw rotation_xyzw{0., 0., 0., 1.};
    Vektor3 translation_m{};
    double fock_versatz_cm{};
    SegelBuglage original_buglage = SegelBuglage::Unbekannt;
    bool konfiguriert{};
};

struct SegelDiagnose {
    std::string code;
    std::string nachricht;
    std::string element_id;
};

struct SegelKonfigurationSnapshot {
    std::string modell_id;
    std::string modell_fassung;
    std::string quellen_fingerprint;
    std::string revision = "0";
    SegelKonfigurationsStatus status = SegelKonfigurationsStatus::Fehler;
    std::vector<SegelReferenz> referenzen;
    std::vector<SegelLage> lagen;
    std::vector<InspectionMesh> grundmeshes;
    std::vector<SegelDiagnose> diagnosen;
};

struct SegelAktionAnforderung {
    SegelAktion aktion = SegelAktion::Automatisch;
    std::string element_id;
    SegelEbene ebene = SegelEbene::XY;
    Vektor3 translation_m{};
    SegelEcke bewegte_ecke = SegelEcke::Schothorn;
    double winkel_grad{};
    double fock_versatz_cm{};
    std::optional<SegelBuglage> original_buglage;
};

struct SegelKonfigurationErgebnis {
    SegelErgebnisStatus status = SegelErgebnisStatus::Fehler;
    std::string revision;
    std::optional<SegelLage> lage;
    std::vector<InspectionMesh> meshes;
    std::vector<SegelDiagnose> diagnosen;
};

SegelKonfigurationSnapshot leseSegelKonfiguration(
    const Modellkatalog& katalog, const ModelInspection& inspection);
SegelKonfigurationErgebnis leseKonfigurierteSegelgeometrie(
    const SegelKonfigurationSnapshot& snapshot);
SegelKonfigurationErgebnis pruefeSegelAktion(
    const SegelKonfigurationSnapshot& snapshot,
    const SegelAktionAnforderung& anforderung);
SegelKonfigurationErgebnis bestaetigeSegelAktion(
    const Modellkatalog& katalog, const ModelInspection& inspection,
    const SegelKonfigurationSnapshot& snapshot,
    const SegelAktionAnforderung& anforderung,
    const std::string& erwartete_revision);
SegelKonfigurationErgebnis stelleSegelStellwinkelVorschauDar(
    const SegelKonfigurationSnapshot& snapshot, const std::string& element_id,
    double stellwinkel_grad);

}
