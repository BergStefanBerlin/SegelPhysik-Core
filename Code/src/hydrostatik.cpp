#include "segelphysik/hydrostatik.hpp"
#include "segelphysik/messung.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <stdexcept>

namespace segelphysik {
namespace {
Vektor3 plus(Vektor3 a, const Vektor3& b)
{
    for (std::size_t i = 0; i < 3; ++i) a[i] += b[i];
    return a;
}
Vektor3 mal(Vektor3 a, double s)
{
    for (double& v : a) v *= s;
    return a;
}
double dot(const Vektor3& a, const Vektor3& b)
{
    return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
}
Vektor3 cross(const Vektor3& a, const Vektor3& b)
{
    return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};
}
void positiv(double v)
{
    if (!std::isfinite(v) || v <= 0.) throw std::invalid_argument("CORE-SIM-0001: Parameter muss endlich positiv sein.");
}
Vektor3 loesen(std::array<double,9> a, Vektor3 b)
{
    double skala = 0.;
    for (double x : a) skala = std::max(skala, std::abs(x));
    for (std::size_t k=0;k<3;++k) {
        std::size_t pivot=k;
        for (std::size_t i=k+1;i<3;++i)
            if (std::abs(a[3*i+k])>std::abs(a[3*pivot+k])) pivot=i;
        if (std::abs(a[3*pivot+k]) <= 64.*std::numeric_limits<double>::epsilon()*skala)
            throw std::invalid_argument("CORE-SIM-0002: Matrix ist singulaer.");
        for (std::size_t j=0;j<3;++j) std::swap(a[3*k+j],a[3*pivot+j]);
        std::swap(b[k],b[pivot]);
        for (std::size_t i=k+1;i<3;++i) {
            const double f=a[3*i+k]/a[3*k+k];
            for (std::size_t j=k;j<3;++j) a[3*i+j]-=f*a[3*k+j];
            b[i]-=f*b[k];
        }
    }
    Vektor3 x{};
    for (int i=2;i>=0;--i) {
        double v=b[static_cast<std::size_t>(i)];
        for (int j=i+1;j<3;++j) v-=a[static_cast<std::size_t>(3*i+j)]*x[static_cast<std::size_t>(j)];
        x[static_cast<std::size_t>(i)]=v/a[static_cast<std::size_t>(4*i)];
    }
    return x;
}
Pose euler(double z, double roll, double pitch, double yaw, double x, double y)
{
    const double cr=std::cos(roll/2),sr=std::sin(roll/2);
    const double cp=std::cos(pitch/2),sp=std::sin(pitch/2);
    const double cy=std::cos(yaw/2),sy=std::sin(yaw/2);
    return {{x,y,z},{sr*cp*cy-cr*sp*sy,cr*sp*cy+sr*cp*sy,
        cr*cp*sy-sr*sp*cy,cr*cp*cy+sr*sp*sy}};
}
QuaternionXyzw produkt(const QuaternionXyzw& a,const QuaternionXyzw& b)
{
    return {a[3]*b[0]+a[0]*b[3]+a[1]*b[2]-a[2]*b[1],
        a[3]*b[1]-a[0]*b[2]+a[1]*b[3]+a[2]*b[0],
        a[3]*b[2]+a[0]*b[1]-a[1]*b[0]+a[2]*b[3],
        a[3]*b[3]-a[0]*b[0]-a[1]*b[1]-a[2]*b[2]};
}
}

