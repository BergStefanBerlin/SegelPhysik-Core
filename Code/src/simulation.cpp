#include "segelphysik/simulation.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>

namespace segelphysik {
namespace {
std::string text(const std::string& s)
{
    std::ostringstream out;
    out << '"';
    for (unsigned char c : s) {
        switch (c) {
        case '"':out<<"\\\"";break;
        case '\\':out<<"\\\\";break;
        case '\n':out<<"\\n";break;
        case '\r':out<<"\\r";break;
        case '\t':out<<"\\t";break;
        default:
            if(c<32) out<<"\\u"<<std::hex<<std::setw(4)<<std::setfill('0')<<static_cast<int>(c)<<std::dec;
            else out<<static_cast<char>(c);
        }
    }
    out<<'"';return out.str();
}
template<std::size_t N>
void array(std::ostream& out,const std::array<double,N>& values)
{
    out<<'[';
    for(std::size_t i=0;i<N;++i) {
        if(!std::isfinite(values[i])) throw std::runtime_error("CORE-SIM-0001: Nicht endliches Ergebnis.");
        if(i) out<<',';
        out<<values[i];
    }
    out<<']';
}
}
std::string pruefung_json(const std::filesystem::path& path,bool& gueltig)
{
    gueltig=false;
    try {
        const auto m=lade_modell(path);
        gueltig=true;
        return "{\"vertragsversion\":\"1\",\"gueltig\":true,\"modell_id\":"+text(m.id)+
            ",\"modell_fassung\":"+text(m.fassung)+",\"fehler\":[]}";
    } catch(const std::exception& e) {
        const std::string diagnose=e.what();
        const auto end=diagnose.find_first_of(" [:");
        const auto id=diagnose.rfind("CORE-",0)==0?diagnose.substr(0,end):"CORE-MOD-0005";
        return "{\"vertragsversion\":\"1\",\"gueltig\":false,\"fehler\":[{\"id\":"+text(id)+
            ",\"datei\":"+text(path.u8string())+",\"details\":"+text(diagnose)+"}]}";
    }
}
std::string zustand_json(const Modell& m,const Zustand& s,const std::string& status,
    const std::string& diagnose,const Gleichgewichtsangaben* gg)
{
    if(!std::isfinite(s.zeit_s))
        throw std::invalid_argument("CORE-SIM-0001: Simulationszeit nicht endlich.");
    if(s.testantrieb_erforderlich&&!s.testantrieb)
        throw std::invalid_argument("CORE-SIM-0001: Erforderlicher Testkontext fehlt.");
    if(s.testantrieb && (s.testantrieb->modell_fassung!=m.fassung ||
        s.testantrieb->auswertung_schritt!=s.schritt || s.testantrieb->auswertung_zeit_s!=s.zeit_s))
        throw std::invalid_argument("CORE-SIM-0001: Historischer Testkontext passt nicht zum Zustand.");
    if(s.hydrodynamik_erforderlich&&!s.hydrodynamik)
        throw std::invalid_argument("CORE-HYD-0001: Erforderlicher Hydrodynamikkontext fehlt.");
    if(s.hydrodynamik && (s.hydrodynamik->modell_fassung!=m.fassung ||
        s.hydrodynamik->auswertung_schritt!=s.schritt ||
        s.hydrodynamik->auswertung_zeit_s!=s.zeit_s))
        throw std::invalid_argument("CORE-HYD-0001: Historischer Hydrodynamikkontext passt nicht zum Zustand.");
    auto k=kraefte(m,s.pose,s.testantrieb.get(),1000.,9.81,s.geflutet,
        s.hydrodynamik.get(),s.geschwindigkeit_welt_m_s);
    const auto b2=bilanz(m,k.beitraege,static_cast<bool>(s.hydrodynamik));
    std::ostringstream out;out.imbue(std::locale::classic());out<<std::setprecision(17);
    out<<"{\"vertragsversion\":"<<text(s.testantrieb?"3":"2")<<",\"modell_id\":"<<text(m.id)
       <<",\"modell_fassung\":"<<text(m.fassung)
       <<",\"status\":"<<text(status)<<",\"diagnose\":"<<text(diagnose)<<",\"zeit_s\":"<<s.zeit_s
       <<",\"schritt\":"<<s.schritt<<",\"position_m\":";
    array(out,s.pose.position_m());out<<",\"orientierung_xyzw\":";
    array(out,s.pose.orientierung_xyzw());out<<",\"geschwindigkeit_welt_m_s\":";
    array(out,s.geschwindigkeit_welt_m_s);out<<",\"omega_modell_rad_s\":";
    array(out,s.omega_modell_rad_s);
    out<<",\"volumen_m3\":"<<k.volumen_m3<<",\"geflutete_elemente\":[";
    for(std::size_t i=0;i<s.geflutet.size();++i) out<<(i?",":"")<<text(s.geflutet[i]);
    out<<"],\"vektoren\":[";
    bool first=true;
    for(const auto& b:k.beitraege) {
        if(!first) out<<',';
        first=false;
        const bool werte=b.zustand==Datenzustand::gueltig||b.zustand==Datenzustand::null;
        out<<"{\"id\":"<<text(b.id)<<",\"element_id\":"<<text(b.element_id)
           <<",\"art\":"<<text(b.art)<<",\"einheit\":"<<text(b.art=="moment"?"N\u00b7m":"N")
           <<",\"frame\":\"modell\",\"punkt_m\":";
        if(werte&&b.punkt_bekannt) array(out,b.punkt_m);else out<<"null";
        out<<",\"vektor\":";
        if(werte) array(out,b.vektor);else out<<"null";
        out<<",\"quelle\":"<<text(b.quelle==Quelle::nicht_zugeordnet&&!b.quellname.empty()?
                b.quellname:name(b.quelle))
           <<",\"gruppe\":"<<text(std::string(b.gruppe))<<",\"rolle\":"<<text(name(b.rolle))
           <<",\"punktart\":"<<text(name(b.punktart))<<",\"zustand\":"<<text(name(b.zustand))
           <<",\"fehler_id\":";
        if(b.fehler_id.empty()) out<<"null";else out<<text(b.fehler_id);
        out<<",\"moment_s_nm\":";
        if(werte) array(out,b.moment_s_nm);else out<<"null";
        out<<'}';
    }
    out<<"],\"elementvolumina\":[";
    first=true;
    for(const auto& e:k.elementvolumina) {
        if(!first) out<<',';
        first=false;
        out<<"{\"element_id\":"<<text(e.first)<<",\"volumen_m3\":"<<e.second.volumen_m3
           <<",\"geflutet\":"<<(e.second.geflutet?"true":"false")
           <<",\"auftriebspunkt_modell_m\":";
        if(e.second.schwerpunkt_m) array(out,*e.second.schwerpunkt_m);else out<<"null";
        out<<'}';
    }
    out<<"],\"moment_modell_nm\":";array(out,k.moment_modell_nm);
    out<<",\"moment_bezug_modell_m\":";array(out,m.schwerpunkt_m);
    out<<",\"beitraege_auswertung\":\"gelieferter_zustand\",\"summen\":{\"gesamt\":";
    auto summe=[&](const Summe& sum) {
        out<<"{\"gruppe\":"<<text(sum.gruppe)<<",\"frame\":\"modell\",\"kraft_modell_n\":";
        array(out,sum.kraft_modell_n);out<<",\"moment_s_nm\":";array(out,sum.moment_s_nm);
        out<<",\"quellen\":[";
        for(std::size_t i=0;i<sum.quellen.size();++i) out<<(i?",":"")<<text(sum.quellen[i]);
        out<<"]}";
    };
    summe(b2.gesamt);out<<",\"gruppen\":[";
    for(std::size_t i=0;i<b2.gruppen.size();++i) {if(i) out<<',';summe(b2.gruppen[i]);}
    out<<"]},\"bilanzstatus\":"<<text(b2.status)<<",\"erwartete_beitraege\":[";
    for(std::size_t i=0;i<b2.erwartete_beitraege.size();++i) out<<(i?",":"")<<text(b2.erwartete_beitraege[i]);
    out<<"],\"fehlende_beitraege\":[";
    for(std::size_t i=0;i<b2.fehlende_beitraege.size();++i) out<<(i?",":"")<<text(b2.fehlende_beitraege[i]);
    out<<"],\"gleichgewicht\":";
    if(gg) {
        out<<"{\"krafttoleranz_n\":"<<gg->krafttoleranz_n<<",\"momenttoleranz_nm\":"<<gg->momenttoleranz_nm
           <<",\"residuum_kraft_n\":"<<gg->residuum_kraft_n<<",\"residuum_moment_nm\":"<<gg->residuum_moment_nm
           <<",\"freiheitsgrade\":[";
        for(std::size_t i=0;i<gg->freiheitsgrade.size();++i) out<<(i?",":"")<<text(gg->freiheitsgrade[i]);
        out<<"],\"iterationen\":"<<gg->iterationen<<'}';
    } else out<<"null";
    if(s.testantrieb) {
        const auto& c=*s.testantrieb;
        out<<",\"testantrieb\":{\"modus\":\"temporaerer_bugtest\",\"konfiguration\":{\"kraft_n\":"
           <<c.konfiguration.kraft_n<<",\"grenze_m_s\":"<<c.konfiguration.grenze_m_s
           <<"},\"aktiv\":"<<(c.aktiv?"true":"false")
           <<",\"geometriequelle\":"<<text(c.geometriequelle)
           <<",\"modell_fassung\":"<<text(c.modell_fassung)<<",\"zugpunkt_modell_m\":";
        if(c.zugpunkt_modell_m) array(out,*c.zugpunkt_modell_m);else out<<"null";
        out<<",\"schwerpunkt_modell_m\":";array(out,m.schwerpunkt_m);
        out<<",\"schub_welt_n\":";
        Vektor3 schub{};
        for(const auto& beitrag:k.beitraege) if(beitrag.quelle==Quelle::testantrieb)
            schub=s.pose.modellvektor_nach_welt(beitrag.vektor);
        array(out,schub);
        out<<",\"auswertung_schritt\":"<<c.auswertung_schritt
           <<",\"auswertung_zeit_s\":"<<c.auswertung_zeit_s
           <<",\"letzte_ereigniskennung\":"<<c.letzte_ereigniskennung<<",\"uebergang\":";
        if(!c.uebergang) out<<"null";
        else {
            const auto& u=*c.uebergang;const auto& b=u.begrenzung;
            out<<"{\"von_schritt\":"<<u.von_schritt<<",\"nach_schritt\":"<<u.nach_schritt
               <<",\"startzeit_s\":"<<u.startzeit_s<<",\"dt_s\":"<<u.dt_s
               <<",\"aktiv\":"<<(c.aktiv?"true":"false")<<",\"ereignisse\":[";
            for(std::size_t i=0;i<u.ereignisse.size();++i) {
                if(i) out<<',';
                out<<"{\"kennung\":"<<u.ereignisse[i].kennung<<",\"aktiv\":"
                   <<(u.ereignisse[i].aktiv?"true":"false")<<'}';
            }
            out<<"],\"begrenzung\":{\"wirksam\":"<<(b.wirksam?"true":"false")
               <<",\"vorher_welt_m_s\":";array(out,b.vorher_welt_m_s);
            out<<",\"nachher_welt_m_s\":";array(out,b.nachher_welt_m_s);
            out<<",\"korrekturimpuls_ns\":";array(out,b.korrekturimpuls_ns);
            if(!std::isfinite(b.energieaenderung_j))
                throw std::invalid_argument("CORE-SIM-0001: Begrenzerenergie nicht endlich.");
            out<<",\"energieaenderung_j\":"<<b.energieaenderung_j<<"}}";
        }
        out<<'}';
    }
    if(s.hydrodynamik) {
        const auto& c=s.hydrodynamik->konfiguration;
        out<<",\"hydrodynamik\":{\"rho_kg_m3\":"<<c.rho_kg_m3
           <<",\"nu_m2_s\":"<<c.nu_m2_s<<",\"fluidgeschwindigkeit_welt_m_s\":";
        array(out,c.fluidgeschwindigkeit_welt_m_s);
        out<<",\"modell_fassung\":"<<text(s.hydrodynamik->modell_fassung)
           <<",\"auswertung_schritt\":"<<s.hydrodynamik->auswertung_schritt
           <<",\"auswertung_zeit_s\":"<<s.hydrodynamik->auswertung_zeit_s<<'}';
    }
    out<<'}';return out.str();
}

Laufbericht simulieren(const std::filesystem::path& path,const std::filesystem::path& ausgabe,
    const Laufoptionen& o)
{
    const auto start=std::chrono::steady_clock::now();
    testmessungen_zuruecksetzen();
    Laufbericht result;
    try {
        if(!std::isfinite(o.dt_s)||o.dt_s<=0.||!std::isfinite(o.dauer_s)||o.dauer_s<0.||
            !std::isfinite(o.start.zeit_s)||!o.ausgabe_intervall || o.dauer_s/o.dt_s>100000000.)
            throw std::invalid_argument("CORE-SIM-0001: Ungueltige Laufparameter.");
        const double end=o.start.zeit_s+o.dauer_s;
        if(o.dynamisch && (!std::isfinite(end) || (o.dauer_s>0. && end<=o.start.zeit_s)))
            throw std::invalid_argument("CORE-SIM-0001: Laufende nicht endlich oder ohne Zeitfortschritt.");
        if (std::filesystem::exists(ausgabe) && std::filesystem::equivalent(path,ausgabe))
            throw std::invalid_argument("CORE-SIM-0001: Ausgabedatei darf nicht Modelldatei sein.");
        if (std::filesystem::exists(ausgabe))
            throw std::invalid_argument("CORE-SIM-0001: Ausgabedatei existiert bereits; kein Ueberschreiben.");
        const auto m=lade_modell(path);
        Zustand s=o.start;
        if(o.testantrieb) s=testantrieb_start(m,s,*o.testantrieb,o.testantrieb_aktiv);
        if(o.hydrodynamik && !o.dynamisch)
            throw std::invalid_argument("CORE-HYD-0001: Hydrodynamik ist nur fuer dynamische Laeufe verfuegbar.");
        const bool rumpfvorhanden=std::any_of(m.elemente.begin(),m.elemente.end(),
            [](const Element& e) {return e.hydrodynamik_rolle==HydrodynamikRolle::rumpf;});
        if(o.hydrodynamik) s=hydrodynamik_start(m,s,*o.hydrodynamik);
        else if(o.dynamisch && rumpfvorhanden && !s.hydrodynamik &&
            !s.hydrodynamik_erforderlich) {
            HydrodynamikKonfiguration standardwasser;
            s=hydrodynamik_start(m,s,standardwasser);
        }
        if(!o.dynamisch && (s.hydrodynamik || s.hydrodynamik_erforderlich))
            throw std::invalid_argument("CORE-HYD-0001: Hydrodynamikkontext in statischem Lauf.");
        if((!o.testantrieb&&!s.testantrieb) &&
            (o.testantrieb_aktiv||!o.testantrieb_ereignisse.empty()))
            throw std::invalid_argument("CORE-SIM-0001: Teststeuerung ohne Konfiguration.");
        if(!o.dynamisch && ((s.testantrieb&&s.testantrieb->aktiv) ||
            !o.testantrieb_ereignisse.empty()))
            throw std::invalid_argument("CORE-SIM-0001: Aktiver Testantrieb oder Ereignisse in Statik.");
        std::size_t letzte_grenze=s.schritt,letzte_kennung=s.testantrieb?s.testantrieb->letzte_ereigniskennung:0;
        for(const auto& e:o.testantrieb_ereignisse) {
            if(e.first<letzte_grenze || e.second.kennung<=letzte_kennung)
                throw std::invalid_argument("CORE-SIM-0001: Ungeordnete Testereignisse.");
            letzte_grenze=e.first;letzte_kennung=e.second.kennung;
        }
        std::ofstream out(ausgabe,std::ios::binary);
        if(!out) throw std::runtime_error("CORE-IO-0001: Ergebnisdatei nicht schreibbar.");
        if(!o.jsonl) out<<'[';
        auto schreiben=[&](const Zustand& s,const std::string& status,
            const Gleichgewichtsangaben* gg=nullptr) {
            SEGEL_MESSUNG("ergebnisaufzeichnung");
            if(!o.jsonl && result.zustaende) out<<',';
            out<<zustand_json(m,s,status,result.diagnose,gg);
            if(o.jsonl) out<<'\n';
            if(!out) throw std::runtime_error("CORE-IO-0001: Ergebnis konnte nicht geschrieben werden.");
            ++result.zustaende;
        };
        if(o.dynamisch) {
            result.status="abgeschlossen";
            if(o.dauer_s>0.) schreiben(s,"laufend");
            const double anfang=s.zeit_s;
            std::size_t tick=0;
            while(s.zeit_s<end) {
                if(o.abbruch&&o.abbruch->load()) {result.status="abgebrochen";break;}
                ++tick;
                const double ziel=std::min(end,anfang+static_cast<double>(tick)*o.dt_s);
                const double dt=ziel-s.zeit_s;
                std::vector<TestantriebEreignis> ereignisse;
                for(const auto& e:o.testantrieb_ereignisse) if(e.first==s.schritt) ereignisse.push_back(e.second);
                auto next=dynamikschritt(m,s,dt,ereignisse);
                if(next.zeit_s<=s.zeit_s) throw std::runtime_error("CORE-SIM-0001: Kein Zeitfortschritt.");
                next.zeit_s=ziel;
                if(next.testantrieb) {
                    auto context=std::make_shared<TestantriebKontext>(*next.testantrieb);
                    context->auswertung_zeit_s=ziel;next.testantrieb=context;
                }
                if(next.hydrodynamik) {
                    auto context=std::make_shared<HydrodynamikKontext>(*next.hydrodynamik);
                    context->auswertung_zeit_s=ziel;next.hydrodynamik=context;
                }
                s=next;
                if(s.schritt%o.ausgabe_intervall==0 && s.zeit_s<end) schreiben(s,"laufend");
            }
            schreiben(s,result.status);
        } else {
            const auto r=gleichgewicht(m,s,.001,.001,200,o.abbruch);
            result.status=r.status;result.diagnose=r.diagnose;
            schreiben(r.zustand,r.status,r.gleichgewicht?&*r.gleichgewicht:nullptr);
        }
        if(!o.jsonl) out<<"]\n";
        out.flush();
        if(!out) throw std::runtime_error("CORE-IO-0001: Ergebnisabschluss fehlgeschlagen.");
        out.close();
        if(out.fail()) throw std::runtime_error("CORE-IO-0001: Dateischluss fehlgeschlagen.");
    } catch(const std::exception& e) {
        result.status="fehlgeschlagen";result.diagnose=e.what();
    }
    result.gesamtlaufzeit_s=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    result.messungen=testmessungen();
    return result;
}
}
