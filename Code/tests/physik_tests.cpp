#include "segelphysik/hydrostatik.hpp"
#include "segelphysik/messung.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void near(double a,double b,double tol=1e-9)
{
    if(!std::isfinite(a)||std::abs(a-b)>tol)
        throw std::runtime_error("Referenzabweichung: "+std::to_string(a)+" statt "+std::to_string(b));
}
segelphysik::Mesh box()
{
    return {{{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},
             {-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}},
            {{0,2,1},{0,3,2},{4,5,6},{4,6,7},{0,1,5},{0,5,4},
             {1,2,6},{1,6,5},{2,3,7},{2,7,6},{3,0,4},{3,4,7}}};
}
void pruefen()
{
    using namespace segelphysik;
    const auto mesh=box();
    for(double z:{-2.,-1.,-.5,0.,.5,1.,2.}) {
        const auto v=wasseranteil(mesh,Pose{{0,0,z},{0,0,0,1}});
        const double h=std::max(0.,std::min(2.,1.-z));
        near(v.volumen_m3,4*h);
        if(h>0.) {if(!v.schwerpunkt_m) throw std::runtime_error("Schwerpunkt fehlt");near((*v.schwerpunkt_m)[2],-1.+h/2.);}
        else if(v.schwerpunkt_m) throw std::runtime_error("Nullvolumen mit Schwerpunkt");
    }
    const double s=std::sqrt(.5);
    near(wasseranteil(mesh,Pose{{},{s,0,0,s}}).volumen_m3,4.);
    near(wasseranteil(mesh,Pose{{},{.1,.2,.3,.4}}).volumen_m3,4.);
    const double n=1./std::sqrt(3.);
    const double qw=std::sqrt((1.+n)/2.);
    const QuaternionXyzw diagonal{n/(2.*qw),-n/(2.*qw),0.,qw};
    const auto half=wasseranteil(mesh,Pose{{},diagonal});
    near(half.volumen_m3,4.);
    for(double p:*half.schwerpunkt_m) near(p,-13./48.);
    for(double h:{.1,.001,.00001}) {
        const auto cap=wasseranteil(mesh,Pose{{0,0,(3.-h)*n},diagonal});
        const double expected=h*h*h/6.;
        near(cap.volumen_m3,expected,expected*1e-8);
        if(!cap.schwerpunkt_m) throw std::runtime_error("Kleines Restvolumen verloren");
        for(double p:*cap.schwerpunkt_m) near(p,-1.+h/4.,1e-10);
    }
    auto shifted=mesh;
    for(auto& p:shifted.punkte) {p[0]+=1e6;p[1]-=1e6;}
    const auto shiftedwet=wasseranteil(shifted,Pose{{-1e6,1e6,0},{0,0,0,1}});
    near(shiftedwet.volumen_m3,4.);
    near((*shiftedwet.schwerpunkt_m)[0],1e6);
    near((*shiftedwet.schwerpunkt_m)[2],-.5);
    Modell m;
    m.id="referenz";m.masse_kg=4000.;m.schwerpunkt_m={0,0,-.75};
    m.traegheit_kg_m2={2000,0,0,0,2000,0,0,0,2000};
    Element e;e.id="Rumpf";e.masse_kg=m.masse_kg;e.schwerpunkt_m=m.schwerpunkt_m;e.volumen=mesh;
    m.elemente.push_back(e);
    const auto k=hydrostatik(m,Pose{{},{0,0,0,1}});
    near(k.kraft_welt_n[2],0.,1e-8);near(k.moment_modell_nm[0],0.);
    Zustand start;start.pose=Pose{{0,0,3},{0,0,0,1}};
    auto r=gleichgewicht(m,start);
    if(r.status!="konvergiert") throw std::runtime_error("Statik konvergiert nicht: "+r.diagnose);
    near(r.zustand.pose.position_m()[2],0.,1e-6);
    auto beladen=m;beladen.schwerpunkt_m={.05,.03,-.75};
    beladen.elemente[0].schwerpunkt_m=beladen.schwerpunkt_m;
    r=gleichgewicht(beladen,Zustand{});
    if(r.status!="konvergiert") throw std::runtime_error("Asymmetrische Statik: "+r.diagnose);
    const auto kr=hydrostatik(beladen,r.zustand.pose);
    near(kr.kraft_welt_n[2],0.,.001);near(kr.moment_modell_nm[0],0.,.002);near(kr.moment_modell_nm[1],0.,.002);
    Zustand ruhe;
    for(int i=0;i<100;++i) ruhe=dynamikschritt(m,ruhe,.01);
    near(ruhe.pose.position_m()[2],0.,1e-12);
    for(double v:ruhe.geschwindigkeit_welt_m_s) near(v,0.,1e-12);
    for(double w:ruhe.omega_modell_rad_s) near(w,0.,1e-12);
    m.masse_kg=9000.;m.elemente[0].masse_kg=9000.;
    if(gleichgewicht(m,Zustand{}).status!="unloesbar") throw std::runtime_error("Ueberlast nicht erkannt");
    std::atomic_bool stop{true};
    if(gleichgewicht(m,Zustand{},.001,.001,10,&stop).status!="abgebrochen") throw std::runtime_error("Abbruch fehlt");
    m.masse_kg=4000.;m.elemente[0].masse_kg=4000.;m.elemente[0].volumen.reset();
    Zustand fall;
    for(int i=0;i<100;++i) fall=dynamikschritt(m,fall,.01);
    near(fall.pose.modellpunkt_nach_welt(m.schwerpunkt_m)[2],-.75-4.905,1e-10);
    near(fall.geschwindigkeit_welt_m_s[2],-9.81,1e-10);
    near(hydrostatik(m,fall.pose).volumen_m3,0.);
    Zustand spin;spin.omega_modell_rad_s={0,0,2};
    for(int i=0;i<100;++i) spin=dynamikschritt(m,spin,.01);
    near(spin.omega_modell_rad_s[2],2.);
    near(spin.pose.orientierung_xyzw()[2],std::sin(1.),1e-12);
    near(spin.pose.orientierung_xyzw()[3],std::cos(1.),1e-12);
    auto singular=m;singular.traegheit_kg_m2={};
    bool inertia_rejected=false;
    try {dynamikschritt(singular,Zustand{},.01);} catch(const std::invalid_argument&) {inertia_rejected=true;}
    if(!inertia_rejected) throw std::runtime_error("Singulaere Traegheit nicht abgelehnt");
    m.elemente[0].volumen=mesh;
    auto bewegung=[&](double dt) {
        Zustand z;z.pose=Pose{{0,0,.1},{0,0,0,1}};
        const auto n=static_cast<int>(std::round(1./dt));
        for(int i=0;i<n;++i) z=dynamikschritt(m,z,dt);
        return z.pose.position_m()[2];
    };
    const double reference=bewegung(.000625),coarse=bewegung(.02),fine=bewegung(.01);
    near(bewegung(.01),fine,0.);
    if(std::abs(fine-reference)>=std::abs(coarse-reference)*.4)
        throw std::runtime_error("Schrittweitenkonvergenz nicht nachgewiesen");
    if(std::abs(fine-reference)>.001)
        throw std::runtime_error("Numerischer Hoehenfehler groesser als 1 mm");
    m.elemente[0].volumen.reset();
    testmessungen_zuruecksetzen();
    hydrostatik(m,fall.pose);
    if(!testmessungen().empty()) throw std::runtime_error("Detailtimer ohne Aktivierung");
#ifdef SEGELPHYSIK_TEST_INSTRUMENTATION
    testmessungen_aktivieren(true);
    hydrostatik(m,fall.pose);
    testmessungen_aktivieren(false);
    if(testmessungen().at("hydrostatikauswertung").anzahl!=1) throw std::runtime_error("Messzaehler falsch");
#else
    bool rejected=false;
    try {testmessungen_aktivieren(true);} catch(const std::invalid_argument&) {rejected=true;}
    if(!rejected) throw std::runtime_error("Standardbuild aktiviert Detailmessung");
#endif
}
}
int main()
{
    try {pruefen();} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
    return 0;
}