VolumenWerte wasseranteil(const Mesh& mesh, const Pose& pose)
{
    SEGEL_MESSUNG("ebenenschnitt");
    if (mesh.punkte.empty() || mesh.dreiecke.empty())
        throw std::invalid_argument("CORE-MOD-0006: Leeres Volumenmesh.");
    const auto n=pose.weltvektor_nach_modell({0.,0.,1.});
    const double offset=-pose.position_m()[2];
    double minimum=std::numeric_limits<double>::infinity(),maximum=-minimum;
    for(const auto& p:mesh.punkte) {
        const double d=dot(n,p)-offset;
        minimum=std::min(minimum,d);maximum=std::max(maximum,d);
    }
    if(minimum>=0.) return {};
    const auto apex=plus(mesh.punkte.front(),mal(n,offset-dot(n,mesh.punkte.front())));
    double volumen=0.,korrektur=0.;
    Vektor3 moment{};
    auto add=[&](const Vektor3& pa,const Vektor3& pb,const Vektor3& pc) {
        const auto a=plus(pa,mal(apex,-1.)),b=plus(pb,mal(apex,-1.)),c=plus(pc,mal(apex,-1.));
        const double v=dot(a,cross(b,c))/6.;
        const double y=v-korrektur,t=volumen+y;
        korrektur=(t-volumen)-y;volumen=t;
        moment=plus(moment,mal(plus(plus(a,b),c),v/4.));
    };
    for (const auto& tri : mesh.dreiecke) {
        if(maximum<=0.) {
            add(mesh.punkte.at(tri[0]),mesh.punkte.at(tri[1]),mesh.punkte.at(tri[2]));
            continue;
        }
        std::array<Vektor3,4> clipped{};
        std::size_t count=0;
        for (std::size_t i=0;i<3;++i) {
            const auto& a=mesh.punkte.at(tri[i]);
            const auto& b=mesh.punkte.at(tri[(i+1)%3]);
            const double da=dot(n,a)-offset,db=dot(n,b)-offset;
            if (da<=0.) clipped[count++]=a;
            if ((da<0. && db>0.) || (da>0. && db<0.))
                clipped[count++]=plus(a,mal(plus(b,mal(a,-1.)),da/(da-db)));
        }
        for (std::size_t i=1;i+1<count;++i) add(clipped[0],clipped[i],clipped[i+1]);
    }
    if (!std::isfinite(volumen) || volumen<0.)
        throw std::runtime_error("CORE-MOD-0006: Negatives oder ungueltiges Schnittvolumen.");
    if (volumen==0.) return {};
    return {volumen,plus(apex,mal(moment,1./volumen))};
}

Vektor3 moment_um(const Vektor3& a,const Vektor3& p,const Vektor3& f,const Vektor3& eigen)
{
    return plus(cross(plus(a,mal(p,-1.)),f),eigen);
}

Kraefte hydrostatik(const Modell& modell,const Pose& pose,double rho,double g,
    const std::vector<std::string>& geflutet)
{
    SEGEL_MESSUNG("hydrostatikauswertung");
    positiv(rho);positiv(g);positiv(modell.masse_kg);
    Kraefte result;
    result.beitraege.reserve(2*modell.elemente.size()+1);
    const auto gewicht_richtung=pose.weltvektor_nach_modell({0.,0.,-g});
    for (const auto& e : modell.elemente) {
        const auto gewicht=mal(gewicht_richtung,e.masse_kg);
        const auto moment_g=moment_um(e.schwerpunkt_m,modell.schwerpunkt_m,gewicht);
        auto& bg=result.beitraege.emplace_back();
        bg.id=e.id+":gewicht";bg.element_id=e.id;bg.art="kraft";bg.punkt_m=e.schwerpunkt_m;
        bg.vektor=gewicht;bg.quelle=Quelle::schwerkraft;bg.moment_s_nm=moment_g;
        result.kraft_welt_n[2]-=e.masse_kg*g;
        result.moment_modell_nm=plus(result.moment_modell_nm,moment_g);
        if (e.volumen) {
            const bool voll=!geflutet.empty() &&
                std::binary_search(geflutet.begin(),geflutet.end(),e.id);
            VolumenWerte wet;
            if(voll) wet.geflutet=true;else wet=wasseranteil(*e.volumen,pose);
            result.elementvolumina.emplace_back(e.id,wet);
            result.volumen_m3+=wet.volumen_m3;
            auto& ba=result.beitraege.emplace_back();
            ba.id=e.id+":auftrieb";ba.element_id=e.id;ba.art="kraft";ba.quelle=Quelle::hydrostatik;
            if (wet.schwerpunkt_m) {
                const auto f=pose.weltvektor_nach_modell({0.,0.,rho*g*wet.volumen_m3});
                const auto moment_a=moment_um(*wet.schwerpunkt_m,modell.schwerpunkt_m,f);
                ba.punkt_m=*wet.schwerpunkt_m;ba.vektor=f;ba.moment_s_nm=moment_a;
                result.kraft_welt_n[2]+=rho*g*wet.volumen_m3;
                result.moment_modell_nm=plus(result.moment_modell_nm,moment_a);
            } else {
                // Dry or flooded body: valid zero buoyancy, shown at the element reference point.
                ba.punkt_m=e.schwerpunkt_m;ba.punktart=Punktart::bezugspunkt;ba.zustand=Datenzustand::null;
            }
        } else result.elementvolumina.emplace_back(e.id,VolumenWerte{});
    }
    auto& gm=result.beitraege.emplace_back();
    gm.id="gesamt:moment";gm.art="moment";gm.punkt_m=modell.schwerpunkt_m;gm.vektor=result.moment_modell_nm;
    gm.quelle=Quelle::summe;gm.gruppe="gesamt";gm.rolle=Rolle::gesamtsumme;gm.punktart=Punktart::bezugspunkt;
    gm.moment_s_nm=result.moment_modell_nm;
    return result;
}

