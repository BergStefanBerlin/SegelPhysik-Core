#include "segelphysik/aufbereitung.hpp"
#include <future>
#include <iostream>
#include <stdexcept>

using namespace segelphysik;
namespace {
void check(bool value,const char* text) { if(!value) throw std::runtime_error(text); }
template<class F> void fehler(F&& f)
{
    bool abgelehnt=false;
    try { f(); } catch(const std::invalid_argument&) { abgelehnt=true; }
    check(abgelehnt,"Ungueltige Eingabe nicht abgelehnt");
}
Zustand start(const Modell& m,int fall)
{
    Zustand s;
    if(fall&1) s=hydrodynamik_start(m,s,{});
    if(fall&2) {
        TestantriebKonfiguration c;
        if(fall==3) {
            c.grenze_m_s=.01;
            s.geschwindigkeit_welt_m_s={.1,0.,0.};
        }
        s=testantrieb_start(m,s,c,true);
    }
    if(fall&4) {
        DynamischeFremdkraftKonfiguration c;
        c.aktiv=(fall&8)!=0; c.kraft_modellfest_n={10.,0.,0.};
        c.angriffspunkt_modell_m=m.schwerpunkt_m;
        s=dynamische_fremdkraft_start(m,s,c);
    }
    return s;
}
void vergleich(const Modell& m,int fall)
{
    Modellaufbereitung cache(m);
    auto a=start(m,fall),b=a;
    const auto historisch=a;
    std::vector<std::string> bewegungen;
    for(const auto& e:m.elemente) if(e.bewegung) bewegungen.push_back(e.id);
    for(int i=0;i<4;++i) {
        std::vector<Bewegungsereignis> ereignisse;
        if(!bewegungen.empty() && i>0) ereignisse.push_back(
            {static_cast<std::size_t>(i),
             bewegungen[i==3?bewegungen.size()-1:0],50.});
        a=dynamikschritt(m,a,.01,{},ereignisse);
        b=cache.schritt(b,.01,{},ereignisse);
        check(zustand_json(m,a,"laufend")==cache.json(b,"laufend"),
            "JSON/Physik/Stellung/CG/Traegheitskontext nicht identisch");
        const auto einmal=cache.json(b,"laufend");
        cache.auswertung(b);
        check(einmal==cache.json(b,"laufend"),"Ergebniswiederverwendung veraendert JSON");
    }
    if(fall==3) check(b.testantrieb->uebergang->begrenzung.wirksam,
        "Hydrodynamischer Begrenzerfall nicht ausgefuehrt");
    check(cache.json(historisch,"historisch")==zustand_json(m,historisch,"historisch"),
        "Rueckblick verwendet aktuelle Geometrie");
    for(const auto& parameter:std::vector<std::array<double,2>>{
        {999.,9.81},{1000.,9.7},{1000.,9.81}}) {
        const auto k=cache.auswertung(historisch,parameter[0],parameter[1]);
        const auto r=kraefte(m,historisch.pose,historisch.testantrieb.get(),
            parameter[0],parameter[1],historisch.geflutet,historisch.hydrodynamik.get(),
            historisch.geschwindigkeit_welt_m_s,historisch.omega_modell_rad_s,
            historisch.bewegungsstellungen,historisch.dynamische_fremdkraft.get());
        check(k.kraft_welt_n==r.kraft_welt_n && k.moment_modell_nm==r.moment_modell_nm &&
            k.schwerpunkt_modell_m==r.schwerpunkt_modell_m && k.volumen_m3==r.volumen_m3,
            "rho/g-Konfigurationswechsel wird nicht invalidiert");
    }
    if(historisch.testantrieb) {
        auto anders=historisch;
        auto c=std::make_shared<TestantriebKontext>(*anders.testantrieb);
        c->konfiguration.kraft_n+=11.;
        c->aktiv=!c->aktiv; anders.testantrieb=c;
        check(cache.json(anders,"laufend")==zustand_json(m,anders,"laufend"),
            "Testantriebskonfiguration wird nicht invalidiert");
    }
    if(historisch.dynamische_fremdkraft) {
        auto anders=historisch;
        auto c=std::make_shared<DynamischeFremdkraftKontext>(*anders.dynamische_fremdkraft);
        c->konfiguration.kraft_modellfest_n={21.,3.,1.};
        c->konfiguration.angriffspunkt_modell_m[0]+=.2;
        c->konfiguration.aktiv=!c->konfiguration.aktiv; anders.dynamische_fremdkraft=c;
        check(cache.json(anders,"laufend")==zustand_json(m,anders,"laufend"),
            "Fremdkraftkonfiguration wird nicht invalidiert");
    }
    for(int aenderung=0;aenderung<5;++aenderung) {
        auto anders=historisch;
        if(aenderung==0) anders.pose=Pose{{.1,.2,.3},{0.,0.,0.,1.}};
        if(aenderung==1) anders.geschwindigkeit_welt_m_s={.3,.1,-.2};
        if(aenderung==2) anders.omega_modell_rad_s={.01,.02,.03};
        if(aenderung==3 && !m.elemente.empty()) anders.geflutet={m.elemente.front().id};
        if(aenderung==4) {
            anders.zeit_s+=1.;
            if(anders.hydrodynamik) {
                auto c=std::make_shared<HydrodynamikKontext>(*anders.hydrodynamik);
                c->auswertung_zeit_s=anders.zeit_s; anders.hydrodynamik=c;
            }
            if(anders.testantrieb) {
                auto c=std::make_shared<TestantriebKontext>(*anders.testantrieb);
                c->auswertung_zeit_s=anders.zeit_s; anders.testantrieb=c;
            }
            if(anders.dynamische_fremdkraft) {
                auto c=std::make_shared<DynamischeFremdkraftKontext>(*anders.dynamische_fremdkraft);
                c->auswertung_zeit_s=anders.zeit_s; anders.dynamische_fremdkraft=c;
            }
        }
        check(cache.json(anders,"laufend")==zustand_json(m,anders,"laufend"),
            "Pose/Geschwindigkeit/Rotation/Flutung/Zeit falsch wiederverwendet");
    }
    auto ungueltig=b;
    ungueltig.dynamische_fremdkraft_erforderlich=true;
    ungueltig.dynamische_fremdkraft.reset();
    fehler([&] {cache.json(ungueltig,"ungueltig");});
    check(cache.json(b,"laufend")==zustand_json(m,b,"laufend"),
        "Fehlerpfad vergiftet Cache");
    if(b.hydrodynamik) {
        auto kontext=std::make_shared<HydrodynamikKontext>(*b.hydrodynamik);
        kontext->konfiguration.fluidgeschwindigkeit_welt_m_s={.2,.1,0.};
        b.hydrodynamik=kontext;
        check(cache.json(b,"laufend")==zustand_json(m,b,"laufend"),
            "Fluidkonfiguration invalidiert Kraefte nicht");
        kontext=std::make_shared<HydrodynamikKontext>(*b.hydrodynamik);
        kontext->auswertung_zeit_s+=1.;
        b.hydrodynamik=kontext;
        fehler([&] {cache.json(b,"ungueltig");});
    }
}
}
int main(int argc,char** argv)
{
    if(argc!=3) return 2;
    try {
        for(int model=1;model<argc;++model) {
            auto m=lade_modell(argv[model]);
            for(int fall:{0,1,2,3,4,5,12,13}) vergleich(m,fall);
            Modellaufbereitung original(m);
            const auto unveraendert=original.json({},"laufend");
            m.elemente.front().schwerpunkt_m[0]+=.5;
            if(m.elemente.front().volumen)
                m.elemente.front().volumen->punkte.front()[2]+=.01;
            check(original.json({},"laufend")==unveraendert,
                "Unveraenderliche Basis teilt mutable Quelldaten");
            Modellaufbereitung neu(m);
            check(neu.json({},"laufend")==zustand_json(m,{},"laufend"),
                "Neues Modell bei gleicher Fassung ignoriert");
            auto parallel=[m] {Modellaufbereitung c(m);auto s=start(m,1);
                s=c.schritt(s,.01);return c.json(s,"laufend");};
            auto f=std::async(std::launch::async,parallel);
            check(f.get()==parallel(),"Parallele Laeufe teilen veraenderliche Daten");
        }
#ifdef SEGELPHYSIK_TEST_INSTRUMENTATION
        const auto m=lade_modell(argv[1]);
        auto s=start(m,1);
        testmessungen_aktivieren(true); testmessungen_zuruecksetzen();
        auto ungebunden=s;
        for(int i=0;i<4;++i) {
            ungebunden=dynamikschritt(m,ungebunden,.01);
            zustand_json(m,ungebunden,"laufend");
            zustand_json(m,ungebunden,"laufend");
        }
        const auto referenzzaehler=testmessungen();
        check(referenzzaehler.at("stellungsaufbereitung").anzahl>1,
            "Ungebundener Kontrollpfad fehlt");
        for(const auto& wert:referenzzaehler)
            std::cout<<"ungebunden."<<wert.first<<": "<<wert.second.anzahl<<'\n';
        testmessungen_zuruecksetzen();
        {
            Modellaufbereitung cache(m);
            for(int i=0;i<4;++i) {
                s=cache.schritt(s,.01);
                cache.json(s,"laufend");
                cache.json(s,"laufend");
            }
            const auto zaehler=testmessungen();
            for(const auto& wert:zaehler)
                std::cout<<wert.first<<": "<<wert.second.anzahl<<'\n';
            check(zaehler.at("modellbasis_aufbau").anzahl==1,"Basis mehrfach aufgebaut");
            check(zaehler.at("stellungsaufbereitung").anzahl==1,"Feste Stellung erneut aufbereitet");
            check(zaehler.at("modell_geometriekopie").anzahl==1,"Modell mehrfach tief kopiert");
            check(zaehler.at("kraft_ergebnis_wiederverwendung").anzahl>=8,
                "Identische Endauswertung nicht geteilt");
            check(zaehler.at("ebenenschnitt").anzahl>4,"Notwendige Schnitte fehlen");
            std::string id;
            for(const auto& e:m.elemente) if(e.bewegung) {id=e.id;break;}
            s=cache.schritt(s,.01,{},{{1,id,50.}});
            const auto nach_aenderung=testmessungen();
            s=cache.schritt(s,.01,{},{{2,id,50.}});
            const auto nach_identisch=testmessungen();
            check(s.letzte_bewegungsereigniskennung==2 &&
                s.bewegungsuebergang->ereignisse.front().kennung==2,
                "Identischer Stellwert verliert Quittierung/Historie");
            check(nach_aenderung.at("stellungsaufbereitung").anzahl==2 &&
                nach_identisch.at("stellungsaufbereitung").anzahl==2,
                "Identisches Ereignis bereitet Geometrie erneut auf");
            check(nach_aenderung.at("mesh_transformation").anzahl==
                nach_identisch.at("mesh_transformation").anzahl,
                "Identisches Ereignis transformiert Mesh erneut");
            for(const auto& e:m.elemente) {
                if(!e.bewegung || e.bewegung->mitbewegte_elemente.empty()) continue;
                const auto davor=testmessungen();
                s=cache.schritt(s,.01,{},{{3,e.id,25.}});
                const auto danach=testmessungen();
                const auto anzahl=[](const Messwerte& w,const char* name) {
                    const auto f=w.find(name);return f==w.end()?0:f->second.anzahl;
                };
                check(anzahl(danach,"element_geometriekopie")-
                    anzahl(davor,"element_geometriekopie")==
                    1+e.bewegung->mitbewegte_elemente.size(),
                    "Unveraenderte Elemente kopiert oder mitbewegte Kinder vergessen");
                break;
            }
        }
        testmessungen_aktivieren(false);
#endif
        std::cout<<"Modellaufbereitung: alle Referenzen bestanden\n";
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
