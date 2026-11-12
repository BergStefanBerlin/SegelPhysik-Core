#pragma once

#include <nlohmann/json.hpp>

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace segelphysik::detail {

struct Modellformat03Pruefung {
    nlohmann::json daten;
    nlohmann::json validierung;
    std::vector<std::string> fehler;
    std::vector<std::string> fehler_bereiche;
    std::vector<std::string> veraltete_bereiche;
    std::map<std::string, std::string> berechnete_hashes;
    std::string fassung;
    bool schemas_gueltig{};
};

Modellformat03Pruefung pruefe_modellformat03(
    const std::filesystem::path& modellordner,
    const std::filesystem::path& schemaordner,
    bool fingerprints = true);

nlohmann::json normalisiere_modellformat03(
    const nlohmann::json& daten,
    const std::filesystem::path& modellordner);

std::string modellformat03_fassung(const Modellformat03Pruefung& pruefung);

bool ist_modellformat03_datei(const std::filesystem::path& datei);

}