std::vector<std::string> flutung(const Modell& modell,const Pose& pose,std::vector<std::string> bisher)
{
    std::sort(bisher.begin(),bisher.end());
    bisher.erase(std::unique(bisher.begin(),bisher.end()),bisher.end());
    for (const auto& e : modell.elemente) {
        if (e.flutrand_m.empty() || !e.volumen ||
            std::binary_search(bisher.begin(),bisher.end(),e.id)) continue;
        for (const auto& p : e.flutrand_m)
            if (pose.modellpunkt_nach_welt(p)[2]<0.) {
                bisher.insert(std::lower_bound(bisher.begin(),bisher.end(),e.id),e.id);
                break;
            }
    }
    return bisher;
}

const char* name(Quelle wert)
{
    switch (wert) {
    case Quelle::testantrieb: return "testantrieb";
    case Quelle::schwerkraft: return "schwerkraft";
    case Quelle::hydrostatik: return "hydrostatik";
    case Quelle::summe: return "summe";
    default: return "nicht_zugeordnet";
    }
}
const char* name(Rolle wert)
{
    switch (wert) {
    case Rolle::gruppensumme: return "gruppensumme";
    case Rolle::gesamtsumme: return "gesamtsumme";
    default: return "einzelbeitrag";
    }
}
const char* name(Punktart wert)
{
    return wert==Punktart::bezugspunkt?"bezugspunkt":"angriffspunkt";
}
const char* name(Datenzustand wert)
{
    switch (wert) {
    case Datenzustand::null: return "null";
    case Datenzustand::deaktiviert: return "deaktiviert";
    case Datenzustand::nicht_berechnet: return "nicht_berechnet";
    case Datenzustand::ungueltig: return "ungueltig";
    default: return "gueltig";
    }
}

Bilanz bilanz(const Modell& modell,std::vector<Beitrag>& beitraege)
{
    Bilanz result;
    result.gesamt.gruppe="gesamt";
    if(std::any_of(beitraege.begin(),beitraege.end(),
        [](const Beitrag& b){return b.quelle==Quelle::testantrieb;}))
        result.erwartete_beitraege.push_back("testantrieb.bug.kraft");
    for (const auto& e : modell.elemente) {
        result.erwartete_beitraege.push_back(e.id+":gewicht");
        if (e.volumen) result.erwartete_beitraege.push_back(e.id+":auftrieb");
    }
    auto endlich=[](const Vektor3& v) {return std::isfinite(v[0])&&std::isfinite(v[1])&&std::isfinite(v[2]);};
    for (auto& b : beitraege) {
        const bool hat_werte=b.zustand==Datenzustand::gueltig||b.zustand==Datenzustand::null;
        if (hat_werte && (!endlich(b.vektor)||!endlich(b.punkt_m)||!endlich(b.moment_s_nm))) {
            b.zustand=Datenzustand::ungueltig;b.fehler_id="CORE-BIL-0001";
        }
        if (b.rolle!=Rolle::einzelbeitrag ||
            (b.zustand!=Datenzustand::gueltig && b.zustand!=Datenzustand::null)) continue;
        auto gruppe=std::find_if(result.gruppen.begin(),result.gruppen.end(),
            [&](const Summe& s) {return s.gruppe==b.gruppe;});
        if (gruppe==result.gruppen.end()) {
            result.gruppen.push_back({std::string(b.gruppe),{},{},{}});
            gruppe=std::prev(result.gruppen.end());
        }
        const Vektor3 kraft=b.art=="kraft"?b.vektor:Vektor3{};
        for (auto* s : {&*gruppe,&result.gesamt}) {
            s->kraft_modell_n=plus(s->kraft_modell_n,kraft);
            s->moment_s_nm=plus(s->moment_s_nm,b.moment_s_nm);
            s->quellen.push_back(b.id);
        }
    }
    bool luecke=false;
    for (const auto& id : result.erwartete_beitraege) {
        const auto fund=std::find_if(beitraege.begin(),beitraege.end(),[&](const Beitrag& b) {return b.id==id;});
        if (fund==beitraege.end() || fund->zustand==Datenzustand::nicht_berechnet ||
            fund->zustand==Datenzustand::ungueltig) {
            result.fehlende_beitraege.push_back(id);luecke=true;
        }
    }
    result.status=modell.elemente.empty()?"ungeklaert":(luecke?"teilsumme":"vollstaendig");
    return result;
}

