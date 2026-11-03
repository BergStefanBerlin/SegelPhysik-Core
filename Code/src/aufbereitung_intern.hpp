#pragma once
#include "segelphysik/aufbereitung.hpp"
#include <mutex>

namespace segelphysik::detail {
struct Weltmesh {
    Vektor3 position{};
    QuaternionXyzw orientierung{};
    std::vector<Vektor3> punkte;
    bool gueltig{};
};
struct Aufbereitungsdaten {
    explicit Aufbereitungsdaten(const Modell& modell) : basis(modell) {}
    const Modell basis;
    std::recursive_mutex mutex;
    std::shared_ptr<Modell> physisch;
    std::vector<Bewegungsstellung> stellungen;
    std::string kraftschluessel;
    std::optional<Kraefte> kraft;
    std::map<const Mesh*,std::array<double,6>> lokale_bounds;
    std::map<const Mesh*,Weltmesh> weltmeshes;
    double zeit_s{};
    std::size_t schritt{};
};
extern thread_local Aufbereitungsdaten* aktive_aufbereitung;
void pruefe_auswertungszustand(const Modell&,const Zustand&);
inline void zaehle_meshkopien(const Element& element)
{
#ifdef SEGELPHYSIK_TEST_INSTRUMENTATION
    if(element.volumen) { SEGEL_MESSUNG("meshkopie"); }
    if(element.massen_mesh) { SEGEL_MESSUNG("meshkopie"); }
#else
    static_cast<void>(element);
#endif
}
inline void zaehle_meshkopien(const Modell& modell)
{
#ifdef SEGELPHYSIK_TEST_INSTRUMENTATION
    for(const auto& element:modell.elemente) zaehle_meshkopien(element);
#else
    static_cast<void>(modell);
#endif
}

// The thread-local pointer only routes a bounded call stack; all cached data
// belong to the owning context and are released with it.
class Aufbereitungsbereich {
public:
    explicit Aufbereitungsbereich(Modellaufbereitung& kontext)
        : daten_(*kontext.daten_), lock_(daten_.mutex),
          vorher_(aktive_aufbereitung)
    { aktive_aufbereitung = &daten_; }
    ~Aufbereitungsbereich() { aktive_aufbereitung = vorher_; }
    void zustand(const Zustand& s) { daten_.zeit_s=s.zeit_s; daten_.schritt=s.schritt; }
private:
    Aufbereitungsdaten& daten_;
    std::unique_lock<std::recursive_mutex> lock_;
    Aufbereitungsdaten* vorher_;
};
}
