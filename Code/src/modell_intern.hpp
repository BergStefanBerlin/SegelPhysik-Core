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

struct ModellInspektionsImport {
    ModellBeschreibung beschreibung;
    std::optional<Modell> modell;
    std::string import_fehler;
};

ModellInspektionsImport lade_inspektionsimport(const std::filesystem::path& datei);

}
