#include "segelphysik/core.hpp"

#include <iostream>

int main()
{
    const auto konfiguration = segelphysik::build_konfiguration();
#ifdef SEGELPHYSIK_EXPECT_INSTRUMENTATION
    constexpr bool erwartet = true;
#else
    constexpr bool erwartet = false;
#endif
    if (konfiguration.testinstrumentierung_vorbereitet != erwartet) {
        std::cerr << "Falsche Buildkonfiguration der Bibliothek.\n";
        return 1;
    }
    if (konfiguration.status !=
        "Entwicklungsstand: Hydrostatikkern; fachliche Gesamtabnahme offen") {
        std::cerr << "Der Implementierungsstand muss eindeutig ausgewiesen sein.\n";
        return 1;
    }
    return 0;
}
