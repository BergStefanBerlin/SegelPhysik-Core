#include "segelphysik/simulation.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

int main(int argc,char* argv[])
{
    if(argc!=2) return 2;
    const auto base=std::filesystem::temp_directory_path()/
        ("segel-core-lauf-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    const auto output=base.string()+".jsonl";
    int status=0;
    try {
#ifdef SEGELPHYSIK_TEST_INSTRUMENTATION
        segelphysik::testmessungen_aktivieren(true);
#endif
        segelphysik::Laufoptionen o;
        o.jsonl=true;
        bool gueltig=false;
        const auto validierung=segelphysik::pruefung_json(argv[1],gueltig);
        if(!gueltig || validierung.find("\"fehler\":[]")==std::string::npos)
            throw std::runtime_error("Gueltiges Modellpruefergebnis fehlt");
        const auto ungueltig=segelphysik::pruefung_json("nicht-vorhandenes-modell.json",gueltig);
        if(gueltig || ungueltig.find("CORE-MOD-0005")==std::string::npos)
            throw std::runtime_error("Strukturierter Modellfehler fehlt");
        auto r=segelphysik::simulieren(argv[1],output,o);
        if(r.status!="konvergiert" || r.zustaende!=1 || r.gesamtlaufzeit_s<0.)
            throw std::runtime_error("Statischer Gesamtlauf fehlgeschlagen: "+r.diagnose);
#ifdef SEGELPHYSIK_TEST_INSTRUMENTATION
        for(const auto* name:{"modellvorbereitung","masseneigenschaften","gleichgewichtssuche",
            "hydrostatikauswertung","ebenenschnitt","ergebnisaufzeichnung"})
            if(r.messungen.at(name).anzahl==0 || r.messungen.at(name).sekunden<0.)
                throw std::runtime_error("Messpunkt fehlt");
#else
        if(!r.messungen.empty()) throw std::runtime_error("Detailmessung im Normalbetrieb");
#endif
        std::ifstream in(output);std::string line;std::getline(in,line);
        if(line.find("\"vektoren\"")==std::string::npos || line.find("\"orientierung_xyzw\"")==std::string::npos)
            throw std::runtime_error("Ergebnisvertrag unvollstaendig");
        in.close();
        const auto refused=segelphysik::simulieren(argv[1],output,o);
        if(refused.status!="fehlgeschlagen"||refused.diagnose.find("CORE-SIM-0001")==std::string::npos)
            throw std::runtime_error("Vorhandene Ausgabe nicht geschuetzt");
        std::ifstream unchanged(output);std::string same;std::getline(unchanged,same);
        if(same!=line) throw std::runtime_error("Vorhandene Datei wurde veraendert");
        unchanged.close();std::filesystem::remove(output);
        o.dynamisch=true;o.dauer_s=.1;o.dt_s=.01;
        r=segelphysik::simulieren(argv[1],output,o);
        if(r.status!="abgeschlossen" || r.zustaende!=11) throw std::runtime_error("Dynamiklauf oder Tickzahl falsch: "+r.diagnose);
#ifdef SEGELPHYSIK_TEST_INSTRUMENTATION
        if(!r.messungen.at("dynamischer_zeitschritt").anzahl) throw std::runtime_error("Dynamikmesspunkt fehlt");
#endif
        std::filesystem::remove(output);
        o.dauer_s=0.;
        r=segelphysik::simulieren(argv[1],output,o);
        if(r.status!="abgeschlossen" || r.zustaende!=1)
            throw std::runtime_error("Leerer Zeitraum muss genau einen Endzustand liefern");
        std::filesystem::remove(output);
        o.dauer_s=.105;
        r=segelphysik::simulieren(argv[1],output,o);
        if(r.status!="abgeschlossen" || r.zustaende!=12)
            throw std::runtime_error("Restschritt fehlt: "+r.diagnose);
        std::filesystem::remove(output);
        for(double startzeit:{std::numeric_limits<double>::quiet_NaN(),
            std::numeric_limits<double>::infinity(),1e100}) {
            o.start.zeit_s=startzeit;
            r=segelphysik::simulieren(argv[1],output,o);
            if(r.status!="fehlgeschlagen" || r.diagnose.find("CORE-SIM-0001")==std::string::npos ||
                std::filesystem::exists(output))
                throw std::runtime_error("Ungueltiger Zeitbereich nicht vor Dateierstellung abgelehnt");
        }
        o.start.zeit_s=0.;
        std::atomic_bool stop{true};o.abbruch=&stop;
        r=segelphysik::simulieren(argv[1],output,o);
        if(r.status!="abgebrochen") throw std::runtime_error("Abbruchstatus fehlt");
        std::filesystem::remove(output);
        r=segelphysik::simulieren("nicht-vorhandenes-modell.json",output,o);
        if(r.status!="fehlgeschlagen"||r.diagnose.empty()||r.gesamtlaufzeit_s<0.)
            throw std::runtime_error("Fehlerlauf nicht gemessen");
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';status=1;}
    std::filesystem::remove(output);
    return status;
}