namespace {
SolverErgebnis gleichgewicht_suche(const Modell& modell,const Zustand& start,
    double ftol,double mtol,std::size_t budget,const std::atomic_bool* abbruch,
    const std::vector<std::string>& geflutet)
{
    SEGEL_MESSUNG("gleichgewichtssuche");
    positiv(ftol);positiv(mtol);
    if (!budget) throw std::invalid_argument("CORE-SIM-0001: Iterationsbudget muss positiv sein.");
    if (abbruch && abbruch->load())
        return {start,"abgebrochen","Abbruch angefordert",0};
    const auto q=start.pose.orientierung_xyzw();
    Vektor3 lage{start.pose.position_m()[2],
        std::atan2(2*(q[3]*q[0]+q[1]*q[2]),1-2*(q[0]*q[0]+q[1]*q[1])),
        std::asin(std::clamp(2*(q[3]*q[1]-q[2]*q[0]),-1.,1.))};
    const double yaw=std::atan2(2*(q[3]*q[2]+q[0]*q[1]),1-2*(q[1]*q[1]+q[2]*q[2]));
    SolverErgebnis result{start,"nicht_konvergiert","CORE-SIM-0004: Iterationsbudget erschoepft",0};
    result.zustand.geflutet=geflutet;
    auto pose=[&](const Vektor3& p) {return euler(p[0],p[1],p[2],yaw,start.pose.position_m()[0],start.pose.position_m()[1]);};
    auto residuum=[&](const Vektor3& p) {
        const auto k=hydrostatik(modell,pose(p),1000.,9.81,geflutet);
        const auto mw=pose(p).modellvektor_nach_welt(k.moment_modell_nm);
        return Vektor3{k.kraft_welt_n[2]/ftol,mw[0]/mtol,mw[1]/mtol};
    };
    double gesamtvolumen=0.;
    for(const auto& e:modell.elemente) {
        if(e.volumen && !std::binary_search(geflutet.begin(),geflutet.end(),e.id)) {
            double maxz=-std::numeric_limits<double>::infinity();
            for(const auto& p:e.volumen->punkte) maxz=std::max(maxz,p[2]);
            gesamtvolumen+=wasseranteil(*e.volumen,Pose{{0,0,-maxz-1.},{0,0,0,1}}).volumen_m3;
        }
    }
    if(gesamtvolumen*1000.*9.81<modell.masse_kg*9.81-ftol) {
        result.status="unloesbar";result.diagnose="CORE-SIM-0003: Maximaler Auftrieb kleiner als Gewicht";return result;
    }
    // Establish a wet starting height even for an initially dry body.
    double low=lage[0]-1.,high=lage[0]+1.;
    for (int i=0;i<30 && residuum({low,lage[1],lage[2]})[0]<0.;++i) low-=std::pow(2.,i);
    if (residuum({low,lage[1],lage[2]})[0]<0.) {
        result.status="unloesbar";result.diagnose="CORE-SIM-0003: Maximaler Auftrieb kleiner als Gewicht";return result;
    }
    for (int i=0;i<30 && residuum({high,lage[1],lage[2]})[0]>0.;++i) high+=std::pow(2.,i);
    for (int i=0;i<50;++i) {
        const double mid=(low+high)/2.;
        if (residuum({mid,lage[1],lage[2]})[0]>0.) low=mid;else high=mid;
    }
    lage[0]=(low+high)/2.;
    for (std::size_t iteration=0;iteration<budget;++iteration) {
        result.iterationen=iteration+1;result.zustand.pose=pose(lage);
        if (abbruch && abbruch->load()) {result.status="abgebrochen";result.diagnose="Abbruch angefordert";return result;}
        const auto r=residuum(lage);
        const double norm=std::max({std::abs(r[0]),std::abs(r[1]),std::abs(r[2])});
        if (norm<=1.) {result.status="konvergiert";result.diagnose.clear();return result;}
        std::array<double,9> jac{};
        for (std::size_t j=0;j<3;++j) {
            auto p=lage,m=lage;const double h=1e-5;
            p[j]+=h;m[j]-=h;const auto rp=residuum(p),rm=residuum(m);
            for (std::size_t i=0;i<3;++i) jac[3*i+j]=(rp[i]-rm[i])/(2*h);
        }
        Vektor3 delta;
        try {delta=loesen(jac,mal(r,-1.));}
        catch (const std::invalid_argument& e) {result.diagnose=e.what();return result;}
        double scale=1.;
        for (std::size_t j=0;j<3;++j) scale=std::min(scale,(j==0?.5:.2)/std::max(std::abs(delta[j]),1e-300));
        bool verbessert=false;
        for (int ls=0;ls<20;++ls) {
            const auto candidate=plus(lage,mal(delta,scale));
            const auto v=residuum(candidate);
            if (std::max({std::abs(v[0]),std::abs(v[1]),std::abs(v[2])})<norm) {
                lage=candidate;verbessert=true;break;
            }
            scale*=.5;
        }
        if (!verbessert) {result.diagnose="CORE-SIM-0004: Liniensuche stagniert";return result;}
    }
    return result;
}
}

