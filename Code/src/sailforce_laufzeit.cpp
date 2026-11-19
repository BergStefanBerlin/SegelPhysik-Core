#include "segelphysik/sailforce.hpp"
#include "segelphysik/hydrostatik.hpp"
#include "segelphysik/segelkonfigurator.hpp"
#include "segelphysik/wind.hpp"
#include "modell_intern.hpp"

#include <segel/modell.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>

namespace segelphysik {
namespace {
using Json = nlohmann::json;
using Linien = std::vector<SailforceGrenzlinie>;

struct SegelQuelle {
    std::string element_id;
    std::string fassung;
    std::shared_ptr<const segel::modell::Modell> modell;
    LuftGueltigkeitsbereich luftbereich;
    std::shared_ptr<const Linien> linien;
};

Json lesen(const std::filesystem::path& path)
{
    std::ifstream input(path);
    if (!input) throw std::invalid_argument("CORE-SIM-0001: sailforce-Datei nicht lesbar: " + path.u8string());
    try { return Json::parse(input); }
    catch (const Json::exception& e) {
        throw std::invalid_argument("CORE-SIM-0001: sailforce-JSON " + path.u8string() + ": " + e.what());
    }
}

void add(Vektor3& summe, const Vektor3& value)
{
    for (std::size_t i = 0; i < 3; ++i) {
        summe[i] += value[i];
        if (!std::isfinite(summe[i]))
            throw std::invalid_argument("CORE-SIM-0001: Nicht endliche sailforce-Lastsumme.");
    }
}

std::shared_ptr<const Linien> schneiden(const InspectionMesh& mesh,
    const std::vector<double>& grenzen, std::vector<::segel::modell::Punkt> streifen)
{
    auto result = std::make_shared<Linien>();
    std::sort(streifen.begin(), streifen.end(), [](const auto& a, const auto& b) {
        return a.position_m[2] < b.position_m[2];
    });
    if (grenzen.size() != streifen.size()+1 || streifen.empty())
        throw std::invalid_argument("CORE-SIM-0001: Streifengrenzen und Abfragepunkte passen nicht zusammen.");
    for (std::size_t i = 0; i < streifen.size(); ++i)
        if (!std::isfinite(grenzen[i]) || !std::isfinite(grenzen[i+1]) ||
            grenzen[i] >= grenzen[i+1] ||
            streifen[i].position_m[2] < grenzen[i] ||
            streifen[i].position_m[2] > grenzen[i+1])
            throw std::invalid_argument("CORE-SIM-0001: Ungueltige Streifengrenzenzuordnung.");
    const auto gleich = [](const Vektor3& a, const Vektor3& b) {
        return std::hypot(a[0]-b[0], a[1]-b[1], a[2]-b[2]) < 1e-10;
    };
    for (std::size_t grenze = 0; grenze < grenzen.size(); ++grenze) {
        const double z = grenzen[grenze];
        std::vector<std::vector<Vektor3>> segmente;
        for (const auto& triangle : mesh.triangles) {
            std::vector<Vektor3> points;
            const auto aufnehmen = [&](const Vektor3& point) {
                if (std::none_of(points.begin(), points.end(), [&](const Vektor3& p) {
                    return std::hypot(p[0]-point[0], p[1]-point[1], p[2]-point[2]) < 1e-10;
                })) points.push_back(point);
            };
            for (std::size_t i = 0; i < 3; ++i) {
                const auto& a = mesh.points_m.at(triangle[i]);
                const auto& b = mesh.points_m.at(triangle[(i+1)%3]);
                const double da = a[2]-z, db = b[2]-z;
                if (std::abs(da) < 1e-10) aufnehmen({a[0], a[1], z});
                if ((da < 0. && db > 0.) || (da > 0. && db < 0.)) {
                    const double t = da/(da-db);
                    aufnehmen({a[0]+t*(b[0]-a[0]), a[1]+t*(b[1]-a[1]), z});
                }
            }
            if (points.size() == 2 && std::none_of(segmente.begin(), segmente.end(),
                [&](const auto& s) {
                    return (gleich(s.front(),points.front()) && gleich(s.back(),points.back())) ||
                        (gleich(s.front(),points.back()) && gleich(s.back(),points.front()));
                })) segmente.push_back(std::move(points));
        }
        while (!segmente.empty()) {
            auto punkte = std::move(segmente.back());
            segmente.pop_back();
            bool verbunden = true;
            while (verbunden) {
                verbunden = false;
                for (auto s = segmente.begin(); s != segmente.end(); ++s) {
                    if (gleich(punkte.back(),s->front())) punkte.push_back(s->back());
                    else if (gleich(punkte.back(),s->back())) punkte.push_back(s->front());
                    else if (gleich(punkte.front(),s->back())) punkte.insert(punkte.begin(),s->front());
                    else if (gleich(punkte.front(),s->front())) punkte.insert(punkte.begin(),s->back());
                    else continue;
                    segmente.erase(s);
                    verbunden = true;
                    break;
                }
            }
            SailforceGrenzlinie linie;
            linie.id = result->size();
            linie.grenze_id = grenze;
            if (grenze > 0) linie.streifen_unten_id = streifen[grenze-1].id;
            if (grenze < streifen.size()) linie.streifen_oben_id = streifen[grenze].id;
            linie.punkte_segel_m = std::move(punkte);
            result->push_back(std::move(linie));
        }
    }
    if (result->empty())
        throw std::invalid_argument("CORE-SIM-0001: Keine echten sailforce-Streifengrenzlinien.");
    return result;
}

Json sdp_json(const SailforceSdp& value)
{
    const auto& s = value.summen;
    Json result{{"status", value.status == SailforceSdpStatus::gueltig ? "gueltig" :
        value.status == SailforceSdpStatus::inaktiv ? "inaktiv" : "nicht_bestimmbar"},
        {"position_modell_m", nullptr}, {"Q_N", s.querkraft_n},
        {"B_N", s.einzelbetragssumme_n}, {"X_Nm", s.erstes_moment_x_nm},
        {"Z_Nm", s.erstes_moment_z_nm}};
    if (value.position_modell_m) result["position_modell_m"] = *value.position_modell_m;
    return result;
}
}

struct SailforceVorbereitung {
    SegelKonfigurationSnapshot stellung;
    std::vector<SegelQuelle> quellen;
};

Zustand sailforce_start(const Modell& modell, const Zustand& start,
    const std::filesystem::path& modellpfad, const SailforceKonfiguration& konfiguration) try
{
    if (start.sailforce || start.sailforce_erforderlich)
        throw std::invalid_argument("CORE-SIM-0001: sailforce bereits vorbereitet.");
    const double min = std::numeric_limits<double>::min();
    const double max = std::numeric_limits<double>::max();
    pruefe_lufteigenschaften(konfiguration.luft, {min, max, min, max});
    const auto document = lesen(modellpfad);
    std::map<std::string, std::filesystem::path> zuordnung;
    std::map<std::string, std::filesystem::path> definitionen;
    if (document.contains("groups")) {
        for (const auto& group : document.at("groups"))
            for (const auto& element : group.at("elements")) {
                if (!element.contains("physics") || !element.at("physics").contains("aerodynamic"))
                    continue;
                const auto& aero = element.at("physics").at("aerodynamic");
                if (aero.contains("sailModel")) {
                    const auto id = element.at("id").get<std::string>();
                    zuordnung.emplace(id,
                        modellpfad.parent_path() / std::filesystem::u8path(aero.at("sailModel").get<std::string>()));
                    definitionen.emplace(id, modellpfad.parent_path() /
                        std::filesystem::u8path(aero.at("definition").get<std::string>()));
                }
            }
    }
    std::set<std::string> gewaehlt;
    for (const auto& wahl : konfiguration.segel) {
        if (!zuordnung.count(wahl.element_id) || !gewaehlt.insert(wahl.element_id).second ||
            !std::isfinite(wahl.stellwinkel_grad))
            throw std::invalid_argument("CORE-SIM-0001: Unbekannte, doppelte oder ungueltige sailforce-Auswahl: " + wahl.element_id);
    }
    if (zuordnung.empty()) return start;
    auto preparation = std::make_shared<SailforceVorbereitung>();
    const auto catalog = modellkatalog(modellpfad.parent_path());
    const auto inspection = readModelInspection(catalog, modell.id, modell.fassung);
    preparation->stellung = leseSegelKonfiguration(catalog, *inspection);
    if (preparation->stellung.status == SegelKonfigurationsStatus::Fehler)
        throw std::invalid_argument("CORE-SIM-0001: Ungueltige sailforce-Montage: " +
            (preparation->stellung.diagnosen.empty() ? std::string("ohne Diagnose") :
                preparation->stellung.diagnosen.front().nachricht));
    auto context = std::make_shared<SailforceKontext>();
    context->modell_fassung = modell.fassung;
    std::string quellen_fassung = document.dump();
    context->konfiguration.luft = konfiguration.luft;
    for (const auto& entry : zuordnung) {
        const auto choice = std::find_if(konfiguration.segel.begin(), konfiguration.segel.end(),
            [&](const SailforceSegelwahl& w) { return w.element_id == entry.first; });
        const auto wahl = choice == konfiguration.segel.end()
            ? SailforceSegelwahl{entry.first, true, 0.} : *choice;
        context->konfiguration.segel.push_back(wahl);
        const auto manifest = lesen(entry.second / "manifest.json");
        std::ifstream original(definitionen.at(entry.first), std::ios::binary);
        if (!original)
            throw std::invalid_argument("CORE-SIM-0001: Intrinsische sailforce-Quelle nicht lesbar: " + entry.first);
        std::ostringstream bytes;
        bytes << original.rdbuf();
        if (original.bad() || sha256_bytes(bytes.str()) !=
            manifest.at("eingaben").at("geometrie").at("sha256").get<std::string>())
            throw std::invalid_argument("CORE-SIM-0001: sailforce-Quellgeometrie passt nicht zum Manifest: " + entry.first);
        const auto& validity = manifest.at("gueltigkeit");
        const LuftGueltigkeitsbereich range{
            validity.at("dichte_kg_m3").at(0).get<double>(),
            validity.at("dichte_kg_m3").at(1).get<double>(),
            validity.at("viskositaet_m2_s").at(0).get<double>(),
            validity.at("viskositaet_m2_s").at(1).get<double>()};
        if (wahl.aktiv) pruefe_lufteigenschaften(konfiguration.luft, range);
        SegelQuelle quelle;
        quelle.element_id = entry.first;
        quelle.fassung = sha256_bytes(manifest.dump());
        quellen_fassung += "\n" + entry.first + ":" + quelle.fassung;
        quelle.luftbereich = range;
        try { quelle.modell = segel::modell::laden(entry.second); }
        catch (const segel::modell::Fehler& e) {
            throw std::invalid_argument("CORE-SIM-0001: sailforce " + entry.first + ": " + e.what());
        }
        const auto mesh = std::find_if(preparation->stellung.grundmeshes.begin(),
            preparation->stellung.grundmeshes.end(),
            [&](const InspectionMesh& value) { return value.element_id == entry.first; });
        if (mesh == preparation->stellung.grundmeshes.end())
            throw std::invalid_argument("CORE-SIM-0001: sailforce-Quellmesh fehlt: " + entry.first);
        const auto reference = std::find_if(preparation->stellung.referenzen.begin(),
            preparation->stellung.referenzen.end(),
            [&](const SegelReferenz& r) { return r.element_id == entry.first; });
        if (reference == preparation->stellung.referenzen.end() ||
            std::abs(reference->ecken.hals_m[1]) > 1e-9 ||
            std::abs(reference->ecken.kopf_vorliek_m[1]) > 1e-9)
            throw std::invalid_argument("CORE-SIM-0001: sailforce-Vorliek liegt nicht in der intrinsischen x/z-Spiegelebene: " + entry.first);
        const auto lage = std::find_if(preparation->stellung.lagen.begin(),
            preparation->stellung.lagen.end(),
            [&](const SegelLage& l) { return l.element_id == entry.first; });
        if (lage == preparation->stellung.lagen.end() ||
            lage->original_buglage == SegelBuglage::Unbekannt)
            throw std::invalid_argument("CORE-SIM-0001: Explizite sailforce-Originalformseite fehlt: " + entry.first);
        quelle.linien = schneiden(*mesh, quelle.modell->streifengrenzen_m(), quelle.modell->punkte());
        (void)segel_stellung(preparation->stellung, entry.first, wahl.stellwinkel_grad);
        preparation->quellen.push_back(std::move(quelle));
    }
    context->vorbereitung = std::move(preparation);
    context->quellen_fassung = sha256_bytes(quellen_fassung);
    Zustand result = start;
    result.sailforce = std::move(context);
    result.sailforce_erforderlich = true;
    return result;
}
catch (const Json::exception& e) {
    throw std::invalid_argument("CORE-SIM-0001: Ungueltiger sailforce-Modellvertrag: " +
        std::string(e.what()));
}

Zustand sailforce_stellwinkel(const Zustand& start, const std::string& element_id,
    double winkel_grad)
{
    if (!start.sailforce || !start.sailforce->vorbereitung || !std::isfinite(winkel_grad))
        throw std::invalid_argument("CORE-SIM-0001: sailforce-Kontext fehlt oder Winkel ungueltig.");
    auto context = std::make_shared<SailforceKontext>(*start.sailforce);
    const auto choice = std::find_if(context->konfiguration.segel.begin(), context->konfiguration.segel.end(),
        [&](const SailforceSegelwahl& value) { return value.element_id == element_id; });
    if (choice == context->konfiguration.segel.end())
        throw std::invalid_argument("CORE-SIM-0001: Unbekanntes sailforce-Segel: " + element_id);
    (void)segel_stellung(context->vorbereitung->stellung, element_id, winkel_grad);
    if (context->revision == std::numeric_limits<std::size_t>::max())
        throw std::invalid_argument("CORE-SIM-0001: sailforce-Stellungsrevision erschoepft.");
    choice->stellwinkel_grad = winkel_grad;
    ++context->revision;
    Zustand result = start;
    result.sailforce = std::move(context);
    return result;
}

SailforceAuswertung sailforce_auswertung(const Modell& modell, const Zustand& zustand,
    const Vektor3& schwerpunkt_modell_m)
{
    if (!zustand.sailforce || !zustand.sailforce->vorbereitung ||
        zustand.sailforce->modell_fassung != modell.fassung)
        throw std::invalid_argument("CORE-SIM-0001: Fehlender oder fremder sailforce-Kontext.");
    const auto& context = *zustand.sailforce;
    const auto& preparation = *context.vorbereitung;
    SailforceAuswertung result;
    result.revision = context.revision;
    result.darstellungsbezug_modell_m = schwerpunkt_modell_m;
    std::vector<SailforceSdpSummen> summen;
    for (const auto& quelle : preparation.quellen) {
        const auto choice = std::find_if(context.konfiguration.segel.begin(), context.konfiguration.segel.end(),
            [&](const SailforceSegelwahl& w) { return w.element_id == quelle.element_id; });
        if (choice == context.konfiguration.segel.end())
            throw std::invalid_argument("CORE-SIM-0001: sailforce-Auswahl unvollstaendig.");
        SailforceSegelErgebnis segel;
        segel.element_id = quelle.element_id;
        segel.quellfassung = quelle.fassung;
        segel.aktiv = choice->aktiv;
        segel.stellwinkel_grad = choice->stellwinkel_grad;
        const auto stellung = segel_stellung(preparation.stellung, quelle.element_id,
            choice->stellwinkel_grad);
        segel.form_gespiegelt = stellung.form_gespiegelt;
        segel.segel_nach_modell = stellung.segel_nach_modell;
        segel.grenzlinien_segel_m = quelle.linien;
        segel.darstellungsbezug_modell_m = stellung.segel_nach_modell.position_m();
        if (!choice->aktiv) {
            segel.sdp.status = SailforceSdpStatus::inaktiv;
            result.segel.push_back(std::move(segel));
            continue;
        }
        const auto points = quelle.modell->punkte(stellung.form_gespiegelt);
        std::vector<::segel::modell::Wind> winds;
        winds.reserve(points.size());
        for (const auto& point : points) {
            const auto position = stellung.segel_nach_modell.modellpunkt_nach_welt(point.position_m);
            const auto wind = wind_am_punkt(modell, zustand, position, schwerpunkt_modell_m);
            const auto body = zustand.pose.weltvektor_nach_modell(wind.scheinbar.stroemung_welt_m_s);
            winds.push_back({point.id, stellung.segel_nach_modell.weltvektor_nach_modell(body)});
        }
        ::segel::modell::Transformation transform;
        transform.translation_m = stellung.segel_nach_modell.position_m();
        for (std::size_t j = 0; j < 3; ++j) {
            Vektor3 basis{};
            basis[j] = 1.;
            const auto column = stellung.segel_nach_modell.modellvektor_nach_welt(basis);
            for (std::size_t i = 0; i < 3; ++i) transform.rotation[i][j] = column[i];
        }
        ::segel::modell::Berechnung evaluation;
        try {
            evaluation = quelle.modell->berechnen_mit_sdp(winds,
                context.konfiguration.luft.dichte_kg_m3,
                context.konfiguration.luft.kinematische_viskositaet_m2_s,
                transform, stellung.form_gespiegelt);
        } catch (const ::segel::modell::Fehler& e) {
            throw std::invalid_argument("CORE-SIM-0001: sailforce " + quelle.element_id + ": " + e.what());
        }
        if (evaluation.streifenlasten.size() != points.size())
            throw std::invalid_argument("CORE-SIM-0001: Unvollstaendige sailforce-Streifenlasten.");
        std::set<std::size_t> ids;
        for (const auto& last : evaluation.streifenlasten) {
            const auto point = std::find_if(points.begin(), points.end(),
                [&](const ::segel::modell::Punkt& p) { return p.id == last.id; });
            if (point == points.end() || !ids.insert(last.id).second)
                throw std::invalid_argument("CORE-SIM-0001: Fremde oder doppelte sailforce-Streifen-ID.");
            const auto force = sailforce_modelllast(
                {point->position_m, last.kraft_N, last.moment_Nm},
                stellung.segel_nach_modell, schwerpunkt_modell_m);
            segel.streifen.push_back({last.id, force});
            add(segel.kraft_modell_n, force.kraft_modell_n);
            add(segel.moment_s_modell_nm, force.moment_s_modell_nm);
        }
        const auto& sdp = evaluation.sdp;
        const SailforceSdpSummen s{sdp.Q_N, sdp.B_N, sdp.X_Nm, sdp.Z_Nm};
        segel.sdp = sailforce_sdp(s);
        summen.push_back(s);
        add(result.kraft_modell_n, segel.kraft_modell_n);
        add(result.moment_s_modell_nm, segel.moment_s_modell_nm);
        result.segel.push_back(std::move(segel));
    }
    result.sdp_gesamt = sailforce_sdp_gesamt(summen);
    return result;
}

std::string sailforce_json(const SailforceAuswertung& value, const Lufteigenschaften& luft)
{
    Json result{{"revision", value.revision},
        {"luft", {{"dichte_kg_m3", luft.dichte_kg_m3},
            {"kinematische_viskositaet_m2_s", luft.kinematische_viskositaet_m2_s}}},
        {"kraft_modell_n", value.kraft_modell_n}, {"moment_s_modell_nm", value.moment_s_modell_nm},
        {"darstellungsbezug_modell_m", value.darstellungsbezug_modell_m},
        {"sdp_gesamt", sdp_json(value.sdp_gesamt)}, {"segel", Json::array()}};
    for (const auto& segel : value.segel) {
        Json s{{"element_id", segel.element_id}, {"quellfassung", segel.quellfassung},
            {"aktiv", segel.aktiv}, {"stellwinkel_grad", segel.stellwinkel_grad},
            {"form_gespiegelt", segel.form_gespiegelt}, {"kraft_modell_n", segel.kraft_modell_n},
            {"moment_s_modell_nm", segel.moment_s_modell_nm},
            {"darstellungsbezug_modell_m", segel.darstellungsbezug_modell_m},
            {"sdp", sdp_json(segel.sdp)}, {"streifen", Json::array()},
            {"grenzlinien_modell_m", Json::array()}, {"grenzlinien_zuordnung", Json::array()}};
        for (const auto& strip : segel.streifen)
            s["streifen"].push_back({{"id", strip.id}, {"punkt_modell_m", strip.last.darstellungsbezug_modell_m},
                {"kraft_modell_n", strip.last.kraft_modell_n}, {"moment_s_modell_nm", strip.last.moment_s_modell_nm}});
        if (segel.grenzlinien_segel_m)
            for (const auto& line : *segel.grenzlinien_segel_m) {
                Json transformed = Json::array();
                for (auto point : line.punkte_segel_m) {
                    if (segel.form_gespiegelt) point[1] = -point[1];
                    transformed.push_back(segel.segel_nach_modell.modellpunkt_nach_welt(point));
                }
                s["grenzlinien_modell_m"].push_back(std::move(transformed));
                Json zuordnung{{"id",line.id},{"grenze_id",line.grenze_id},
                    {"streifen_unten_id",nullptr},{"streifen_oben_id",nullptr}};
                if (line.streifen_unten_id) zuordnung["streifen_unten_id"] = *line.streifen_unten_id;
                if (line.streifen_oben_id) zuordnung["streifen_oben_id"] = *line.streifen_oben_id;
                s["grenzlinien_zuordnung"].push_back(std::move(zuordnung));
            }
        result["segel"].push_back(std::move(s));
    }
    return result.dump();
}

void sailforce_addieren(Kraefte& kraefte, const Modell& modell, const Zustand& zustand)
{
    auto result = std::make_shared<SailforceAuswertung>(
        sailforce_auswertung(modell, zustand, kraefte.schwerpunkt_modell_m));
    add(kraefte.kraft_welt_n, zustand.pose.modellvektor_nach_welt(result->kraft_modell_n));
    add(kraefte.moment_modell_nm, result->moment_s_modell_nm);
    for (const auto& segel : result->segel)
        for (const auto& strip : segel.streifen) {
            Beitrag b;
            b.id = "sailforce:" + segel.element_id + ":" + std::to_string(strip.id);
            b.element_id = segel.element_id;
            b.art = "kraft";
            b.quelle = Quelle::sailforce;
            b.gruppe = "sailforce";
            b.punktart = Punktart::bezugspunkt;
            b.punkt_m = strip.last.darstellungsbezug_modell_m;
            b.vektor = strip.last.kraft_modell_n;
            b.moment_s_nm = strip.last.moment_s_modell_nm;
            b.zustand = b.vektor == Vektor3{} && b.moment_s_nm == Vektor3{}
                ? Datenzustand::null : Datenzustand::gueltig;
            kraefte.beitraege.push_back(std::move(b));
        }
    kraefte.sailforce = std::move(result);
}
}
