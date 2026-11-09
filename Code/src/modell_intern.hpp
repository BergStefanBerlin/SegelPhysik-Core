#pragma once

#include "segelphysik/modell.hpp"

#include <optional>
#include <string>

namespace segelphysik {

Mesh lade_quellmesh(const std::filesystem::path& datei,
                   const std::optional<std::string>& objekt = std::nullopt,
                   bool darstellung = false);

const nlohmann::json* geometrieableitung(const ModellBeschreibung& beschreibung,
    const std::string& element_id, const std::string& verwendung);
Mesh abgeleitetes_mesh(const Mesh& quelle, const nlohmann::json& definition);
const nlohmann::json* element_setting(const nlohmann::json& dokument,
                                     const std::string& element_id);
nlohmann::json normalisiere_modell_information(const nlohmann::json& dokument);
nlohmann::json aktualisiere_modell_information(const nlohmann::json& information,
                                               const nlohmann::json& core_dokument);
Pose setting_pose(const nlohmann::json& setting);
void validiere_element_settings(const nlohmann::json& dokument,
                                const std::string& kontext = "modells_information.json");

struct ModellInspektionsImport {
    ModellBeschreibung beschreibung;
    std::optional<Modell> modell;
    std::string import_fehler;
};

ModellInspektionsImport lade_inspektionsimport(const std::filesystem::path& datei);

}
