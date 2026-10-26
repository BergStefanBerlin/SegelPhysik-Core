#include "segelphysik/core.hpp"
#include "fehlerkatalog.hpp"

namespace segelphysik {

BuildKonfiguration build_konfiguration() noexcept
{
#ifdef SEGELPHYSIK_TEST_INSTRUMENTATION
    constexpr bool instrumentierung = true;
#else
    constexpr bool instrumentierung = false;
#endif
    return {instrumentierung, "Entwicklungsstand: Hydrostatikkern; fachliche Gesamtabnahme offen"};
}

std::string_view fehlerdokumentation_json() noexcept
{
    return intern::fehlerkatalog_json;
}

}