SolverErgebnis gleichgewicht(const Modell& modell,const Zustand& start,
    double ftol,double mtol,std::size_t budget,const std::atomic_bool* abbruch)
{
    if((start.testantrieb_erforderlich&&!start.testantrieb) ||
        (start.testantrieb&&start.testantrieb->aktiv))
        throw std::invalid_argument("CORE-SIM-0001: Aktiver Testantrieb in Statik oder fehlender Kontext.");
    // The start pose is only an initial guess and does not flood anything.
    auto geflutet=start.geflutet;
    std::sort(geflutet.begin(),geflutet.end());
    geflutet.erase(std::unique(geflutet.begin(),geflutet.end()),geflutet.end());
    SolverErgebnis result;
    std::size_t iterationen=0;
    // Each repetition floods at least one more element, so this terminates.
    for(;;) {
        result=gleichgewicht_suche(modell,start,ftol,mtol,budget,abbruch,geflutet);
        iterationen+=result.iterationen;
        if(result.status!="konvergiert") break;
        auto neu=flutung(modell,result.zustand.pose,geflutet);
        if(neu==geflutet) break;
        geflutet=std::move(neu);
    }
    result.iterationen=iterationen;
    result.zustand.geflutet=geflutet;
    // An equilibrium is a state at rest, whatever motion the start carried.
    result.zustand.geschwindigkeit_welt_m_s={};
    result.zustand.omega_modell_rad_s={};
    if(!geflutet.empty() && result.status=="unloesbar") {
        result.diagnose+="; geflutet:";
        for(const auto& id:geflutet) result.diagnose+=" "+id;
    }
    // Residuals of the reported pose, measured exactly like the convergence test.
    const auto k=hydrostatik(modell,result.zustand.pose,1000.,9.81,geflutet);
    const auto mw=result.zustand.pose.modellvektor_nach_welt(k.moment_modell_nm);
    result.gleichgewicht=Gleichgewichtsangaben{ftol,mtol,std::abs(k.kraft_welt_n[2]),
        std::max(std::abs(mw[0]),std::abs(mw[1])),{"position_z","rollen","stampfen"},result.iterationen};
    return result;
}

