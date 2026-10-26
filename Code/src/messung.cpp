#include "segelphysik/messung.hpp"
#include <stdexcept>

namespace segelphysik {
namespace {
thread_local bool aktiv = false;
thread_local Messwerte werte;
}
void testmessungen_aktivieren(bool an)
{
#ifndef SEGELPHYSIK_TEST_INSTRUMENTATION
    if (an) throw std::invalid_argument("CORE-SIM-0001: Build enthaelt keine Testinstrumentierung.");
#endif
    aktiv=an;
}
void testmessungen_zuruecksetzen() { werte.clear(); }
Messwerte testmessungen() { return werte; }
Messbereich::Messbereich(const char* name) : name_(name),aktiv_(aktiv)
{
    if (aktiv_) {
        werte.try_emplace(name_);
        start_=std::chrono::steady_clock::now();
    }
}
Messbereich::~Messbereich()
{
    if (aktiv_) {
        auto& w=werte.find(name_)->second;
        ++w.anzahl;
        w.sekunden+=std::chrono::duration<double>(std::chrono::steady_clock::now()-start_).count();
    }
}
}
