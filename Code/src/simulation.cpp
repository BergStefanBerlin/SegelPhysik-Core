#include "segelphysik/simulation.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <locale>
#include <set>
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
Vektor3 add(const Vektor3& a, const Vektor3& b)
{
    return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}
Vektor3 cross(const Vektor3& a, const Vektor3& b)
{
    return {a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]};
}
bool gleicher_stationaerer_snapshot(const Zustand& zustand, const Zustand& snapshot)
{
    if (zustand.schritt != snapshot.schritt || zustand.zeit_s != snapshot.zeit_s ||
        zustand.pose.position_m() != snapshot.pose.position_m() ||
        zustand.pose.orientierung_xyzw() != snapshot.pose.orientierung_xyzw() ||
        zustand.geschwindigkeit_welt_m_s != snapshot.geschwindigkeit_welt_m_s ||
        zustand.omega_modell_rad_s != snapshot.omega_modell_rad_s ||
        zustand.geflutet != snapshot.geflutet ||
        zustand.bewegungsstellungen.size() != snapshot.bewegungsstellungen.size())
        return false;
    for (std::size_t i = 0; i < zustand.bewegungsstellungen.size(); ++i) {
        const auto& a = zustand.bewegungsstellungen[i];
        const auto& b = snapshot.bewegungsstellungen[i];
        if (a.element_id != b.element_id || a.wert != b.wert) return false;
    }
    return true;
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
    const std::string& diagnose,const Gleichgewichtsangaben* gg,
    const StationaeresGleichgewichtErgebnis* stationaer, ErgebnisJsonFormat format)
{
    if (format == ErgebnisJsonFormat::Kompatibel && stationaer &&
        (stationaer->kurvenfahrt || stationaer->konfiguration.kurvenfahrt))
        throw std::invalid_argument("CORE-SIM-0001: Stationaere Kurvenfahrt benoetigt ErgebnisJsonFormat::V6.");
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
    if(stationaer && !gleicher_stationaerer_snapshot(s, stationaer->zustand))
        throw std::invalid_argument("CORE-SIM-0001: Stationaerer Druckpunktsnapshot passt nicht zum serialisierten Zustand.");
    std::set<std::string> bewegte_ids;
    for (const auto& stellung : s.bewegungsstellungen) {
        const auto element = std::find_if(m.elemente.begin(), m.elemente.end(),
            [&](const Element& candidate) { return candidate.id == stellung.element_id; });
        if (element == m.elemente.end() || !element->bewegung ||
            !bewegte_ids.insert(stellung.element_id).second)
            throw std::invalid_argument("CORE-MOD-0002: Ungueltige oder doppelte Bewegungsstellung.");
        const double minimum = element->bewegung->ruderwinkel ? -90. : 0.;
        const double maximum = element->bewegung->ruderwinkel ? 90. : 100.;
        if (!std::isfinite(stellung.wert) || stellung.wert < minimum || stellung.wert > maximum)
            throw std::invalid_argument("CORE-MOD-0002: Bewegungsstellung ausserhalb des zulaessigen Bereichs.");
    }
    auto k=kraefte(m,s.pose,s.testantrieb.get(),1000.,9.81,s.geflutet,
        s.hydrodynamik.get(),s.geschwindigkeit_welt_m_s,s.omega_modell_rad_s,
        s.bewegungsstellungen);
    if (stationaer && stationaer->status != "ungueltig") {
        const auto force_world = !stationaer->konfiguration.aktiv ? Vektor3{} :
            stationaer->konfiguration.modellfest
            ? s.pose.modellvektor_nach_welt(stationaer->konfiguration.kraft_welt_n)
            : stationaer->konfiguration.kraft_welt_n;
        const auto force_model = s.pose.weltvektor_nach_modell(force_world);
        Beitrag contribution;
        contribution.id = "stationaer.fremdkraft";
        contribution.art = "kraft";
        contribution.quelle = Quelle::externe_kraft;
        contribution.gruppe = "stationaer";
        contribution.punkt_m = stationaer->konfiguration.angriffspunkt_modell_m;
        contribution.vektor = force_model;
        contribution.moment_s_nm = moment_um(contribution.punkt_m,
            stationaer->schwerpunkt_modell_m, force_model);
        contribution.zustand = std::hypot(force_world[0], force_world[1], force_world[2]) == 0.
            ? Datenzustand::null : Datenzustand::gueltig;
        k.kraft_welt_n = add(k.kraft_welt_n, force_world);
        k.moment_modell_nm = add(k.moment_modell_nm, contribution.moment_s_nm);
        k.beitraege.push_back(std::move(contribution));
    }
    const auto b2=bilanz(m,k.beitraege,static_cast<bool>(s.hydrodynamik));
    std::ostringstream out;out.imbue(std::locale::classic());out<<std::setprecision(17);
    const bool beweglich=std::any_of(m.elemente.begin(),m.elemente.end(),
        [](const Element& element) { return element.bewegung.has_value(); });
    const Vektor3 forward = s.pose.modellvektor_nach_welt({1.,0.,0.});
    const Vektor3 omega_world = s.pose.modellvektor_nach_welt(s.omega_modell_rad_s);
    const Vektor3 forward_derivative = cross(omega_world, forward);
    const double forward_horizontal_squared = forward[0]*forward[0] + forward[1]*forward[1];
    std::optional<double> heading_rate;
    std::string heading_rate_status = "nicht_bestimmbar";
    if (std::isfinite(forward_horizontal_squared) && forward_horizontal_squared > 1e-16) {
        const double value = (forward[0]*forward_derivative[1] - forward[1]*forward_derivative[0]) /
            forward_horizontal_squared;
        if (std::isfinite(value)) { heading_rate = value; heading_rate_status = "gueltig"; }
    }
    if (format == ErgebnisJsonFormat::V6 && stationaer && heading_rate_status == "gueltig") {
        heading_rate = stationaer->kursdrehrate_rad_s;
    }
    const char* contract = format == ErgebnisJsonFormat::V6 ? "6" :
        stationaer ? "5" : beweglich ? "4" : s.testantrieb ? "3" : "2";
    out<<"{\"vertragsversion\":"<<text(contract)<<",\"modell_id\":"<<text(m.id)
       <<",\"modell_fassung\":"<<text(m.fassung)
       <<",\"status\":"<<text(status)<<",\"diagnose\":"<<text(diagnose)<<",\"zeit_s\":"<<s.zeit_s
       <<",\"schritt\":"<<s.schritt<<",\"position_m\":";
    array(out,s.pose.position_m());out<<",\"orientierung_xyzw\":";
    array(out,s.pose.orientierung_xyzw());out<<",\"geschwindigkeit_welt_m_s\":";
    array(out,s.geschwindigkeit_welt_m_s);out<<",\"omega_modell_rad_s\":";
    array(out,s.omega_modell_rad_s);
    if (format == ErgebnisJsonFormat::V6) {
        out << ",\"kursdrehrate_rad_s\":";
        if (heading_rate) out << *heading_rate; else out << "null";
        out << ",\"kursdrehrate_status\":" << text(heading_rate_status);
    }
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
           <<",\"modell_status\":"<<text(b.modell_status)
           <<",\"modell_methode\":"<<text(b.modell_methode)
           <<",\"modell_quelle\":"<<text(b.modell_quelle)
           <<",\"modell_unsicherheit\":"<<text(b.modell_unsicherheit)
           <<",\"modell_parameter\":{";
        bool first_parameter = true;
        for (const auto& parameter : b.modell_parameter) {
            const auto& value = parameter.second;
            if (!std::isfinite(value.wert))
                throw std::runtime_error("CORE-SIM-0001: Hydrodynamikparameter nicht endlich.");
            if (!first_parameter) out << ',';
            first_parameter = false;
            out << text(parameter.first) << ":{\"wert\":" << value.wert
                << ",\"einheit\":" << text(value.einheit)
                << ",\"status\":" << text(value.status)
                << ",\"quelle\":" << text(value.quelle)
                << ",\"gueltigkeit\":" << text(value.gueltigkeit)
                << ",\"unsicherheit\":" << text(value.unsicherheit) << '}';
        }
        out << "},\"modell_widerstandsanteile\":[";
        for (std::size_t i = 0; i < b.modell_widerstandsanteile.size(); ++i)
            out << (i ? "," : "") << text(b.modell_widerstandsanteile[i]);
        out << "],\"fehler_id\":";
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
    out<<",\"moment_bezug_modell_m\":";
    if (stationaer) array(out, stationaer->schwerpunkt_modell_m);
    else array(out,m.schwerpunkt_m);
    if(beweglich) {
        out<<",\"bewegungen\":[";
        bool erste_bewegung=true;
        for(const auto& element:m.elemente) {
            if(!element.bewegung) continue;
            const auto stellung=std::find_if(s.bewegungsstellungen.begin(),
                s.bewegungsstellungen.end(),[&](const Bewegungsstellung& wert) {
                    return wert.element_id==element.id;
                });
            const double wert=stellung==s.bewegungsstellungen.end()
                ? (element.bewegung->ruderwinkel?0.:100.)
                : stellung->wert;
            if(!erste_bewegung) out<<',';
            erste_bewegung=false;
            out<<"{\"element_id\":"<<text(element.id)<<",\"wert\":"<<wert<<'}';
        }
        out<<"],\"letzte_bewegungsereigniskennung\":"<<s.letzte_bewegungsereigniskennung
           <<",\"elementtransformationen_modell\":[";
        bool erste_transformation=true;
        for(const auto& element:m.elemente) {
            const auto matrix=bewegungstransformation(m,element.id,s.bewegungsstellungen);
            if(!erste_transformation) out<<',';
            erste_transformation=false;
            out<<"{\"element_id\":"<<text(element.id)<<",\"matrix_zeilenweise\":[";
            for(std::size_t i=0;i<matrix.size();++i) out<<(i?",":"")<<matrix[i];
            out<<"]}";
        }
        out<<"],\"bewegungsuebergang\":";
        if(!s.bewegungsuebergang) out<<"null";
        else {
            const auto& uebergang=*s.bewegungsuebergang;
            out<<"{\"von_schritt\":"<<uebergang.von_schritt
               <<",\"nach_schritt\":"<<uebergang.nach_schritt
               <<",\"startzeit_s\":"<<uebergang.startzeit_s
               <<",\"dt_s\":"<<uebergang.dt_s<<",\"ereignisse\":[";
            for(std::size_t i=0;i<uebergang.ereignisse.size();++i) {
                const auto& ereignis=uebergang.ereignisse[i];
                if(!std::isfinite(ereignis.wert))
                    throw std::invalid_argument("CORE-SIM-0001: Bewegungsereignis nicht endlich.");
                out<<(i?",":"")<<"{\"kennung\":"<<ereignis.kennung
                   <<",\"element_id\":"<<text(ereignis.element_id)
                   <<",\"wert\":"<<ereignis.wert<<'}';
            }
            out<<"]}";
        }
    }
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
    if (stationaer) {
        const auto& config = stationaer->konfiguration;
        const auto& fluid = stationaer->zustand.hydrodynamik
            ? stationaer->zustand.hydrodynamik->konfiguration.fluidgeschwindigkeit_welt_m_s
            : Vektor3{};
        const Vektor3 relative{
            s.geschwindigkeit_welt_m_s[0] - fluid[0],
            s.geschwindigkeit_welt_m_s[1] - fluid[1],
            s.geschwindigkeit_welt_m_s[2] - fluid[2]};
        const double relative_speed = std::sqrt(relative[0]*relative[0] +
            relative[1]*relative[1] + relative[2]*relative[2]);
        out << ",\"stationaer\":{\"aktiv\":" << (config.aktiv ? "true" : "false")
            << ",\"status\":" << text(stationaer->status)
            << ",\"diagnose\":" << text(stationaer->diagnose)
            << ",\"konvergiert\":" << (stationaer->status == "konvergiert" ? "true" : "false")
            << ",\"modellfest\":" << (config.modellfest ? "true" : "false")
            << ",\"kraft_n\":";
        const bool input_finite = std::all_of(config.kraft_welt_n.begin(),
            config.kraft_welt_n.end(), [](double value) { return std::isfinite(value); }) &&
            std::all_of(config.angriffspunkt_modell_m.begin(),
                config.angriffspunkt_modell_m.end(),
                [](double value) { return std::isfinite(value); });
        if (input_finite) array(out, config.kraft_welt_n); else out << "null";
        out << ",\"kraft_bezugssystem\":" << text(config.modellfest ? "modell" : "welt")
            << ",\"angriffspunkt_modell_m\":";
        if (input_finite) array(out, config.angriffspunkt_modell_m); else out << "null";
        out << ",\"stellungen\":[";
        for (std::size_t i = 0; i < config.stellungen.size(); ++i) {
            const auto& setting = config.stellungen[i];
            out << (i ? "," : "") << "{\"element_id\":" << text(setting.element_id)
                << ",\"wert\":";
            if (std::isfinite(setting.wert)) out << setting.wert; else out << "null";
            out << '}';
        }
        out << "],\"iterationen\":" << stationaer->iterationen
            << ",\"flutungswiederholungen\":" << stationaer->flutungswiederholungen
            << ",\"residuum_kraft_welt_n\":";
        array(out, stationaer->residuum_kraft_welt_n);
        out << ",\"residuum_moment_modell_nm\":";
        array(out, stationaer->residuum_moment_modell_nm);
        out << ",\"krafttoleranz_n\":" << stationaer->krafttoleranz_n
            << ",\"momenttoleranz_nm\":" << stationaer->momenttoleranz_nm
            << ",\"referenzkurs_verwendet\":"
            << (stationaer->referenzkurs_verwendet ? "true" : "false")
            << ",\"geschwindigkeit_relativ_wasser_m_s\":";
        array(out, relative);
        out << ",\"geschwindigkeit_relativ_wasser_betrag_m_s\":" << relative_speed
            << ",\"abdriftwinkel_rad\":";
        if (relative_speed > 0.) out << std::atan2(relative[1], relative[0]);
        else if (format == ErgebnisJsonFormat::V6) out << "null";
        else out << 0.;
        if (format == ErgebnisJsonFormat::V6) {
            out << ",\"kurvenfahrt\":" << ((stationaer->kurvenfahrt || config.kurvenfahrt) ? "true" : "false")
                << ",\"loesungstyp\":" << text(stationaer->loesungstyp)
                << ",\"omega_modell_rad_s\":";
            array(out, stationaer->omega_modell_rad_s);
            out << ",\"aeussere_kraft_welt_n\":";
            array(out, stationaer->aeussere_kraft_welt_n);
            out << ",\"aeusseres_moment_modell_nm\":";
            array(out, stationaer->aeusseres_moment_modell_nm);
            out << ",\"residuum_kraft_bewegung_welt_n\":";
            array(out, stationaer->residuum_bewegung_kraft_welt_n);
            out << ",\"residuum_moment_bewegung_modell_nm\":";
            array(out, stationaer->residuum_bewegung_moment_modell_nm);
            out << ",\"radius_m\":";
            if (stationaer->radius_m && std::isfinite(*stationaer->radius_m)) out << *stationaer->radius_m;
            else out << "null";
            const char* radius_status = stationaer->radius_m ? "gueltig" :
                stationaer->loesungstyp == "geradeaus" ? "geradeaus" :
                stationaer->loesungstyp == "reine_drehung" ? "reine_drehung" : "nicht_bestimmbar";
            out << ",\"radius_status\":" << text(radius_status) << "}";
        } else {
            out << ",\"nullen\":{\"vz_m_s\":0,\"omega_modell_rad_s\":[0,0,0]}}";
        }
    }
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
        if(const auto* ht=s.hydrodynamik->traegheit.get()) {
            auto wert=[&](const char* name,const TraegheitsWert& w) {
                out<<'"'<<name<<"\":{\"wert\":"<<w.wert<<",\"status\":"<<text(w.status)
                   <<",\"methode\":"<<text(w.methode)<<",\"quelle\":"<<text(w.quelle)
                   <<",\"unsicherheit\":"<<text(w.unsicherheit)<<'}';
            };
            static const char* namen[6]={"a11","a22","a33","a44","a55","a66"};
            out<<",\"hydrodynamische_traegheit\":{\"zusatzmasse\":{";
            for(std::size_t i=0;i<6;++i) {
                if(i) out<<',';
                wert(namen[i],ht->zusatzmasse[i]);
            }
            out<<"},\"daempfung\":{";
            wert("zeta_rollen",ht->zeta_rollen);out<<',';
            wert("zeta_stampfen",ht->zeta_stampfen);out<<',';
            wert("zeta_tauchen",ht->zeta_tauchen);out<<',';
            wert("gieren_linear",ht->gieren_linear);out<<',';
            wert("gieren_quadratisch",ht->gieren_quadratisch);
            out<<"},\"effektive_masse_kg\":";array(out,ht->effektive_masse_kg);
            out<<",\"effektive_traegheit_kg_m2\":";array(out,ht->effektive_traegheit_kg_m2);
            out<<",\"rueckstellung\":{\"c33_n_m\":"<<ht->c33_n_m<<",\"c44_nm_rad\":"<<ht->c44_nm_rad
               <<",\"c55_nm_rad\":"<<ht->c55_nm_rad<<"},\"diagnosen\":[";
            for(std::size_t i=0;i<ht->diagnosen.size();++i) {
                if(i) out<<',';
                out<<text(ht->diagnosen[i]);
            }
            out<<"]}";
        }
    }
    const auto ldp=lateraldruckpunkt(m,s.pose,k,s.bewegungsstellungen);
    if(ldp.vorhanden || stationaer) {
        out<<",\"druckpunkte\":{";
        bool has_pressure_point = false;
        if(ldp.vorhanden) {
            out<<"\"lateral\":{\"auswertung_version\":2,\"status\":"<<text(ldp.status)
               <<",\"querkraft_modell_n\":"<<ldp.querkraft_modell_n
               <<",\"lateralflaeche_m2\":"<<ldp.lateralflaeche_m2;
            if(ldp.position_modell_m) {
                out<<",\"position_modell_m\":";array(out,*ldp.position_modell_m);
            }
            out<<",\"quellen\":[";
            for(std::size_t i=0;i<ldp.quellen.size();++i) {
                if(i) out<<',';
                out<<text(ldp.quellen[i]);
            }
            out<<"]}";
            has_pressure_point = true;
        }
        if(stationaer) {
            const auto& config = stationaer->konfiguration;
            const bool finite_force = std::all_of(config.kraft_welt_n.begin(),
                config.kraft_welt_n.end(), [](double value) { return std::isfinite(value); });
            const bool finite_point = std::all_of(config.angriffspunkt_modell_m.begin(),
                config.angriffspunkt_modell_m.end(),
                [](double value) { return std::isfinite(value); });
            if (!finite_force || !finite_point)
                throw std::invalid_argument("CORE-SIM-0001: Stationaerer Fremdkraft-Snapshot ist nicht endlich.");
            const bool force_null = std::all_of(config.kraft_welt_n.begin(),
                config.kraft_welt_n.end(), [](double value) { return value == 0.; });
            const char* ersatz_status = !config.aktiv ? "inaktiv" :
                force_null ? "kraft_null" : "gueltig";
            if (has_pressure_point) out<<',';
            out<<"\"ersatz_segel\":{\"status\":"<<text(ersatz_status)
               <<",\"quelle\":\"fremdkraft\"";
            if (std::string(ersatz_status) == "gueltig") {
                out<<",\"position_modell_m\":";
                array(out,config.angriffspunkt_modell_m);
            }
            out<<'}';
        }
        out<<'}';
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
        for(const auto& element:m.elemente) if(element.bewegung) {
            const auto fund=std::find_if(s.bewegungsstellungen.begin(),s.bewegungsstellungen.end(),
                [&](const Bewegungsstellung& stellung) { return stellung.element_id==element.id; });
            if(fund==s.bewegungsstellungen.end())
                s.bewegungsstellungen.push_back({element.id,
                    element.bewegung->ruderwinkel?0.:100.});
        }
        if(o.testantrieb) s=testantrieb_start(m,s,*o.testantrieb,o.testantrieb_aktiv);
        const bool stationaer_aktiv = o.stationaer.has_value();
        if(o.hydrodynamik && !o.dynamisch && !stationaer_aktiv)
            throw std::invalid_argument("CORE-HYD-0001: Hydrodynamik ist nur fuer dynamische Laeufe verfuegbar.");
        if(stationaer_aktiv && o.dynamisch)
            throw std::invalid_argument("CORE-SIM-0001: Stationaerer Gleichgewichtslauf ist nicht dynamisch.");
        const bool hydrodynamikvorhanden=std::any_of(m.elemente.begin(),m.elemente.end(),
            [](const Element& e) {return e.hydrodynamik_rolle.has_value();});
        if(o.hydrodynamik && o.dynamisch) s=hydrodynamik_start(m,s,*o.hydrodynamik);
        else if(o.dynamisch && hydrodynamikvorhanden && !s.hydrodynamik &&
            !s.hydrodynamik_erforderlich) {
            HydrodynamikKonfiguration standardwasser;
            s=hydrodynamik_start(m,s,standardwasser);
        }
        if(!o.dynamisch && !stationaer_aktiv && (s.hydrodynamik || s.hydrodynamik_erforderlich))
            throw std::invalid_argument("CORE-HYD-0001: Hydrodynamikkontext in statischem Lauf.");
        if((!o.testantrieb&&!s.testantrieb) &&
            (o.testantrieb_aktiv||!o.testantrieb_ereignisse.empty()))
            throw std::invalid_argument("CORE-SIM-0001: Teststeuerung ohne Konfiguration.");
        if(!o.dynamisch && ((s.testantrieb&&s.testantrieb->aktiv) ||
            !o.testantrieb_ereignisse.empty() || !o.bewegungsereignisse.empty()))
            throw std::invalid_argument("CORE-SIM-0001: Aktiver Testantrieb oder Ereignisse in Statik.");
        std::size_t letzte_grenze=s.schritt,letzte_kennung=s.testantrieb?s.testantrieb->letzte_ereigniskennung:0;
        for(const auto& e:o.testantrieb_ereignisse) {
            if(e.first<letzte_grenze || e.second.kennung<=letzte_kennung)
                throw std::invalid_argument("CORE-SIM-0001: Ungeordnete Testereignisse.");
            letzte_grenze=e.first;letzte_kennung=e.second.kennung;
        }
        letzte_grenze=s.schritt;
        letzte_kennung=s.letzte_bewegungsereigniskennung;
        for(const auto& e:o.bewegungsereignisse) {
            if(e.first<letzte_grenze || e.second.kennung<=letzte_kennung)
                throw std::invalid_argument("CORE-SIM-0001: Ungeordnete Bewegungsereignisse.");
            letzte_grenze=e.first;letzte_kennung=e.second.kennung;
            const auto element=std::find_if(m.elemente.begin(),m.elemente.end(),
                [&](const Element& candidate) { return candidate.id==e.second.element_id; });
            if(element==m.elemente.end() || !element->bewegung)
                throw std::invalid_argument("CORE-MOD-0002: Bewegungsereignis fuer Element ohne Bewegung: "+
                    e.second.element_id);
            const double minimum=element->bewegung->ruderwinkel?-90.:0.;
            const double maximum=element->bewegung->ruderwinkel?90.:100.;
            if(!std::isfinite(e.second.wert)||e.second.wert<minimum||e.second.wert>maximum)
                throw std::invalid_argument("CORE-MOD-0002: Bewegungsereignis ausserhalb des zulaessigen Bereichs: "+
                    e.second.element_id);
        }
        std::set<std::string> start_bewegungen;
        for(const auto& stellung:s.bewegungsstellungen) {
            const auto element=std::find_if(m.elemente.begin(),m.elemente.end(),
                [&](const Element& candidate) { return candidate.id==stellung.element_id; });
            if(element==m.elemente.end() || !element->bewegung ||
                !start_bewegungen.insert(stellung.element_id).second)
                throw std::invalid_argument("CORE-MOD-0002: Ungueltige oder doppelte Bewegungsstellung im Startzustand.");
            const double minimum=element->bewegung->ruderwinkel?-90.:0.;
            const double maximum=element->bewegung->ruderwinkel?90.:100.;
            if(!std::isfinite(stellung.wert)||stellung.wert<minimum||stellung.wert>maximum)
                throw std::invalid_argument("CORE-MOD-0002: Bewegungsstellung im Startzustand ausserhalb des Bereichs.");
        }
        std::ofstream out(ausgabe,std::ios::binary);
        if(!out) throw std::runtime_error("CORE-IO-0001: Ergebnisdatei nicht schreibbar.");
        if(!o.jsonl) out<<'[';
        auto schreiben=[&](const Zustand& s,const std::string& status,
            const Gleichgewichtsangaben* gg=nullptr,
            const StationaeresGleichgewichtErgebnis* stationaer=nullptr) {
            SEGEL_MESSUNG("ergebnisaufzeichnung");
            if(!o.jsonl && result.zustaende) out<<',';
            out<<zustand_json(m,s,status,result.diagnose,gg,stationaer,o.ergebnis_json_format);
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
                std::vector<Bewegungsereignis> bewegungsereignisse;
                for(const auto& e:o.bewegungsereignisse)
                    if(e.first==s.schritt) bewegungsereignisse.push_back(e.second);
                auto next=dynamikschritt(m,s,dt,ereignisse,bewegungsereignisse);
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
            if (stationaer_aktiv) {
                const auto hydro = o.hydrodynamik.value_or(HydrodynamikKonfiguration{});
                const auto r = stationaeres_gleichgewicht(m, s, *o.stationaer,
                    hydro, .05, .05, 100, o.abbruch);
                result.status = r.status;
                result.diagnose = r.diagnose;
                schreiben(r.zustand, r.status, nullptr, &r);
            } else {
                const auto r=gleichgewicht(m,s,.001,.001,200,o.abbruch);
                result.status=r.status;result.diagnose=r.diagnose;
                schreiben(r.zustand,r.status,r.gleichgewicht?&*r.gleichgewicht:nullptr);
            }
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
