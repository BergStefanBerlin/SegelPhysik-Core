#pragma once

#include <string_view>

namespace segelphysik {

struct BuildKonfiguration {
    bool testinstrumentierung_vorbereitet;
    std::string_view status;
};

BuildKonfiguration build_konfiguration() noexcept;
std::string_view fehlerdokumentation_json() noexcept;

}
