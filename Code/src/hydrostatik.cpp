#include "segelphysik/hydrostatik.hpp"
#include "segelphysik/messung.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <locale>
#include <sstream>
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
const Element* bewegungseigentuemer(const Modell& modell, const std::string& element_id)
{
    for (const auto& element : modell.elemente) {
        if (!element.bewegung) continue;
        if (element.id == element_id ||
            std::find(element.bewegung->mitbewegte_elemente.begin(),
                element.bewegung->mitbewegte_elemente.end(), element_id) !=
                element.bewegung->mitbewegte_elemente.end())
            return &element;
    }
    return nullptr;
}
double bewegungswert(const Element& element,
    const std::vector<Bewegungsstellung>& stellungen)
{
    const auto fund = std::find_if(stellungen.begin(), stellungen.end(),
        [&](const Bewegungsstellung& stellung) { return stellung.element_id == element.id; });
    if (fund != stellungen.end()) return fund->wert;
    return element.bewegung->ruderwinkel ? 0. : 100.;
}
Vektor3 bewegungspunkt(const Vektor3& punkt, const Element& eigentuemer,
    double wert)
{
    const auto& bewegung = *eigentuemer.bewegung;
    if (bewegung.art == Bewegungsart::heben)
        return plus(punkt, mal(bewegung.richtung_modell,
            bewegung.hubweg_m * (100. - wert) / 100.));
    const bool ruder=bewegung.ruderwinkel;
    const double winkel = ruder ? wert * 3.14159265358979323846 / 180. :
        (100. - wert) * 3.14159265358979323846 / 200.;
    const auto achse = bewegung.achse_modell;
    const auto rel = plus(punkt, mal(bewegung.drehpunkt_modell_m, -1.));
    const auto gedreht = plus(plus(mal(rel, std::cos(winkel)),
        mal(cross(achse, rel), std::sin(winkel))),
        mal(achse, dot(achse, rel) * (1. - std::cos(winkel))));
    return plus(bewegung.drehpunkt_modell_m, gedreht);
}
Mesh bewegtes_mesh(const Mesh& mesh, const Element& eigentuemer, double wert)
{
    Mesh result = mesh;
    for (auto& punkt : result.punkte) punkt = bewegungspunkt(punkt, eigentuemer, wert);
    return result;
}
Vektor3 bewegungsvektor(const Vektor3& vektor, const Element& eigentuemer, double wert)
{
    return plus(bewegungspunkt(vektor, eigentuemer, wert),
        mal(bewegungspunkt({}, eigentuemer, wert), -1.));
}
Vektor3 einheitsvektor(Vektor3 vektor)
{
    const double laenge = std::sqrt(dot(vektor, vektor));
    if (!std::isfinite(laenge) || laenge <= 1e-12)
        throw std::invalid_argument("CORE-HYD-0001: Hydrodynamische Profilachse ist singulaer.");
    return mal(vektor, 1. / laenge);
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
bool endlich(const Vektor3& v)
{
    return std::all_of(v.begin(), v.end(), [](double value) { return std::isfinite(value); });
}

struct Nasspanel {
    Vektor3 punkt_welt{};
    Vektor3 normale_welt{};
    double flaeche_m2{};
};

std::vector<Nasspanel> nasspanels(const Mesh& mesh, const Pose& pose)
{
    std::vector<Nasspanel> result;
    for (const auto& triangle : mesh.dreiecke) {
        std::array<Vektor3, 3> vertices{
            pose.modellpunkt_nach_welt(mesh.punkte.at(triangle[0])),
            pose.modellpunkt_nach_welt(mesh.punkte.at(triangle[1])),
            pose.modellpunkt_nach_welt(mesh.punkte.at(triangle[2]))};
        std::array<Vektor3, 4> clipped{};
        std::size_t count = 0;
        for (std::size_t i = 0; i < vertices.size(); ++i) {
            const auto& a = vertices[i];
            const auto& b = vertices[(i + 1) % vertices.size()];
            const double da = a[2], db = b[2];
            if (da <= 0.) clipped[count++] = a;
            if ((da < 0. && db > 0.) || (da > 0. && db < 0.)) {
                const double t = da / (da - db);
                clipped[count++] = plus(a, mal(plus(b, mal(a, -1.)), t));
            }
        }
        for (std::size_t i = 1; i + 1 < count; ++i) {
            const auto ab = plus(clipped[i], mal(clipped[0], -1.));
            const auto ac = plus(clipped[i + 1], mal(clipped[0], -1.));
            const auto area_vector = cross(ab, ac);
            const double twice_area = std::sqrt(dot(area_vector, area_vector));
            if (!std::isfinite(twice_area))
                throw std::overflow_error("CORE-HYD-0001: Benetztes Meshpanel nicht endlich.");
            if (twice_area <= 1e-14) continue;
            result.push_back({mal(plus(plus(clipped[0], clipped[i]), clipped[i + 1]), 1. / 3.),
                mal(area_vector, 1. / twice_area), .5 * twice_area});
        }
    }
    return result;
}

const HydrodynamikKraftmodell* kraftmodell(const Element& element, const std::string& art)
{
    if (!element.hydrodynamik_daten) return nullptr;
    const auto found = element.hydrodynamik_daten->modelle.find(art);
    return found == element.hydrodynamik_daten->modelle.end() ? nullptr : &found->second;
}

bool hat_widerstandsanteil(const HydrodynamikKraftmodell* model, const std::string& name)
{
    return model && std::find(model->widerstandsanteile.begin(),
        model->widerstandsanteile.end(), name) != model->widerstandsanteile.end();
}

bool polare_enthaelt_hautreibung(const HydrodynamikKraftmodell* model)
{
    return model && !model->kennfeld.empty() &&
        (hat_widerstandsanteil(model, "gesamt") ||
         hat_widerstandsanteil(model, "profil") ||
         hat_widerstandsanteil(model, "hautreibung"));
}

double parameterwert(const HydrodynamikKraftmodell* model, const std::string& key, double fallback)
{
    if (!model) return fallback;
    const auto found = model->parameter.find(key);
    return found == model->parameter.end() ? fallback : found->second.wert;
}

std::string parameterstatus(const HydrodynamikKraftmodell* model)
{
    if (!model) return "geschaetzt";
    bool has_derived = false, has_validated = false;
    for (const auto& parameter : model->parameter) {
        if (parameter.second.status == "geschaetzt") return "geschaetzt";
        if (parameter.second.status == "abgeleitet") has_derived = true;
        if (parameter.second.status == "validiert") has_validated = true;
    }
    for (const auto& point : model->kennfeld) {
        if (point.status == "geschaetzt") return "geschaetzt";
        if (point.status == "abgeleitet") has_derived = true;
        if (point.status == "validiert") has_validated = true;
    }
    if (has_derived) return "abgeleitet";
    return has_validated ? "validiert" : "geschaetzt";
}

void provenance(Beitrag& beitrag, const HydrodynamikKraftmodell* model,
    const std::string& fallback_method, const std::string& fallback_source,
    const std::string& fallback_uncertainty)
{
    beitrag.modell_status = parameterstatus(model);
    beitrag.modell_methode = model ? model->methode : fallback_method;
    beitrag.modell_quelle = model ? model->quelle : fallback_source;
    beitrag.modell_unsicherheit = fallback_uncertainty;
    if (model) {
        beitrag.modell_parameter = model->parameter;
        beitrag.modell_widerstandsanteile = model->widerstandsanteile;
    }
    if (model) {
        for (const auto& parameter : model->parameter) {
            if (parameter.second.status == "geschaetzt") {
                beitrag.modell_status = "geschaetzt";
                beitrag.modell_quelle = parameter.second.quelle;
                beitrag.modell_unsicherheit = parameter.second.unsicherheit;
                break;
            }
        }
        if (model->kennfeld.size() > 0) {
            for (const auto& point : model->kennfeld) {
                if (point.status == "geschaetzt") {
                    beitrag.modell_status = "geschaetzt";
                    beitrag.modell_quelle = point.quelle;
                    beitrag.modell_unsicherheit = point.unsicherheit;
                    break;
                }
            }
        }
    }
}

void verwendeten_parameter(Beitrag& beitrag, const std::string& key, double value,
    const std::string& unit, const std::string& status, const std::string& source,
    const std::string& validity, const std::string& uncertainty)
{
    const auto inserted = beitrag.modell_parameter.emplace(key,
        HydrodynamikParameter{value, unit, status, source, validity, uncertainty});
    const auto& effective = inserted.first->second;
    if (effective.status == "geschaetzt") beitrag.modell_status = "geschaetzt";
    else if (effective.status == "abgeleitet" && beitrag.modell_status == "validiert")
        beitrag.modell_status = "abgeleitet";
}

Vektor3 lokale_relativgeschwindigkeit(const Vektor3& point_welt, const Vektor3& cg_welt,
    const Vektor3& velocity_welt, const Vektor3& omega_welt,
    const Vektor3& fluid_welt)
{
    const auto rotational = cross(omega_welt, plus(point_welt, mal(cg_welt, -1.)));
    return plus(fluid_welt, mal(plus(velocity_welt, rotational), -1.));
}

double reibungsbeiwert(double reynolds)
{
    if (reynolds <= 0.) return 0.;
    const double laminar = 1.328 / std::sqrt(reynolds);
    if (reynolds < 5e5) return laminar;
    const double turbulent = .075 / std::pow(std::log10(reynolds) - 2., 2.);
    if (reynolds < 1e6) {
        const double t = (reynolds - 5e5) / 5e5;
        return laminar * (1. - t) + turbulent * t;
    }
    return turbulent;
}

Beitrag basisbeitrag(const Element& element, const std::string& art)
{
    Beitrag result;
    result.id = element.id + ":hydrodynamik:" + art;
    result.element_id = element.id;
    result.art = "kraft";
    result.quelle = Quelle::hydrodynamik;
    result.gruppe = "hydrodynamik";
    result.punkt_m = element.schwerpunkt_m;
    result.punktart = Punktart::angriffspunkt;
    return result;
}

Beitrag reibungsbeitrag(const Element& element, const Modell& modell, const Pose& pose,
    const HydrodynamikKonfiguration& config, const Vektor3& velocity,
    const Vektor3& omega, const std::vector<Nasspanel>& panels)
{
    const auto* model = kraftmodell(element, "hautreibung");
    const auto* side_model = kraftmodell(element, "seitenkraft");
    if (element.hydrodynamik_rolle != HydrodynamikRolle::rumpf &&
        polare_enthaelt_hautreibung(side_model)) {
        auto result = basisbeitrag(element, "reibung");
        provenance(result, side_model, "reibungsanteil_im_polarenkennfeld",
            "Im expliziten Seitenkraftkennfeld enthalten",
            "Keine separate Hautreibung, um Doppelzählung des Kennfeldanteils zu vermeiden");
        result.zustand = Datenzustand::null;
        return result;
    }
    const auto method = model ? model->methode : "ittc_1957_mit_laminarer_naeherung";
    if (method != "ittc_1957_mit_laminarer_naeherung" && method != "ittc_1957")
        throw std::invalid_argument("CORE-HYD-0001: Unbekannte Hautreibungs-Methode '" + method + "'");
    auto result = basisbeitrag(element, "reibung");
    provenance(result, model, method,
        "Geschaetzt: ITTC-1957-Reibungslinie plus laminare Plattennaeherung",
        "Uebertragung der Plattenkorrelation auf lokale Meshpanels; Oberflaechenrauheit nicht modelliert");
    if (panels.empty()) {
        result.zustand = Datenzustand::null;
        return result;
    }
    const auto omega_welt = pose.modellvektor_nach_welt(omega);
    const auto cg_welt = pose.modellpunkt_nach_welt(modell.schwerpunkt_m);
    Vektor3 total_force{}, total_moment{}, application_point{};
    double point_weight = 0.;
    double min_re = std::numeric_limits<double>::infinity(), max_re = 0.;
    double min_cf = std::numeric_limits<double>::infinity(), max_cf = 0.;
    for (const auto& panel : panels) {
        const auto relative = lokale_relativgeschwindigkeit(panel.punkt_welt, cg_welt,
            velocity, omega_welt, config.fluidgeschwindigkeit_welt_m_s);
        const double normal_speed = dot(relative, panel.normale_welt);
        const auto tangent = plus(relative, mal(panel.normale_welt, -normal_speed));
        const double speed = std::sqrt(dot(tangent, tangent));
        if (speed <= 1e-12) continue;
        const double re = speed * std::sqrt(panel.flaeche_m2) / config.nu_m2_s;
        if (!std::isfinite(re) || re > 1e9) {
            result.zustand = Datenzustand::nicht_berechnet;
            result.fehler_id = "CORE-HYD-0001";
            return result;
        }
        const double cf = reibungsbeiwert(re);
        min_re = std::min(min_re, re);
        max_re = std::max(max_re, re);
        min_cf = std::min(min_cf, cf);
        max_cf = std::max(max_cf, cf);
        const auto force_welt = mal(tangent, .5 * config.rho_kg_m3 * cf * panel.flaeche_m2 * speed);
        const auto point_model = pose.weltpunkt_nach_modell(panel.punkt_welt);
        const auto force_model = pose.weltvektor_nach_modell(force_welt);
        total_force = plus(total_force, force_welt);
        total_moment = plus(total_moment, moment_um(point_model, modell.schwerpunkt_m, force_model));
        application_point = plus(application_point, mal(point_model, std::sqrt(dot(force_welt, force_welt))));
        point_weight += std::sqrt(dot(force_welt, force_welt));
    }
    if (point_weight <= 0.) {
        result.zustand = Datenzustand::null;
        return result;
    }
    result.punkt_m = mal(application_point, 1. / point_weight);
    result.vektor = pose.weltvektor_nach_modell(total_force);
    result.moment_s_nm = total_moment;
    verwendeten_parameter(result, "reynolds_zahl_min", min_re, "1", "abgeleitet",
        "Aus lokaler Panelanstroemung berechnet", "0 < Re <= 1e9",
        "Schaetzung der Panel-Langenskala durch Quadratwurzel der Panel-Flaeche");
    verwendeten_parameter(result, "reynolds_zahl_max", max_re, "1", "abgeleitet",
        "Aus lokaler Panelanstroemung berechnet", "0 < Re <= 1e9",
        "Schaetzung der Panel-Langenskala durch Quadratwurzel der Panel-Flaeche");
    verwendeten_parameter(result, "reibungsbeiwert_min", min_cf, "1", "abgeleitet",
        "ITTC-1957 plus laminare Plattennaeherung", "0 < Re <= 1e9",
        "Nicht fuer Rumpfrauheit oder lokale Ablösung kalibriert");
    verwendeten_parameter(result, "reibungsbeiwert_max", max_cf, "1", "abgeleitet",
        "ITTC-1957 plus laminare Plattennaeherung", "0 < Re <= 1e9",
        "Nicht fuer Rumpfrauheit oder lokale Ablösung kalibriert");
    result.zustand = Datenzustand::gueltig;
    return result;
}

Beitrag druckbeitrag(const Element& element, const Modell& modell, const Pose& pose,
    const HydrodynamikKonfiguration& config, const Vektor3& velocity,
    const Vektor3& omega, const std::vector<Nasspanel>& panels)
{
    const auto* model = kraftmodell(element, "druckwiderstand");
    const auto method = model ? model->methode : "geschaetzter_normaler_formwiderstand";
    if (method != "geschaetzter_normaler_formwiderstand" && method != "platten_normal_drag")
        throw std::invalid_argument("CORE-HYD-0001: Unbekannte Druckwiderstands-Methode '" + method + "'");
    const double cd = parameterwert(model, "cd", 1.0);
    if (!std::isfinite(cd) || cd < 0.)
        throw std::invalid_argument("CORE-HYD-0001: Druckwiderstands-Cd muss endlich und nichtnegativ sein.");
    auto result = basisbeitrag(element, "druck");
    provenance(result, model, method,
        "Geschaetzt: Cd=1.0 fuer den lokalen stumpfen Normalanteil",
        "Richtungsabhaengige Panelnaeherung; Cd ist nicht fuer die konkrete Rumpfform kalibriert");
    verwendeten_parameter(result, "cd", cd, "1",
        model && model->parameter.count("cd") ? model->parameter.at("cd").status : "geschaetzt",
        model && model->parameter.count("cd") ? model->parameter.at("cd").quelle :
            "Annahme: Cd=1.0 fuer stumpfen Normalanteil",
        model && model->parameter.count("cd") ? model->parameter.at("cd").gueltigkeit :
            "Untergetauchte Rumpfpanels ohne Wellenwiderstand",
        model && model->parameter.count("cd") ? model->parameter.at("cd").unsicherheit :
            "Nicht fuer die konkrete Rumpfform kalibriert");
    if (panels.empty() || cd == 0.) {
        result.zustand = Datenzustand::null;
        return result;
    }
    const auto omega_welt = pose.modellvektor_nach_welt(omega);
    const auto cg_welt = pose.modellpunkt_nach_welt(modell.schwerpunkt_m);
    Vektor3 total_force{}, total_moment{}, application_point{};
    double point_weight = 0.;
    for (const auto& panel : panels) {
        const auto relative = lokale_relativgeschwindigkeit(panel.punkt_welt, cg_welt,
            velocity, omega_welt, config.fluidgeschwindigkeit_welt_m_s);
        const double normal_speed = dot(relative, panel.normale_welt);
        if (normal_speed >= 0.) continue;
        const auto force_welt = mal(panel.normale_welt,
            .5 * config.rho_kg_m3 * cd * panel.flaeche_m2 * normal_speed * std::abs(normal_speed));
        const auto point_model = pose.weltpunkt_nach_modell(panel.punkt_welt);
        const auto force_model = pose.weltvektor_nach_modell(force_welt);
        const double magnitude = std::sqrt(dot(force_welt, force_welt));
        total_force = plus(total_force, force_welt);
        total_moment = plus(total_moment, moment_um(point_model, modell.schwerpunkt_m, force_model));
        application_point = plus(application_point, mal(point_model, magnitude));
        point_weight += magnitude;
    }
    if (point_weight <= 0.) {
        result.zustand = Datenzustand::null;
        return result;
    }
    result.punkt_m = mal(application_point, 1. / point_weight);
    result.vektor = pose.weltvektor_nach_modell(total_force);
    result.moment_s_nm = total_moment;
    result.zustand = Datenzustand::gueltig;
    return result;
}

Beitrag seitenkraftbeitrag(const Element& element, const Modell& modell, const Pose& pose,
    const HydrodynamikKonfiguration& config, const Vektor3& velocity,
    const Vektor3& omega, const std::vector<Nasspanel>& panels,
    const std::vector<Bewegungsstellung>& bewegungsstellungen)
{
    const auto* model = kraftmodell(element, "seitenkraft");
    const auto method = model ? model->methode : "geschaetzte_endliche_streckungspolare";
    if (method != "geschaetzte_endliche_streckungspolare" && method != "polare_2d")
        throw std::invalid_argument("CORE-HYD-0001: Unbekannte Seitenkraft-Methode '" + method + "'");
    if (model && method == "polare_2d" && model->kennfeld.empty())
        throw std::invalid_argument("CORE-HYD-0001: polare_2d benoetigt ein Kennfeld.");
    if (model && !model->kennfeld.empty() && model->widerstandsanteile.empty())
        throw std::invalid_argument("CORE-HYD-0001: Kennfeld benoetigt explizite Widerstandsanteile.");
    auto result = basisbeitrag(element, "seitenkraft");
    provenance(result, model, method,
        "Geschaetzt: endliche Fluegelstreckung nach a=2*pi*AR/(AR+2)",
        "Lineares Auftriebsmodell mit begrenztem Cl; Profil-, Abloese- und Rumpfinteraktionen nicht modelliert");
    if (panels.empty()) {
        result.zustand = Datenzustand::null;
        return result;
    }
    const auto omega_welt = pose.modellvektor_nach_welt(omega);
    const auto cg_welt = pose.modellpunkt_nach_welt(modell.schwerpunkt_m);
    double low_x = std::numeric_limits<double>::infinity();
    double high_x = -low_x, low_y = low_x, high_y = -low_x, low_z = low_x, high_z = -low_x;
    // Profilabmessungen stets am unverstellten Mesh bestimmen, sonst wird ein
    // gedrehtes Ruder als dicker Koerper und damit als Nicht-Tragflaeche eingestuft.
    const Mesh* reference_mesh = element.volumen ? &*element.volumen : nullptr;
    for (const auto& candidate : modell.elemente)
        if (candidate.id == element.id && candidate.volumen) reference_mesh = &*candidate.volumen;
    if (!reference_mesh)
        throw std::invalid_argument("CORE-HYD-0001: Seitenkraft ohne physisches Volumenmesh: " + element.id);
    for (const auto& point : reference_mesh->punkte) {
        low_x = std::min(low_x, point[0]); high_x = std::max(high_x, point[0]);
        low_y = std::min(low_y, point[1]); high_y = std::max(high_y, point[1]);
        low_z = std::min(low_z, point[2]); high_z = std::max(high_z, point[2]);
    }
    const double span = high_z - low_z;
    const double thickness_ratio = (high_y - low_y) / std::max(high_x - low_x, high_z - low_z);
    auto chord_axis = Vektor3{1., 0., 0.};
    auto span_axis = Vektor3{0., 0., 1.};
    if (const auto* owner = bewegungseigentuemer(modell, element.id)) {
        const double setting = bewegungswert(*owner, bewegungsstellungen);
        chord_axis = einheitsvektor(bewegungsvektor(chord_axis, *owner, setting));
        span_axis = einheitsvektor(bewegungsvektor(span_axis, *owner, setting));
    }
    const auto lift_axis_model = einheitsvektor(cross(span_axis, chord_axis));
    const auto lift_axis_world = pose.modellvektor_nach_welt(lift_axis_model);
    double planform_area = 0.;
    for (const auto& panel : panels) {
        const auto normal_model = pose.weltvektor_nach_modell(panel.normale_welt);
        planform_area += .5 * panel.flaeche_m2 * std::abs(dot(normal_model, lift_axis_model));
    }
    if (planform_area <= 1e-12 || span <= 1e-12 || thickness_ratio > .35) {
        result.zustand = Datenzustand::null;
        result.modell_methode += ":geometrisch_als_nicht_tragflaeche_eingestuft";
        return result;
    }
    const double aspect_ratio = parameterwert(model, "streckung", span * span / planform_area);
    const double efficiency = parameterwert(model, "wirkungsgrad", .8);
    const double cd0 = parameterwert(model, "cd0", .02);
    const double cl_max = parameterwert(model, "cl_max", 1.0);
    if (!std::isfinite(aspect_ratio) || aspect_ratio <= 0. ||
        !std::isfinite(efficiency) || efficiency <= 0. || efficiency > 1. ||
        !std::isfinite(cd0) || cd0 < 0. || !std::isfinite(cl_max) || cl_max <= 0.)
        throw std::invalid_argument("CORE-HYD-0001: Ungueltige Seitenkraftparameter.");
    verwendeten_parameter(result, "streckung", aspect_ratio, "1",
        model && model->parameter.count("streckung") ?
            model->parameter.at("streckung").status : "abgeleitet",
        model && model->parameter.count("streckung") ?
            model->parameter.at("streckung").quelle : "Aus physischem Mesh und benetzter Planform abgeleitet",
        model && model->parameter.count("streckung") ?
            model->parameter.at("streckung").gueltigkeit : "Geometrisch als Tragflaeche eingestufte Kiel-/Rudermeshes",
        model && model->parameter.count("streckung") ?
            model->parameter.at("streckung").unsicherheit : "Meshauflösung und benetzte Kontur beeinflussen den Wert");
    verwendeten_parameter(result, "wirkungsgrad", efficiency, "1",
        model && model->parameter.count("wirkungsgrad") ?
            model->parameter.at("wirkungsgrad").status : "geschaetzt",
        model && model->parameter.count("wirkungsgrad") ?
            model->parameter.at("wirkungsgrad").quelle : "Annahme: Oswald-Wirkungsgrad 0.8",
        model && model->parameter.count("wirkungsgrad") ?
            model->parameter.at("wirkungsgrad").gueltigkeit : "Endliche Streckung; keine Interaktion",
        model && model->parameter.count("wirkungsgrad") ?
            model->parameter.at("wirkungsgrad").unsicherheit : "Nicht fuer konkrete Profile kalibriert");
    verwendeten_parameter(result, "cd0", cd0, "1",
        model && model->parameter.count("cd0") ?
            model->parameter.at("cd0").status : "geschaetzt",
        model && model->parameter.count("cd0") ?
            model->parameter.at("cd0").quelle : "Annahme: Nullauftriebswiderstand 0.02",
        model && model->parameter.count("cd0") ?
            model->parameter.at("cd0").gueltigkeit : "Geschaetztes Profilmodell",
        model && model->parameter.count("cd0") ?
            model->parameter.at("cd0").unsicherheit : "Nicht fuer konkrete Profile kalibriert");
    verwendeten_parameter(result, "cl_max", cl_max, "1",
        model && model->parameter.count("cl_max") ?
            model->parameter.at("cl_max").status : "geschaetzt",
        model && model->parameter.count("cl_max") ?
            model->parameter.at("cl_max").quelle : "Annahme: begrenzter Auftriebsbeiwert 1.0",
        model && model->parameter.count("cl_max") ?
            model->parameter.at("cl_max").gueltigkeit : "Keine explizite Ablösepolare vorhanden",
        model && model->parameter.count("cl_max") ?
            model->parameter.at("cl_max").unsicherheit : "Nicht fuer konkrete Profile kalibriert");
    const double lift_slope = 2. * std::acos(-1.) * aspect_ratio / (aspect_ratio + 2.);
    Vektor3 total_force{}, total_moment{}, application_point{};
    double point_weight = 0.;
    for (const auto& panel : panels) {
        const auto relative = lokale_relativgeschwindigkeit(panel.punkt_welt, cg_welt,
            velocity, omega_welt, config.fluidgeschwindigkeit_welt_m_s);
        const auto relative_model = pose.weltvektor_nach_modell(relative);
        const double speed = std::sqrt(dot(relative, relative));
        if (speed <= 1e-12) continue;
        const double normal_speed = dot(relative_model, lift_axis_model);
        const double chord_speed = dot(relative_model, chord_axis);
        const double alpha = std::atan2(std::abs(normal_speed),
            std::max(std::abs(chord_speed), 1e-12));
        const double cl = std::min(cl_max, lift_slope * alpha);
        const double reynolds = speed * std::max(high_x - low_x, high_z - low_z) / config.nu_m2_s;
        double cd = cd0;
        if (model && !model->kennfeld.empty()) {
            const double positive_alpha = alpha;
            std::vector<double> alpha_axis, reynolds_axis;
            for (const auto& point : model->kennfeld) {
                alpha_axis.push_back(point.anstellwinkel_rad);
                reynolds_axis.push_back(point.reynolds_zahl);
            }
            std::sort(alpha_axis.begin(), alpha_axis.end());
            alpha_axis.erase(std::unique(alpha_axis.begin(), alpha_axis.end()), alpha_axis.end());
            std::sort(reynolds_axis.begin(), reynolds_axis.end());
            reynolds_axis.erase(std::unique(reynolds_axis.begin(), reynolds_axis.end()), reynolds_axis.end());
            if (positive_alpha < alpha_axis.front() || positive_alpha > alpha_axis.back() ||
                reynolds < reynolds_axis.front() || reynolds > reynolds_axis.back()) {
                result.zustand = Datenzustand::nicht_berechnet;
                result.fehler_id = "CORE-HYD-0001";
                return result;
            }
            auto bracket = [](const std::vector<double>& axis, double value) {
                auto upper = std::lower_bound(axis.begin(), axis.end(), value);
                if (upper == axis.begin()) return std::pair<std::size_t, std::size_t>{0, 0};
                if (upper == axis.end()) return std::pair<std::size_t, std::size_t>{axis.size()-1, axis.size()-1};
                const auto hi = static_cast<std::size_t>(upper-axis.begin());
                if (*upper == value) return std::pair<std::size_t, std::size_t>{hi, hi};
                return std::pair<std::size_t, std::size_t>{hi-1, hi};
            };
            const auto [a0,a1] = bracket(alpha_axis, positive_alpha);
            const auto [r0,r1] = bracket(reynolds_axis, reynolds);
            auto value_at = [&](double a, double r, bool lift) {
                const auto found = std::find_if(model->kennfeld.begin(), model->kennfeld.end(),
                    [&](const auto& point) {return point.anstellwinkel_rad == a && point.reynolds_zahl == r;});
                if (found == model->kennfeld.end())
                    throw std::invalid_argument("CORE-HYD-0001: Kennfeld ist kein vollstaendiges Rechteck.");
                return lift ? found->cl : found->cd;
            };
            auto interp = [&](bool lift) {
                const double va0r0 = value_at(alpha_axis[a0], reynolds_axis[r0], lift);
                const double va1r0 = value_at(alpha_axis[a1], reynolds_axis[r0], lift);
                const double va0r1 = value_at(alpha_axis[a0], reynolds_axis[r1], lift);
                const double va1r1 = value_at(alpha_axis[a1], reynolds_axis[r1], lift);
                const double ta = a0 == a1 ? 0. : (positive_alpha-alpha_axis[a0])/(alpha_axis[a1]-alpha_axis[a0]);
                const double tr = r0 == r1 ? 0. : (reynolds-reynolds_axis[r0])/(reynolds_axis[r1]-reynolds_axis[r0]);
                const double low = va0r0*(1.-ta)+va1r0*ta;
                const double high = va0r1*(1.-ta)+va1r1*ta;
                return low*(1.-tr)+high*tr;
            };
            const double polar_cl = interp(true);
            cd = interp(false);
            if (polar_cl < 0. || cd < 0.)
                throw std::invalid_argument("CORE-HYD-0001: Kennfeld-Cl und Cd muessen fuer den positiven Anstellwinkel nichtnegativ sein.");
            const double induced_cd = hat_widerstandsanteil(model, "gesamt") ||
                hat_widerstandsanteil(model, "induzierter") ? 0. :
                polar_cl * polar_cl / (std::acos(-1.) * efficiency * aspect_ratio);
            cd += induced_cd;
            verwendeten_parameter(result, "kennfeld_cl", polar_cl, "1",
                parameterstatus(model), model->quelle, model->gueltigkeit,
                result.modell_unsicherheit);
            verwendeten_parameter(result, "kennfeld_cd", cd, "1",
                parameterstatus(model), model->quelle, model->gueltigkeit,
                result.modell_unsicherheit);
            const double side_sign = normal_speed < 0. ? -1. : 1.;
            const double kennfeld_area = .5 * panel.flaeche_m2 *
                std::abs(dot(pose.weltvektor_nach_modell(panel.normale_welt), lift_axis_model));
            const double lift = .5 * config.rho_kg_m3 * speed * speed *
                kennfeld_area * polar_cl * side_sign;
            const double drag = .5 * config.rho_kg_m3 * speed * speed *
                kennfeld_area * cd;
            const auto force_welt = plus(mal(lift_axis_world, lift),
                mal(relative, drag / speed));
            const auto point_model = pose.weltpunkt_nach_modell(panel.punkt_welt);
            const auto force_model = pose.weltvektor_nach_modell(force_welt);
            const double magnitude = std::sqrt(dot(force_welt, force_welt));
            total_force = plus(total_force, force_welt);
            total_moment = plus(total_moment, moment_um(point_model, modell.schwerpunkt_m, force_model));
            application_point = plus(application_point, mal(point_model, magnitude));
            point_weight += magnitude;
            continue;
        }
        const double side_sign = normal_speed < 0. ? -1. : 1.;
        const double projected_area = .5 * panel.flaeche_m2 *
            std::abs(dot(pose.weltvektor_nach_modell(panel.normale_welt), lift_axis_model));
        const double dynamic = .5 * config.rho_kg_m3 * speed * speed * projected_area;
        const double lift_attached = cl;
        const double cd_attached = cd + cl * cl / (std::acos(-1.) * efficiency * aspect_ratio);
        // Oberhalb des Ablösewinkels geht die Kraft in die Normalkraft einer
        // angestroemten Platte (cn = cn_platte*sin(alpha)) ueber.
        const double alpha_stall = std::min(cl_max / lift_slope, std::acos(-1.) / 4.);
        const double blend_width = 10. * std::acos(-1.) / 180.;
        const double t = std::clamp((alpha - alpha_stall) / blend_width, 0., 1.);
        const double weight = t * t * (3. - 2. * t);
        const double cn_plate = 1.1 + .018 * std::min(aspect_ratio, 10.);
        const double cl_plate = cn_plate * std::sin(alpha) * std::cos(alpha);
        const double cd_plate = cd + cn_plate * std::sin(alpha) * std::sin(alpha);
        const double cl_total = lift_attached * (1. - weight) + cl_plate * weight;
        const double cd_total = cd_attached * (1. - weight) + cd_plate * weight;
        const double lift = dynamic * cl_total * side_sign;
        const double drag = dynamic * cd_total;
        const auto force_welt = plus(mal(lift_axis_world, lift), mal(relative, drag / speed));
        const auto point_model = pose.weltpunkt_nach_modell(panel.punkt_welt);
        const auto force_model = pose.weltvektor_nach_modell(force_welt);
        const double magnitude = std::sqrt(dot(force_welt, force_welt));
        total_force = plus(total_force, force_welt);
        total_moment = plus(total_moment, moment_um(point_model, modell.schwerpunkt_m, force_model));
        application_point = plus(application_point, mal(point_model, magnitude));
        point_weight += magnitude;
    }
    if (point_weight <= 0.) {
        result.zustand = Datenzustand::null;
        return result;
    }
    result.punkt_m = mal(application_point, 1. / point_weight);
    result.vektor = pose.weltvektor_nach_modell(total_force);
    result.moment_s_nm = total_moment;
    result.zustand = Datenzustand::gueltig;
    return result;
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
    case Quelle::hydrodynamik: return "hydrodynamik";
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

Bilanz bilanz(const Modell& modell,std::vector<Beitrag>& beitraege,bool hydrodynamik_aktiv)
{
    Bilanz result;
    result.gesamt.gruppe="gesamt";
    if(std::any_of(beitraege.begin(),beitraege.end(),
        [](const Beitrag& b){return b.quelle==Quelle::testantrieb;}))
        result.erwartete_beitraege.push_back("testantrieb.bug.kraft");
    for (const auto& e : modell.elemente) {
        result.erwartete_beitraege.push_back(e.id+":gewicht");
        if (e.volumen) result.erwartete_beitraege.push_back(e.id+":auftrieb");
        if (hydrodynamik_aktiv && e.hydrodynamik_rolle) {
            result.erwartete_beitraege.push_back(e.id+":hydrodynamik:reibung");
            if (*e.hydrodynamik_rolle == HydrodynamikRolle::rumpf)
                result.erwartete_beitraege.push_back(e.id+":hydrodynamik:druck");
            else
                result.erwartete_beitraege.push_back(e.id+":hydrodynamik:seitenkraft");
        }
    }
    if(hydrodynamik_aktiv &&
        std::any_of(beitraege.begin(),beitraege.end(),[](const Beitrag& b){return b.id.rfind("daempfung:",0)==0;}))
        for(const char* id:{"daempfung:rollen","daempfung:stampfen","daempfung:gieren","daempfung:tauchen"})
            result.erwartete_beitraege.push_back(id);
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
void hydrodynamikparameter(const HydrodynamikKonfiguration& c)
{
    if (!std::isfinite(c.rho_kg_m3) || c.rho_kg_m3 <= 0.)
        throw std::invalid_argument("CORE-HYD-0001: rho_kg_m3 muss endlich und positiv sein.");
    if (!std::isfinite(c.nu_m2_s) || c.nu_m2_s <= 0.)
        throw std::invalid_argument("CORE-HYD-0001: nu_m2_s muss endlich und positiv sein.");
    if (!endlich(c.fluidgeschwindigkeit_welt_m_s))
        throw std::invalid_argument("CORE-HYD-0001: Fluidgeschwindigkeit muss endlich sein.");
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
    if(s.hydrodynamik_erforderlich&&!s.hydrodynamik)
        throw std::invalid_argument("CORE-HYD-0001: Erforderlicher Hydrodynamikkontext fehlt.");
    if(s.hydrodynamik && (s.hydrodynamik->modell_fassung!=m.fassung ||
        s.hydrodynamik->auswertung_schritt!=s.schritt ||
        s.hydrodynamik->auswertung_zeit_s!=s.zeit_s))
        throw std::invalid_argument("CORE-HYD-0001: Hydrodynamikkontext passt nicht zum Zustand oder Modell.");
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

Zustand hydrodynamik_start(const Modell& m,const Zustand& s,
    const HydrodynamikKonfiguration& config)
{
    hydrodynamikparameter(config);
    kontextpruefung(m,s);
    if(s.hydrodynamik || s.hydrodynamik_erforderlich || !std::isfinite(s.zeit_s))
        throw std::invalid_argument("CORE-HYD-0001: Hydrodynamikstart benoetigt einen gueltigen Zustand ohne Hydrokontext.");
    if(std::none_of(m.elemente.begin(),m.elemente.end(),[](const Element& e) {
        return e.hydrodynamik_rolle.has_value();
    }))
        throw std::invalid_argument("CORE-HYD-0001: Modell enthaelt kein Hydrodynamik-Element.");
    auto context=std::make_shared<HydrodynamikKontext>();
    context->konfiguration=config;
    context->modell_fassung=m.fassung;
    context->auswertung_schritt=s.schritt;
    context->auswertung_zeit_s=s.zeit_s;
    if(config.zusatzmasse_daempfung)
        context->traegheit=std::make_shared<const HydrodynamischeTraegheit>(
            berechne_hydrodynamische_traegheit(m,s.pose,config.rho_kg_m3,9.81));
    kraefte(m,s.pose,s.testantrieb.get(),1000.,9.81,s.geflutet,context.get(),
        s.geschwindigkeit_welt_m_s,s.omega_modell_rad_s);
    auto result=s;
    result.hydrodynamik=context;
    result.hydrodynamik_erforderlich=true;
    return result;
}

Kraefte kraefte(const Modell& m,const Pose& p,const TestantriebKontext* c,
    double rho,double g,const std::vector<std::string>& geflutet,
    const HydrodynamikKontext* hydro,const Vektor3& geschwindigkeit,
    const Vektor3& omega,const std::vector<Bewegungsstellung>& bewegungsstellungen)
{
    auto k=hydrostatik(m,p,rho,g,geflutet);
    if(c) {
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
        k.beitraege.push_back(b);
    }
    if(hydro) {
        hydrodynamikparameter(hydro->konfiguration);
        if(hydro->modell_fassung!=m.fassung)
            throw std::invalid_argument("CORE-HYD-0001: Falsche Hydrodynamik-Modellfassung.");
        if(!endlich(geschwindigkeit) || !endlich(omega))
            throw std::invalid_argument("CORE-HYD-0001: Lineare und Winkelgeschwindigkeit muessen endlich sein.");
        for(const auto& e:m.elemente) {
            if(!e.hydrodynamik_rolle) continue;
            if(!e.volumen)
                throw std::invalid_argument("CORE-HYD-0001: Hydrodynamikrolle ohne physisches Volumenmesh: " + e.id);
            Element hydrodynamik_element=e;
            if(const auto* eigentuemer=bewegungseigentuemer(m,e.id)) {
                hydrodynamik_element.volumen=bewegtes_mesh(*e.volumen,*eigentuemer,
                    bewegungswert(*eigentuemer,bewegungsstellungen));
            }
            const auto panels=nasspanels(*hydrodynamik_element.volumen,p);
            auto add=[&](Beitrag b) {
                if(b.zustand==Datenzustand::gueltig) {
                    k.kraft_welt_n=plus(k.kraft_welt_n,p.modellvektor_nach_welt(b.vektor));
                    k.moment_modell_nm=plus(k.moment_modell_nm,b.moment_s_nm);
                }
                k.beitraege.push_back(std::move(b));
            };
            add(reibungsbeitrag(hydrodynamik_element,m,p,hydro->konfiguration,geschwindigkeit,omega,panels));
            if(*e.hydrodynamik_rolle==HydrodynamikRolle::rumpf)
                add(druckbeitrag(hydrodynamik_element,m,p,hydro->konfiguration,geschwindigkeit,omega,panels));
            else
                add(seitenkraftbeitrag(hydrodynamik_element,m,p,hydro->konfiguration,
                    geschwindigkeit,omega,panels,bewegungsstellungen));
        }
        if(const auto* ht=hydro->traegheit.get()) {
            auto moment=[&](const char* id,const Vektor3& wert) {
                Beitrag b;b.id=id;b.art="moment";b.quelle=Quelle::hydrodynamik;
                b.gruppe="hydrodynamik";b.punktart=Punktart::bezugspunkt;
                b.punkt_m=m.schwerpunkt_m;b.vektor=wert;b.moment_s_nm=wert;
                b.modell_status="geschaetzt";b.modell_methode="lineare/quadratische Daempfung";
                k.moment_modell_nm=plus(k.moment_modell_nm,wert);
                k.beitraege.push_back(std::move(b));
            };
            const double izz=m.traegheit_kg_m2[8]+ht->zusatzmasse[5].wert;
            const double b1_gier=ht->gieren_linear.wert*izz;
            const double b2_gier=ht->gieren_quadratisch.wert;
            moment("daempfung:rollen",{-ht->b_rollen_nms*omega[0],0.,0.});
            moment("daempfung:stampfen",{0.,-ht->b_stampfen_nms*omega[1],0.});
            moment("daempfung:gieren",{0.,0.,-(b1_gier+b2_gier*std::abs(omega[2]))*omega[2]});
            Beitrag b;b.id="daempfung:tauchen";b.art="kraft";b.quelle=Quelle::hydrodynamik;
            b.gruppe="hydrodynamik";b.punktart=Punktart::angriffspunkt;b.punkt_m=m.schwerpunkt_m;
            const Vektor3 f{0.,0.,-ht->b_tauchen_ns_m*geschwindigkeit[2]};
            b.vektor=p.weltvektor_nach_modell(f);
            b.modell_status="geschaetzt";b.modell_methode="lineare Daempfung";
            k.kraft_welt_n=plus(k.kraft_welt_n,f);
            k.beitraege.push_back(std::move(b));
        }
    }
    for(auto& sum:k.beitraege) if(sum.rolle==Rolle::gesamtsumme) {
        sum.vektor=k.moment_modell_nm;sum.moment_s_nm=k.moment_modell_nm;
    }
    return k;
}

Lateraldruckpunkt lateraldruckpunkt(const Modell& m,const Pose& p,const Kraefte& k,
    const std::vector<Bewegungsstellung>& bewegungsstellungen)
{
    Lateraldruckpunkt result;
    result.vorhanden=std::any_of(m.elemente.begin(),m.elemente.end(),[](const Element& e) {
        return e.hydrodynamik_rolle.has_value()&&e.volumen.has_value();
    });
    if(!result.vorhanden) return result;
    auto endet_mit=[](const std::string& text,const std::string& ende) {
        return text.size()>=ende.size()&&text.compare(text.size()-ende.size(),ende.size(),ende)==0;
    };
    double fy=0.,fy_betrag=0.,fy_x=0.,fy_z=0.;
    std::vector<std::string> quellen;
    for(const auto& b:k.beitraege) {
        if(b.quelle!=Quelle::hydrodynamik||b.zustand!=Datenzustand::gueltig||b.art!="kraft"||!b.punkt_bekannt)
            continue;
        if(!endet_mit(b.id,":hydrodynamik:druck")&&!endet_mit(b.id,":hydrodynamik:seitenkraft")) continue;
        const double f=b.vektor[1];
        if(!std::isfinite(f)||f==0.) continue;
        fy+=f;fy_betrag+=std::abs(f);fy_x+=f*b.punkt_m[0];fy_z+=f*b.punkt_m[2];
        quellen.push_back(b.id);
    }
    result.querkraft_modell_n=fy;
    if(std::abs(fy)>=1.&&std::abs(fy)>=.05*fy_betrag) {
        result.status="gueltig";
        result.position_modell_m=Vektor3{fy_x/fy,0.,fy_z/fy};
        result.quellen=std::move(quellen);
        return result;
    }
    double flaeche=0.,fx=0.,fz=0.;
    for(const auto& e:m.elemente) {
        if(!e.hydrodynamik_rolle||!e.volumen) continue;
        Mesh mesh=*e.volumen;
        if(const auto* eigentuemer=bewegungseigentuemer(m,e.id))
            mesh=bewegtes_mesh(mesh,*eigentuemer,bewegungswert(*eigentuemer,bewegungsstellungen));
        for(const auto& panel:nasspanels(mesh,p)) {
            const double lateral=panel.flaeche_m2*std::abs(p.weltvektor_nach_modell(panel.normale_welt)[1]);
            const auto punkt=p.weltpunkt_nach_modell(panel.punkt_welt);
            flaeche+=lateral;fx+=lateral*punkt[0];fz+=lateral*punkt[2];
        }
    }
    result.lateralflaeche_m2=flaeche;
    if(flaeche>1e-9&&std::isfinite(fx)&&std::isfinite(fz)) {
        result.status="geometrisch";
        result.position_modell_m=Vektor3{fx/flaeche,0.,fz/flaeche};
    }
    return result;
}

std::array<double,16> bewegungstransformation(const Modell& modell,
    const std::string& element_id,const std::vector<Bewegungsstellung>& stellungen)
{
    const auto* eigentuemer=bewegungseigentuemer(modell,element_id);
    if(!eigentuemer) return {1.,0.,0.,0.,0.,1.,0.,0.,0.,0.,1.,0.,0.,0.,0.,1.};
    const double wert=bewegungswert(*eigentuemer,stellungen);
    const Vektor3 ursprung=bewegungspunkt({},*eigentuemer,wert);
    const auto x=plus(bewegungspunkt({1.,0.,0.},*eigentuemer,wert),mal(ursprung,-1.));
    const auto y=plus(bewegungspunkt({0.,1.,0.},*eigentuemer,wert),mal(ursprung,-1.));
    const auto z=plus(bewegungspunkt({0.,0.,1.},*eigentuemer,wert),mal(ursprung,-1.));
    return {x[0],y[0],z[0],ursprung[0],
        x[1],y[1],z[1],ursprung[1],
        x[2],y[2],z[2],ursprung[2],
        0.,0.,0.,1.};
}

Zustand dynamikschritt(const Modell& modell,const Zustand& s,double dt,double rho,double g)
{
    return dynamikschritt(modell,s,dt,std::vector<TestantriebEreignis>{},rho,g);
}
Zustand dynamikschritt(const Modell& modell,const Zustand& s,double dt,
    const std::vector<TestantriebEreignis>& ereignisse,double rho,double g)
{
    return dynamikschritt(modell,s,dt,ereignisse,std::vector<Bewegungsereignis>{},rho,g);
}
Zustand dynamikschritt(const Modell& modell,const Zustand& s,double dt,
    const std::vector<TestantriebEreignis>& ereignisse,
    const std::vector<Bewegungsereignis>& bewegungsereignisse,double rho,double g)
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
    auto bewegungsstellungen=s.bewegungsstellungen;
    for (const auto& element : modell.elemente) {
        if (!element.bewegung) continue;
        const auto fund=std::find_if(bewegungsstellungen.begin(),bewegungsstellungen.end(),
            [&](const Bewegungsstellung& stellung) { return stellung.element_id==element.id; });
        if (fund==bewegungsstellungen.end())
            bewegungsstellungen.push_back({element.id,
                element.bewegung->ruderwinkel?0.:100.});
    }
    auto wert_pruefen=[&](const std::string& id,double wert) {
        const auto fund=std::find_if(modell.elemente.begin(),modell.elemente.end(),
            [&](const Element& element) { return element.id==id; });
        if(fund==modell.elemente.end() || !fund->bewegung)
            throw std::invalid_argument("CORE-MOD-0002: Element ohne Bewegungsdefinition: "+id);
        const double minimum=fund->bewegung->ruderwinkel?-90.:0.;
        const double maximum=fund->bewegung->ruderwinkel?90.:100.;
        if(!std::isfinite(wert)||wert<minimum||wert>maximum)
            throw std::invalid_argument("CORE-MOD-0002: Bewegungsstellung ausserhalb des zulaessigen Bereichs: "+id);
    };
    for(const auto& stellung:bewegungsstellungen) wert_pruefen(stellung.element_id,stellung.wert);
    auto letzte_bewegungskennung=s.letzte_bewegungsereigniskennung;
    for(const auto& ereignis:bewegungsereignisse) {
        if(ereignis.kennung<=letzte_bewegungskennung)
            throw std::invalid_argument("CORE-SIM-0001: Bewegungsereigniskennungen nicht streng aufsteigend.");
        letzte_bewegungskennung=ereignis.kennung;
        wert_pruefen(ereignis.element_id,ereignis.wert);
        const auto fund=std::find_if(bewegungsstellungen.begin(),bewegungsstellungen.end(),
            [&](const Bewegungsstellung& stellung) { return stellung.element_id==ereignis.element_id; });
        if(fund==bewegungsstellungen.end())
            throw std::invalid_argument("CORE-MOD-0002: Fehlende Bewegungsstellung: "+ereignis.element_id);
        fund->wert=ereignis.wert;
    }
    std::shared_ptr<HydrodynamikKontext> hydro_context;
    if(s.hydrodynamik)
        hydro_context=std::make_shared<HydrodynamikKontext>(*s.hydrodynamik);
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
    const HydrodynamischeTraegheit* ht=hydro_context?hydro_context->traegheit.get():nullptr;
    std::array<double,9> ieff=I;
    Vektor3 mlin{modell.masse_kg,modell.masse_kg,modell.masse_kg};
    if(ht) {
        for(std::size_t i=0;i<3;++i) {
            ieff[4*i]+=ht->zusatzmasse[3+i].wert;
            mlin[i]+=ht->zusatzmasse[i].wert;
        }
        if (ieff[0]<=0. || ieff[0]*ieff[4]-ieff[1]*ieff[3]<=0. ||
            ieff[0]*(ieff[4]*ieff[8]-ieff[5]*ieff[7])-ieff[1]*(ieff[3]*ieff[8]-ieff[5]*ieff[6])+
            ieff[2]*(ieff[3]*ieff[7]-ieff[4]*ieff[6])<=0.)
            throw std::invalid_argument("CORE-SIM-0002: Effektive Traegheit nicht positiv definit.");
    }
    struct Beschleunigung { Vektor3 linear_welt; Vektor3 winkel; };
    // Kirchhoff-Gleichungen im Koerperrahmen; ohne Zusatzmasse identisch zur Gleichung ohne Zusatzmasse.
    auto beschleunigung=[&](const Pose& lage,const Kraefte& kr,const Vektor3& v_welt,
        const Vektor3& om) {
        Beschleunigung r;
        if(!ht) {
            r.linear_welt=mal(kr.kraft_welt_n,1./modell.masse_kg);
            Vektor3 imp0{};
            for (std::size_t i=0;i<3;++i)
                for (std::size_t j=0;j<3;++j) imp0[i]+=I[3*i+j]*om[j];
            r.winkel=loesen(I,plus(kr.moment_modell_nm,mal(cross(om,imp0),-1.)));
            return r;
        }
        const auto vb=lage.weltvektor_nach_modell(v_welt);
        const auto fb=lage.weltvektor_nach_modell(kr.kraft_welt_n);
        const Vektor3 pl{mlin[0]*vb[0],mlin[1]*vb[1],mlin[2]*vb[2]};
        const auto gyro=cross(om,pl);
        Vektor3 dvb{(fb[0]-gyro[0])/mlin[0],(fb[1]-gyro[1])/mlin[1],(fb[2]-gyro[2])/mlin[2]};
        r.linear_welt=lage.modellvektor_nach_welt(plus(dvb,cross(om,vb)));
        Vektor3 imp{};
        for (std::size_t i=0;i<3;++i)
            for (std::size_t j=0;j<3;++j) imp[i]+=ieff[3*i+j]*om[j];
        auto rhs=plus(kr.moment_modell_nm,mal(cross(om,imp),-1.));
        rhs=plus(rhs,mal(cross(vb,pl),-1.));
        r.winkel=loesen(ieff,rhs);
        return r;
    };
    const auto k=kraefte(modell,s.pose,context.get(),rho,g,geflutet,
        hydro_context.get(),s.geschwindigkeit_welt_m_s,s.omega_modell_rad_s,bewegungsstellungen);
    const auto beschl=beschleunigung(s.pose,k,s.geschwindigkeit_welt_m_s,s.omega_modell_rad_s);
    const auto a=beschl.linear_welt;
    const auto alpha=beschl.winkel;
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
    const auto midvelocity=plus(s.geschwindigkeit_welt_m_s,mal(a,dt/2));
    const auto midforce=kraefte(modell,midpose,context.get(),rho,g,geflutet,
        hydro_context.get(),midvelocity,omega_mid,bewegungsstellungen);
    const auto midbeschl=beschleunigung(midpose,midforce,midvelocity,omega_mid);
    const auto midalpha=midbeschl.winkel;
    const auto cgnew=plus(plus(cg,mal(s.geschwindigkeit_welt_m_s,dt)),mal(a,dt*dt/2));
    const Pose rotation{{},qnew};
    Zustand result;
    result.pose=Pose{plus(cgnew,mal(rotation.modellvektor_nach_welt(modell.schwerpunkt_m),-1.)),qnew};
    result.geschwindigkeit_welt_m_s=plus(s.geschwindigkeit_welt_m_s,
        mal(midbeschl.linear_welt,dt));
    result.omega_modell_rad_s=plus(s.omega_modell_rad_s,mal(midalpha,dt));
    result.zeit_s=s.zeit_s+dt;result.schritt=s.schritt+1;
    result.geflutet=flutung(modell,result.pose,geflutet);
    result.bewegungsstellungen=std::move(bewegungsstellungen);
    result.letzte_bewegungsereigniskennung=letzte_bewegungskennung;
    result.bewegungsuebergang=BewegungsUebergang{
        s.schritt,s.schritt+1,s.zeit_s,dt,bewegungsereignisse};
    if(hydro_context) {
        hydro_context->auswertung_schritt=result.schritt;
        hydro_context->auswertung_zeit_s=result.zeit_s;
        result.hydrodynamik=hydro_context;
        result.hydrodynamik_erforderlich=true;
        kraefte(modell,result.pose,context.get(),rho,g,result.geflutet,
            hydro_context.get(),result.geschwindigkeit_welt_m_s,result.omega_modell_rad_s);
    } else result.hydrodynamik_erforderlich=false;
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
