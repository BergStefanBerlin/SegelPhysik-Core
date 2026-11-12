#pragma once

#include <filesystem>

namespace segelphysik {

std::filesystem::path laufzeit_defaults_datei();
std::filesystem::path modellordner_standard(
    const std::filesystem::path& expliziter_ordner = {});
std::filesystem::path schemaordner_fuer_modellordner(
    const std::filesystem::path& modellordner,
    const std::filesystem::path& expliziter_schemaordner = {});

}
