#include "segelphysik/core.hpp"
#include "segelphysik/simulation.hpp"

#include <iostream>
#include <iomanip>
#include <charconv>
#include <cmath>
#include <csignal>
#include <string_view>

namespace {
std::atomic_bool abbruch{false};
static_assert(std::atomic_bool::is_always_lock_free, "Signal cancellation requires lock-free atomics");
void abbrechen(int) { abbruch.store(true); }

void hilfe(std::ostream& ausgabe)
{
    ausgabe << "SegelPhysik-Core\n"
               "Aufruf: segelphysik_cli --help | --build-info | --fehlerdoku\n"
               "        segelphysik_cli --pruefen MODELL\n"
               "        segelphysik_cli --statisch MODELL AUSGABE\n"
               "        segelphysik_cli --dynamisch MODELL AUSGABE DAUER_S DT_S\n";
}

double zahl(const char* input)
{
    const std::string_view s{input};
    double value{};
    const auto result=std::from_chars(s.data(),s.data()+s.size(),value);
    if(result.ec!=std::errc{} || result.ptr!=s.data()+s.size() || !std::isfinite(value))
        throw std::invalid_argument("CORE-SIM-0001: Ungueltiges numerisches Argument.");
    return value;
}
}

int main(int argc, char* argv[])
{
    if(argc>=2 && std::string_view{argv[1]}=="--pruefen") {
        if(argc!=3) {std::cerr<<"Fehler: CORE-CLI-0001: --pruefen erwartet eine Modelldatei.\n";return 2;}
        bool gueltig=false;
        std::cout<<segelphysik::pruefung_json(argv[2],gueltig)<<'\n';
        return gueltig?0:1;
    }
    if (argc>=2 && (std::string_view{argv[1]}=="--statisch" || std::string_view{argv[1]}=="--dynamisch")) {
        const bool dynamisch=std::string_view{argv[1]}=="--dynamisch";
        if(argc!=(dynamisch?6:4)) {
            std::cerr<<"Fehler: CORE-CLI-0001: Falsche Argumentzahl fuer Simulation.\n";return 2;
        }
        segelphysik::Laufoptionen optionen;optionen.dynamisch=dynamisch;
        optionen.jsonl=std::filesystem::path{argv[3]}.extension()==".jsonl";
        optionen.abbruch=&abbruch;
        std::signal(SIGINT,abbrechen);
        try {
            if(dynamisch) {optionen.dauer_s=zahl(argv[4]);optionen.dt_s=zahl(argv[5]);}
        } catch(const std::invalid_argument& e) {
            std::cerr<<"Fehler: "<<e.what()<<'\n';return 2;
        }
        const auto bericht=segelphysik::simulieren(argv[2],argv[3],optionen);
        std::cout<<std::setprecision(17)<<"Status: "<<bericht.status
                 <<"\nGesamtlaufzeit_s: "<<bericht.gesamtlaufzeit_s<<"\nZustaende: "<<bericht.zustaende<<'\n';
        if(!bericht.diagnose.empty()) std::cerr<<"Diagnose: "<<bericht.diagnose<<'\n';
        return bericht.status=="konvergiert"||bericht.status=="abgeschlossen"?0:1;
    }
    if (argc != 2) {
        std::cerr << "Fehler: CORE-CLI-0001: Genau ein CLI-Argument erforderlich.\n";
        hilfe(std::cerr);
        return 2;
    }
    const std::string_view argument{argv[1]};
    if (argument == "--fehlerdoku") {
        std::cout << segelphysik::fehlerdokumentation_json();
        return 0;
    }
    if (argument == "--help") {
        hilfe(std::cout);
        return 0;
    }
    if (argument == "--build-info") {
        const auto konfiguration = segelphysik::build_konfiguration();
        std::cout << "SegelPhysik-Core\n"
                  << konfiguration.status << '\n'
                  << "Testinstrumentierung vorbereitet: "
                  << (konfiguration.testinstrumentierung_vorbereitet ? "ja" : "nein")
                  << "\nGesamtlaufzeit immer aktiv; Detailmessungen nur im expliziten Testmodus.\n";
        return 0;
    }
    std::cerr << "Fehler: CORE-CLI-0002: Unbekannter CLI-Befehl: " << argument << '\n';
    hilfe(std::cerr);
    return 2;
}