namespace {
void testparameter(const TestantriebKonfiguration& c)
{
    if(!std::isfinite(c.kraft_n)||c.kraft_n<0.||c.kraft_n>100000.||
        !std::isfinite(c.grenze_m_s)||c.grenze_m_s<.01||c.grenze_m_s>20.)
        throw std::invalid_argument("CORE-SIM-0001: Ungueltige Testantriebsparameter.");
}
std::optional<Vektor3> bugpunkt(const Modell& m)
{
    std::vector<Vektor3> punkte;
    for(const auto& e:m.elemente) {
        punkte.insert(punkte.end(),e.massenpunkte_m.begin(),e.massenpunkte_m.end());
        if(e.volumen) punkte.insert(punkte.end(),e.volumen->punkte.begin(),e.volumen->punkte.end());
    }
    if(punkte.empty()) return std::nullopt;
    auto lo=punkte.front(),hi=lo;
    for(const auto& p:punkte) for(std::size_t i=0;i<3;++i) {
        if(!std::isfinite(p[i])) throw std::invalid_argument("CORE-SIM-0001: Buggeometrie nicht endlich.");
        lo[i]=std::min(lo[i],p[i]);hi[i]=std::max(hi[i],p[i]);
    }
    const double eps=std::max(1e-9,1e-9*std::max({hi[0]-lo[0],hi[1]-lo[1],hi[2]-lo[2]}));
    std::sort(punkte.begin(),punkte.end());
    std::vector<Vektor3> vorne;
    for(const auto& p:punkte) {
        if(hi[0]-p[0]>eps) continue;
        if(std::none_of(vorne.begin(),vorne.end(),[&](const Vektor3& q) {
            return std::hypot(p[0]-q[0],p[1]-q[1],p[2]-q[2])<=eps;
        })) vorne.push_back(p);
    }
    Vektor3 mittel{};
    for(const auto& p:vorne) mittel=plus(mittel,mal(p,1./static_cast<double>(vorne.size())));
    return mittel;
}
void kontextpruefung(const Modell& m,const Zustand& s)
{
    if(s.testantrieb_erforderlich&&!s.testantrieb)
        throw std::invalid_argument("CORE-SIM-0001: Erforderlicher Testkontext fehlt.");
    if(s.testantrieb && (s.testantrieb->modell_fassung!=m.fassung ||
        s.testantrieb->auswertung_schritt!=s.schritt || s.testantrieb->auswertung_zeit_s!=s.zeit_s))
        throw std::invalid_argument("CORE-SIM-0001: Testkontext passt nicht zum Zustand oder Modell.");
}
}

Zustand testantrieb_start(const Modell& m,const Zustand& s,
    const TestantriebKonfiguration& c,bool aktiv)
{
    testparameter(c);
    if(s.testantrieb || s.testantrieb_erforderlich || !std::isfinite(s.zeit_s))
        throw std::invalid_argument("CORE-SIM-0001: Teststart benoetigt kontextfreien endlichen Zustand.");
    auto k=std::make_shared<TestantriebKontext>();
    k->konfiguration=c;k->aktiv=aktiv;k->modell_fassung=m.fassung;
    k->zugpunkt_modell_m=bugpunkt(m);
    k->auswertung_schritt=s.schritt;k->auswertung_zeit_s=s.zeit_s;
    // Validate active geometry/direction before publishing the initial state.
    kraefte(m,s.pose,k.get(),1000.,9.81,s.geflutet);
    auto result=s;result.testantrieb=k;result.testantrieb_erforderlich=true;
    return result;
}

Kraefte kraefte(const Modell& m,const Pose& p,const TestantriebKontext* c,
    double rho,double g,const std::vector<std::string>& geflutet)
{
    auto k=hydrostatik(m,p,rho,g,geflutet);
    if(!c) return k;
    testparameter(c->konfiguration);
    if(c->modell_fassung!=m.fassung)
        throw std::invalid_argument("CORE-SIM-0001: Falsche Testgeometriefassung.");
    Beitrag b;b.id="testantrieb.bug.kraft";b.art="kraft";
    b.quelle=Quelle::testantrieb;b.gruppe="testantrieb";
    b.zustand=c->aktiv?Datenzustand::null:Datenzustand::deaktiviert;
    b.punkt_bekannt=c->zugpunkt_modell_m.has_value();
    if(c->zugpunkt_modell_m) b.punkt_m=*c->zugpunkt_modell_m;
    if(c->aktiv && c->konfiguration.kraft_n>0.) {
        if(!c->zugpunkt_modell_m)
            throw std::invalid_argument("CORE-SIM-0001: Physische Buggeometrie fehlt.");
        const auto bow=p.modellvektor_nach_welt({1.,0.,0.});
        const double norm=std::hypot(bow[0],bow[1]);
        if(norm<=1e-10)
            throw std::invalid_argument("CORE-SIM-0001: Horizontale Bugprojektion singulaer.");
        const Vektor3 f{c->konfiguration.kraft_n*bow[0]/norm,c->konfiguration.kraft_n*bow[1]/norm,0.};
        b.vektor=p.weltvektor_nach_modell(f);
        b.moment_s_nm=moment_um(b.punkt_m,m.schwerpunkt_m,b.vektor);
        b.zustand=Datenzustand::gueltig;
        k.kraft_welt_n=plus(k.kraft_welt_n,f);
        k.moment_modell_nm=plus(k.moment_modell_nm,b.moment_s_nm);
    }
    for(auto& sum:k.beitraege) if(sum.rolle==Rolle::gesamtsumme) {
        sum.vektor=k.moment_modell_nm;sum.moment_s_nm=k.moment_modell_nm;
    }
    k.beitraege.push_back(b);
    return k;
}

