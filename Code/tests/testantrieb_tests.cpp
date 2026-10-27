#include "segelphysik/simulation.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
using namespace segelphysik;
void check(bool value,const char* message) {if(!value) throw std::runtime_error(message);}
void nahe(double a,double b,double eps=1e-8) {check(std::abs(a-b)<=eps,"Numerische Referenz verfehlt");}
template<class F> void fehler(F f) {
    bool abgelehnt=false;
    try {f();} catch(const std::invalid_argument& e) {
        abgelehnt=std::string(e.what()).find("CORE-SIM-0001")!=std::string::npos;
    }
    check(abgelehnt,"Explizite CORE-SIM-0001 Diagnose fehlt");
}
Modell referenz() {
    Modell m;m.id="test";m.fassung="physik-1";m.masse_kg=10.;
    m.traegheit_kg_m2={100.,0.,0.,0.,100.,0.,0.,0.,100.};
    Element e;e.id="koerper";e.masse_kg=10.;
    e.massenpunkte_m={{-2.,0.,0.},{2.,-1.,1.},{2.,1.,1.},{2.,1.,1.}};
    m.elemente.push_back(e);return m;
}
void gleich(const Zustand& a,const Zustand& b) {
    for(std::size_t i=0;i<3;++i) {
        nahe(a.geschwindigkeit_welt_m_s[i],b.geschwindigkeit_welt_m_s[i],1e-12);
        nahe(a.omega_modell_rad_s[i],b.omega_modell_rad_s[i],1e-12);
        nahe(a.pose.position_m()[i],b.pose.position_m()[i],1e-12);
    }
    for(std::size_t i=0;i<4;++i) nahe(a.pose.orientierung_xyzw()[i],b.pose.orientierung_xyzw()[i],1e-12);
    check(a.geflutet==b.geflutet,"Flutungsparitaet");
}
}
int main(int argc,char* argv[]) {
    if(argc!=3) return 2;
    const std::filesystem::path output="testantrieb-vertrag.jsonl";
    try {
        auto m=referenz();Zustand start;
        const auto initial=testantrieb_start(m,start,{},true);
        check(initial.testantrieb&&!initial.testantrieb->uebergang,"Start hat keinen Uebergang");
        nahe((*initial.testantrieb->zugpunkt_modell_m)[0],2.);
        nahe((*initial.testantrieb->zugpunkt_modell_m)[1],0.);
        nahe((*initial.testantrieb->zugpunkt_modell_m)[2],1.);
        const auto k=kraefte(m,start.pose,initial.testantrieb.get());
        nahe(k.kraft_welt_n[0],200.);nahe(k.kraft_welt_n[1],0.);
        nahe(k.moment_modell_nm[1],200.);
        auto beitraege=k.beitraege;const auto b=bilanz(m,beitraege);
        nahe(b.gesamt.moment_s_nm[1],200.);
        check(b.status=="vollstaendig"&&b.erwartete_beitraege.size()==2,"Testbeitrag Bilanzumfang");
        for(const auto& pose:std::vector<Pose>{
            Pose{{3.,4.,5.},{0.,0.,std::sin(.4),std::cos(.4)}},
            Pose{{},{0.,std::sin(.3),0.,std::cos(.3)}},
            Pose{{},{std::sin(.2),0.,0.,std::cos(.2)}}}) {
            const auto force=kraefte(m,pose,initial.testantrieb.get());
            const auto axis=pose.modellvektor_nach_welt({1.,0.,0.});
            const double n=std::hypot(axis[0],axis[1]);
            nahe(force.kraft_welt_n[0],200.*axis[0]/n);
            nahe(force.kraft_welt_n[1],200.*axis[1]/n);
            const auto expected=moment_um({2.,0.,1.},m.schwerpunkt_m,
                pose.weltvektor_nach_modell({200.*axis[0]/n,200.*axis[1]/n,0.}));
            for(std::size_t i=0;i<3;++i) nahe(force.moment_modell_nm[i],expected[i]);
        }
        auto vertical=start;vertical.pose=Pose{{},{0.,std::sqrt(.5),0.,std::sqrt(.5)}};
        fehler([&]{testantrieb_start(m,vertical,{},true);});
        testantrieb_start(m,vertical,{0.,5.},true);
        auto missing=m;missing.elemente.front().massenpunkte_m.clear();
        fehler([&]{testantrieb_start(missing,start,{},true);});
        testantrieb_start(missing,start,{},false);
        testantrieb_start(missing,start,{0.,5.},true);
        const auto missingzero=testantrieb_start(missing,start,{0.,5.},true);
        check(zustand_json(missing,missingzero,"laufend").find("\"zugpunkt_modell_m\":null")!=std::string::npos,"Erfundener Zugpunkt");
        auto point=m;point.elemente.front().massenpunkte_m={{7.,2.,3.}};
        const auto pointstart=testantrieb_start(point,start,{},true);
        check(*pointstart.testantrieb->zugpunkt_modell_m==Vektor3{7.,2.,3.},"Punktmasse als Bug");
        for(double f:{-1.,100001.,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()})
            fehler([&]{testantrieb_start(m,start,{f,5.},false);});
        for(double v:{0.,.009,20.01,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()})
            fehler([&]{testantrieb_start(m,start,{200.,v},false);});
        testantrieb_start(m,start,{100000.,.01},false);
        testantrieb_start(m,start,{0.,20.},false);
        auto volume=m;const auto physical=lade_modell(argv[1]);
        const auto found=std::find_if(physical.elemente.begin(),physical.elemente.end(),
            [](const Element& e){return e.volumen.has_value();});
        check(found!=physical.elemente.end(),"Volumenreferenz fehlt");
        auto mesh=*found->volumen;
        double maxx=-std::numeric_limits<double>::infinity();
        for(auto& p:mesh.punkte) {p[0]+=10.;maxx=std::max(maxx,p[0]);}
        volume.elemente.front().volumen=mesh;
        // Geometry selection is independent of the waterline and deduplicates
        // physical points appearing in both mesh sources.
        auto geometry=start;geometry.pose=Pose{{0.,0.,100.},{0.,0.,0.,1.}};
        const auto volumecontext=testantrieb_start(volume,geometry,{},false);
        nahe((*volumecontext.testantrieb->zugpunkt_modell_m)[0],maxx);
        auto near=m;near.elemente.front().massenpunkte_m.push_back({2.,1.+1e-10,1.});
        nahe((*testantrieb_start(near,start,{},false).testantrieb->zugpunkt_modell_m)[1],0.);
        const auto off=testantrieb_start(m,start,{},false);
        gleich(dynamikschritt(m,start,.01),dynamikschritt(m,off,.01));
        auto rapid=start;rapid.geschwindigkeit_welt_m_s={6.,0.,1e200};
        gleich(dynamikschritt(m,rapid,.01),
            dynamikschritt(m,testantrieb_start(m,rapid,{0.,5.},false),.01));
        const auto rapidcapped=dynamikschritt(m,testantrieb_start(m,rapid,{0.,5.},true),.01);
        nahe(rapidcapped.testantrieb->uebergang->begrenzung.energieaenderung_j,-55.);
        auto constant=m;constant.elemente.front().massenpunkte_m={{2.,0.,0.}};
        auto turn=start;turn.omega_modell_rad_s={0.,std::acos(-1.)/.02,0.};
        const auto turning=testantrieb_start(constant,turn,{},true);
        const auto turninghistory=zustand_json(constant,turning,"laufend");
        fehler([&]{dynamikschritt(constant,turning,.01);});
        check(zustand_json(constant,turning,"laufend")==turninghistory,
            "Singulaerer Endzustand veraendert veroeffentlichte Historie");
        const auto pushed=dynamikschritt(constant,testantrieb_start(constant,start,{},true),.01);
        nahe(pushed.geschwindigkeit_welt_m_s[0],.2,1e-12);
        nahe(pushed.pose.position_m()[0],.001,1e-12);
        nahe(pushed.omega_modell_rad_s[1],0.);
        const auto torque=dynamikschritt(m,initial,.01);
        check(torque.omega_modell_rad_s[1]>0.,"Echtes Hebelarmmoment fehlt");
        check(torque.geschwindigkeit_welt_m_s[0]>0.,"Schub fehlt an Mittellage");
        for(double speed:{4.,5.,6.}) {
            auto fast=start;fast.geschwindigkeit_welt_m_s={.6*speed,.8*speed,2.};
            fast.omega_modell_rad_s={.1,.2,.3};
            const auto free=dynamikschritt(m,fast,.01);
            const auto capped=dynamikschritt(m,testantrieb_start(m,fast,{0.,5.},true),.01);
            nahe(std::hypot(capped.geschwindigkeit_welt_m_s[0],capped.geschwindigkeit_welt_m_s[1]),std::min(speed,5.),1e-10);
            nahe(capped.geschwindigkeit_welt_m_s[2],free.geschwindigkeit_welt_m_s[2],1e-12);
            check(capped.pose.position_m()==free.pose.position_m(),"Begrenzer aendert Position");
            check(capped.omega_modell_rad_s==free.omega_modell_rad_s,"Begrenzer aendert omega");
            const auto& limit=capped.testantrieb->uebergang->begrenzung;
            check(limit.wirksam==(speed>5.),"Begrenzerflag");
            nahe(limit.korrekturimpuls_ns[0],10.*(capped.geschwindigkeit_welt_m_s[0]-free.geschwindigkeit_welt_m_s[0]));
            nahe(limit.energieaenderung_j,.5*10.*(std::min(speed,5.)*std::min(speed,5.)-speed*speed));
            const auto inactive=dynamikschritt(m,testantrieb_start(m,fast,{0.,5.},false),.01);
            gleich(inactive,free);
            const auto untouched=testantrieb_start(m,fast,{0.,5.},true);
            check(untouched.geschwindigkeit_welt_m_s==fast.geschwindigkeit_welt_m_s,"Start wurde begrenzt");
        }
        const auto history=zustand_json(m,off,"laufend");
        const auto live=dynamikschritt(m,off,.01,std::vector<TestantriebEreignis>{{1,true},{2,false},{3,true}});
        check(live.testantrieb->aktiv&&live.testantrieb->uebergang->ereignisse.size()==3,"Ereignisreihenfolge");
        check(live.testantrieb->uebergang->von_schritt==0&&live.testantrieb->uebergang->nach_schritt==1,"Grenzbestaetigung");
        check(history==zustand_json(m,off,"laufend"),"Historie umgedeutet");
        check(history.find("\"vertragsversion\":\"3\"")!=std::string::npos,"Inaktive V3");
        check(history.find("\"uebergang\":null")!=std::string::npos,"Startnullabbildung");
        check(zustand_json(m,start,"laufend").find("\"vertragsversion\":\"2\"")!=std::string::npos,"V2 geaendert");
        const auto disabled=dynamikschritt(m,live,.01,std::vector<TestantriebEreignis>{{4,false}});
        check(!disabled.testantrieb->aktiv,"Ausschalten");
        const auto livejson=zustand_json(m,live,"laufend");
        check(livejson.find("\"kennung\":3")!=std::string::npos&&livejson.find("\"begrenzung\"")!=std::string::npos,"V3 Pflichtdaten");
        fehler([&]{dynamikschritt(m,live,.01,std::vector<TestantriebEreignis>{{3,false}});});
        fehler([&]{dynamikschritt(m,disabled,.01,std::vector<TestantriebEreignis>{{4,true}});});
        const auto later=dynamikschritt(m,disabled,.01,std::vector<TestantriebEreignis>{{5,true}});
        check(later.testantrieb->letzte_ereigniskennung==5&&
            later.testantrieb->uebergang->von_schritt==2&&
            later.testantrieb->uebergang->ereignisse.front().kennung==5,
            "Folgeereignis nach abgeschlossenem Ereignisschritt");
        fehler([&]{dynamikschritt(m,start,.01,std::vector<TestantriebEreignis>{{1,true}});});
        auto lost=live;lost.testantrieb.reset();
        fehler([&]{zustand_json(m,lost,"laufend");});
        fehler([&]{dynamikschritt(m,lost,.01);});
        fehler([&]{gleichgewicht(m,initial);});
        auto wrong=live;wrong.schritt++;
        fehler([&]{zustand_json(m,wrong,"laufend");});
        auto changedmodel=m;changedmodel.fassung="physik-2";
        fehler([&]{zustand_json(changedmodel,live,"laufend");});
        fehler([&]{dynamikschritt(changedmodel,live,.01);});
        fehler([&]{kraefte(changedmodel,live.pose,live.testantrieb.get());});
        const auto imported=lade_modell(argv[1]);
        check(!imported.elemente.front().massenpunkte_m.empty(),"Import verwirft Massenpunkte");
        Laufoptionen o;o.dynamisch=true;o.dauer_s=.03;o.dt_s=.01;o.testantrieb=TestantriebKonfiguration{};
        o.testantrieb_aktiv=true;o.testantrieb_ereignisse={{1,{1,false}},{2,{2,true}},{3,{3,false}}};
        check(!std::filesystem::exists(output),"Testausgabe existiert bereits");
        const auto report=simulieren(argv[1],output,o);
        check(report.status=="abgeschlossen"&&report.zustaende==4,"Batch-Testlauf");
        auto direct=testantrieb_start(imported,o.start,*o.testantrieb,true);
        std::ifstream in(output);std::string line;std::getline(in,line);
        check(line==zustand_json(imported,direct,"laufend"),"Batch-Startparitaet");
        for(std::size_t i=0;i<3;++i) {
            std::vector<TestantriebEreignis> events;
            for(const auto& e:o.testantrieb_ereignisse) if(e.first==i) events.push_back(e.second);
            const double ziel=static_cast<double>(i+1)*o.dt_s;
            direct=dynamikschritt(imported,direct,ziel-direct.zeit_s,events);
            std::getline(in,line);
            check(line==zustand_json(imported,direct,i==2?"abgeschlossen":"laufend"),"Batch-Schrittparitaet");
        }
        check(line.find("\"kennung\":3")==std::string::npos,"Ereignis nach Laufende bestaetigt");
        in.close();std::filesystem::remove(output);
        Laufoptionen explicitoff;explicitoff.dynamisch=true;
        explicitoff.dauer_s=.02;explicitoff.dt_s=.01;
        explicitoff.testantrieb=TestantriebKonfiguration{};
        check(!explicitoff.testantrieb_aktiv,"Defaultflag nicht inaktiv");
        check(simulieren(argv[1],output,explicitoff).status=="abgeschlossen","Explizit inaktiver Batch");
        std::ifstream inactivefile(output);
        std::size_t inactivecount=0;
        while(std::getline(inactivefile,line)) {
            check(line.find("\"vertragsversion\":\"3\"")!=std::string::npos&&
                line.find("\"testantrieb\":{")!=std::string::npos&&
                line.find("\"aktiv\":false")!=std::string::npos,"Inaktiver Batch verliert V3-Kontext");
            ++inactivecount;
        }
        check(inactivecount==3,"Inaktive Batchzustandszahl");
        inactivefile.close();std::filesystem::remove(output);
        explicitoff.testantrieb_ereignisse={{0,{1,true}},{1,{1,false}}};
        const auto duplicate=simulieren(argv[1],output,explicitoff);
        check(duplicate.status=="fehlgeschlagen"&&duplicate.diagnose.find("CORE-SIM-0001")!=std::string::npos&&
            !std::filesystem::exists(output),"Schrittuebergreifende doppelte Batchkennung");
        explicitoff.testantrieb_ereignisse.clear();
        explicitoff.testantrieb=TestantriebKonfiguration{0.,5.};
        explicitoff.testantrieb_aktiv=true;
        explicitoff.start.geschwindigkeit_welt_m_s={6.,0.,0.};
        check(simulieren(argv[1],output,explicitoff).status=="abgeschlossen","Aktiver Nullkraftbatch");
        std::ifstream zerofile(output);std::getline(zerofile,line);
        check(line.find("\"uebergang\":null")!=std::string::npos,"Nullkraftbatch begrenzt Start");
        std::getline(zerofile,line);
        check(line.find("\"wirksam\":true")!=std::string::npos&&
            line.find("\"geschwindigkeit_welt_m_s\":[5,0,")!=std::string::npos,
            "Nullkraftbatch begrenzt ersten Endzustand nicht");
        zerofile.close();std::filesystem::remove(output);
        std::atomic_bool stop{true};o.abbruch=&stop;
        const auto cancelled=simulieren(argv[1],output,o);
        check(cancelled.status=="abgebrochen"&&cancelled.zustaende==2,"Abbruch erhaelt Startzustand");
        std::ifstream aborted(output);std::string contents((std::istreambuf_iterator<char>(aborted)),{});
        check(contents.find("\"kennung\":1")==std::string::npos,"Abbruch bestaetigt unappliziertes Ereignis");
        aborted.close();std::filesystem::remove(output);o.abbruch=nullptr;
        o.dynamisch=false;
        check(simulieren(argv[1],output,o).status=="fehlgeschlagen"&&!std::filesystem::exists(output),"Aktive Batch-Statik");
        const auto boot=lade_modell(argv[2]);
        auto s=testantrieb_start(boot,start,{},true);
        for(int i=0;i<10;++i) s=dynamikschritt(boot,s,.001);
        check(std::isfinite(s.geschwindigkeit_welt_m_s[0])&&s.geschwindigkeit_welt_m_s[0]>0.,"420er Integrationslauf");
        auto fastboot=start;fastboot.geschwindigkeit_welt_m_s={6.,0.,0.};
        s=testantrieb_start(boot,fastboot,{},true);
        std::ofstream reference("testantrieb-420-referenz.jsonl",std::ios::binary);
        reference<<zustand_json(boot,s,"laufend")<<'\n';
        s=dynamikschritt(boot,s,.001);
        check(s.testantrieb->uebergang->begrenzung.wirksam,"420er Begrenzungsnachweis");
        reference<<zustand_json(boot,s,"laufend")<<'\n';
        s=dynamikschritt(boot,s,.001,std::vector<TestantriebEreignis>{{1,false}});
        reference<<zustand_json(boot,s,"laufend")<<'\n';
        s=dynamikschritt(boot,s,.001,std::vector<TestantriebEreignis>{{2,true}});
        reference<<zustand_json(boot,s,"abgeschlossen")<<'\n';
        reference.close();check(!reference.fail(),"Referenzaufzeichnung nicht schreibbar");
        std::cout<<"Testantrieb: Geometrie, Kraft/Moment, Integration, Grenze, Ereignisse, V2/V3, Batch und 420 bestanden.\n";
    } catch(const std::exception& e) {
        std::cerr<<e.what()<<'\n';return 1;
    }
    return 0;
}
