#pragma once
#include <chrono>
#include <map>
#include <string>

namespace segelphysik {
struct Messwert { std::size_t anzahl{}; double sekunden{}; };
using Messwerte = std::map<std::string, Messwert>;
void testmessungen_aktivieren(bool aktiv);
void testmessungen_zuruecksetzen();
Messwerte testmessungen();

class Messbereich {
public:
    explicit Messbereich(const char* name);
    ~Messbereich();
    Messbereich(const Messbereich&) = delete;
    Messbereich& operator=(const Messbereich&) = delete;
private:
    const char* name_;
    bool aktiv_;
    std::chrono::steady_clock::time_point start_;
};
}

#ifdef SEGELPHYSIK_TEST_INSTRUMENTATION
#define SEGEL_MESSUNG(name) segelphysik::Messbereich segel_messbereich{name}
#else
#define SEGEL_MESSUNG(name) static_cast<void>(0)
#endif