Zustand dynamikschritt(const Modell& modell,const Zustand& s,double dt,double rho,double g)
{
    return dynamikschritt(modell,s,dt,std::vector<TestantriebEreignis>{},rho,g);
}
Zustand dynamikschritt(const Modell& modell,const Zustand& s,double dt,
    const std::vector<TestantriebEreignis>& ereignisse,double rho,double g)
{
    SEGEL_MESSUNG("dynamischer_zeitschritt");
    positiv(dt);
    kontextpruefung(modell,s);
    if(!s.testantrieb&&!ereignisse.empty())
        throw std::invalid_argument("CORE-SIM-0001: Ereignisse ohne Testkontext.");
    std::shared_ptr<TestantriebKontext> context;
    if(s.testantrieb) {
        context=std::make_shared<TestantriebKontext>(*s.testantrieb);
        for(const auto& e:ereignisse) {
            if(e.kennung<=context->letzte_ereigniskennung)
                throw std::invalid_argument("CORE-SIM-0001: Ereigniskennungen nicht streng aufsteigend.");
            context->letzte_ereigniskennung=e.kennung;context->aktiv=e.aktiv;
        }
        context->uebergang=TestantriebUebergang{s.schritt,s.schritt+1,s.zeit_s,dt,ereignisse,{}};
    }
    for(double value:s.geschwindigkeit_welt_m_s)
        if(!std::isfinite(value)) throw std::invalid_argument("CORE-SIM-0001: Geschwindigkeit nicht endlich.");
    for(double value:s.omega_modell_rad_s)
        if(!std::isfinite(value)) throw std::invalid_argument("CORE-SIM-0001: Winkelgeschwindigkeit nicht endlich.");
    if(!std::isfinite(s.zeit_s)) throw std::invalid_argument("CORE-SIM-0001: Simulationszeit nicht endlich.");
    const auto& I=modell.traegheit_kg_m2;
    // Positive definiteness is required, not a regularized inverse.
    if (I[0]<=0. || I[0]*I[4]-I[1]*I[3]<=0. ||
        I[0]*(I[4]*I[8]-I[5]*I[7])-I[1]*(I[3]*I[8]-I[5]*I[6])+I[2]*(I[3]*I[7]-I[4]*I[6])<=0.)
        throw std::invalid_argument("CORE-SIM-0002: Dynamiktraegheit nicht positiv definit.");
    const auto geflutet=flutung(modell,s.pose,s.geflutet);
    const auto k=kraefte(modell,s.pose,context.get(),rho,g,geflutet);
    const auto a=mal(k.kraft_welt_n,1./modell.masse_kg);
    Vektor3 impuls{};
    for (std::size_t i=0;i<3;++i)
        for (std::size_t j=0;j<3;++j) impuls[i]+=I[3*i+j]*s.omega_modell_rad_s[j];
    const auto alpha=loesen(I,plus(k.moment_modell_nm,mal(cross(s.omega_modell_rad_s,impuls),-1.)));
    const auto omega_mid=plus(s.omega_modell_rad_s,mal(alpha,dt/2));
    const double angle=std::sqrt(dot(omega_mid,omega_mid))*dt;
    QuaternionXyzw dq{0.,0.,0.,1.};
    if (angle!=0.) {
        const double factor=std::sin(angle/2)*dt/angle;
        dq={omega_mid[0]*factor,omega_mid[1]*factor,omega_mid[2]*factor,std::cos(angle/2)};
    }
    const auto qnew=produkt(s.pose.orientierung_xyzw(),dq);
    const auto cg=s.pose.modellpunkt_nach_welt(modell.schwerpunkt_m);
    const auto midq=produkt(s.pose.orientierung_xyzw(),
        QuaternionXyzw{omega_mid[0]*dt/4,omega_mid[1]*dt/4,omega_mid[2]*dt/4,1.});
    const Pose midrotation{{},midq};
    const auto midcg=plus(plus(cg,mal(s.geschwindigkeit_welt_m_s,dt/2)),mal(a,dt*dt/8));
    const Pose midpose{plus(midcg,mal(midrotation.modellvektor_nach_welt(modell.schwerpunkt_m),-1.)),midq};
    const auto midforce=kraefte(modell,midpose,context.get(),rho,g,geflutet);
    Vektor3 midimpuls{};
    for(std::size_t i=0;i<3;++i)
        for(std::size_t j=0;j<3;++j) midimpuls[i]+=I[3*i+j]*omega_mid[j];
    const auto midalpha=loesen(I,plus(midforce.moment_modell_nm,
        mal(cross(omega_mid,midimpuls),-1.)));
    const auto cgnew=plus(plus(cg,mal(s.geschwindigkeit_welt_m_s,dt)),mal(a,dt*dt/2));
    const Pose rotation{{},qnew};
    Zustand result;
    result.pose=Pose{plus(cgnew,mal(rotation.modellvektor_nach_welt(modell.schwerpunkt_m),-1.)),qnew};
    result.geschwindigkeit_welt_m_s=plus(s.geschwindigkeit_welt_m_s,
        mal(midforce.kraft_welt_n,dt/modell.masse_kg));
    result.omega_modell_rad_s=plus(s.omega_modell_rad_s,mal(midalpha,dt));
    result.zeit_s=s.zeit_s+dt;result.schritt=s.schritt+1;
    result.geflutet=flutung(modell,result.pose,geflutet);
    if(context) {
        auto& b=context->uebergang->begrenzung;
        b.vorher_welt_m_s=result.geschwindigkeit_welt_m_s;
        const double speed=std::hypot(b.vorher_welt_m_s[0],b.vorher_welt_m_s[1]);
        if(context->aktiv && speed>context->konfiguration.grenze_m_s) {
            b.wirksam=true;
            const double scale=context->konfiguration.grenze_m_s/speed;
            result.geschwindigkeit_welt_m_s[0]*=scale;
            result.geschwindigkeit_welt_m_s[1]*=scale;
        }
        b.nachher_welt_m_s=result.geschwindigkeit_welt_m_s;
        if(b.wirksam) {
            b.korrekturimpuls_ns=mal(plus(b.nachher_welt_m_s,mal(b.vorher_welt_m_s,-1.)),modell.masse_kg);
            // Only XY changes; avoid cancellation of a large unchanged Z energy.
            b.energieaenderung_j=.5*modell.masse_kg*
                ((b.nachher_welt_m_s[0]*b.nachher_welt_m_s[0]+b.nachher_welt_m_s[1]*b.nachher_welt_m_s[1])-
                 (b.vorher_welt_m_s[0]*b.vorher_welt_m_s[0]+b.vorher_welt_m_s[1]*b.vorher_welt_m_s[1]));
        }
        if(!std::isfinite(b.energieaenderung_j))
            throw std::overflow_error("CORE-SIM-0001: Begrenzerenergie nicht endlich.");
        for(double value:b.korrekturimpuls_ns) if(!std::isfinite(value))
            throw std::overflow_error("CORE-SIM-0001: Korrekturimpuls nicht endlich.");
        context->auswertung_schritt=result.schritt;context->auswertung_zeit_s=result.zeit_s;
        result.testantrieb=context;result.testantrieb_erforderlich=true;
        if(context->aktiv&&context->konfiguration.kraft_n>0.)
            kraefte(modell,result.pose,context.get(),rho,g,result.geflutet);
    }
    for(double value:result.geschwindigkeit_welt_m_s)
        if(!std::isfinite(value)) throw std::overflow_error("CORE-SIM-0001: Geschwindigkeitsueberlauf.");
    for(double value:result.omega_modell_rad_s)
        if(!std::isfinite(value)) throw std::overflow_error("CORE-SIM-0001: Rotationsueberlauf.");
    if(!std::isfinite(result.zeit_s) || result.zeit_s<=s.zeit_s || result.schritt<=s.schritt)
        throw std::overflow_error("CORE-SIM-0001: Zeit- oder Schrittueberlauf.");
    return result;
}
}
