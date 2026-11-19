#include "segelphysik/sailforce.hpp"
#include "segelphysik/simulation.hpp"
#include "segelphysik/wind.hpp"
#include "segelphysik/aufbereitung.hpp"
#include "segelphysik/messung.hpp"
#include "segelphysik/segelkonfigurator.hpp"

#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <future>
#include <iostream>
#include <stdexcept>
#include <string>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <Psapi.h>
#endif

using namespace segelphysik;
using Json = nlohmann::json;

namespace {
Json speicher()
{
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX counters{};
    if (!K32GetProcessMemoryInfo(GetCurrentProcess(),
        reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),sizeof(counters)))
        throw std::runtime_error("Prozessspeicher nicht messbar: "+std::to_string(GetLastError()));
    return {{"status","gemessen"},{"working_set_bytes",counters.WorkingSetSize},
        {"peak_working_set_bytes_prozess",counters.PeakWorkingSetSize},
        {"private_bytes",counters.PrivateUsage}};
#else
    return {{"status","auf dieser Plattform nicht gemessen"}};
#endif
}
void require(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}
void close(const Vektor3& a, const Vektor3& b, const char* label = "")
{
    for (std::size_t i = 0; i < 3; ++i)
        if (std::abs(a[i]-b[i]) > 1e-9 + 1e-10*std::max(std::abs(a[i]),std::abs(b[i])))
            throw std::runtime_error(std::string(label) + " Kraft-/Momentparitaet verletzt: " +
                std::to_string(a[i]) + " != " + std::to_string(b[i]));
}
template<class F> void rejected(F fn)
{
    bool caught = false;
    try { fn(); }
    catch (const std::invalid_argument&) { caught = true; }
    require(caught, "Ungueltige sailforce-Eingabe nicht abgelehnt");
}
Zustand start(const Modell& model, const std::filesystem::path& path)
{
    Zustand initial;
    initial.pose = Pose{{0.,0.,.2},{0.,0.,0.,1.}};
    WindKonfiguration wind;
    wind.herkunft_welt_grad = 65.;
    wind.referenzgeschwindigkeit_m_s = 3.;
    initial = wind_start(model,initial,wind,std::nullopt,"","Keine Mastreferenz fuer Vertragstest.");
    return sailforce_start(model,initial,path);
}
Kraefte forces(const Modell& model, const Zustand& state)
{
    return kraefte(model,state.pose,nullptr,1000.,9.81,state.geflutet,
        state.hydrodynamik.get(),state.geschwindigkeit_welt_m_s,
        state.omega_modell_rad_s,state.bewegungsstellungen,
        state.dynamische_fremdkraft.get(),&state);
}
void contract(const std::filesystem::path& path, std::size_t expected)
{
    const auto model = lade_modell(path);
    auto state = start(model,path);
    require(state.sailforce && state.sailforce->konfiguration.segel.size() == expected,
        "Explizite Modellzuordnung unvollstaendig");
    const auto initial = state;
    Modellaufbereitung prepared(model);
    const auto frozen=prepared.snapshot(initial);
    auto base = state;
    base.sailforce.reset();
    base.sailforce_erforderlich = false;
    const auto baseline = forces(model,base);
    const auto loaded = forces(model,state);
    require(loaded.sailforce && loaded.sailforce->segel.size() == expected, "Segelergebnis fehlt");
    auto expected_force = baseline.kraft_welt_n;
    auto expected_moment = baseline.moment_modell_nm;
    const auto aero_world = state.pose.modellvektor_nach_welt(loaded.sailforce->kraft_modell_n);
    for (std::size_t i = 0; i < 3; ++i) {
        expected_force[i] += aero_world[i];
        expected_moment[i] += loaded.sailforce->moment_s_modell_nm[i];
    }
    close(loaded.kraft_welt_n,expected_force,"Gesamtkraft");
    close(loaded.moment_modell_nm,expected_moment,"Gesamtmoment");
    auto contributions = loaded.beitraege;
    const auto balance = bilanz(model,contributions,false);
    close(balance.gesamt.moment_s_nm,loaded.moment_modell_nm,"Kraeftebilanz Moment");
    close(state.pose.modellvektor_nach_welt(balance.gesamt.kraft_modell_n),
        loaded.kraft_welt_n,"Kraeftebilanz Kraft");
    const auto json = Json::parse(zustand_json(model,state,"test"));
    require(Json::parse(snapshot_json(*frozen,"test"))==json,
        "Eingefrorene Sailforce-Lasten nicht identisch serialisiert");
    require(json.at("vertragsversion") == "8" && json.contains("sailforce"),"Ergebnisvertrag fehlt");
    const auto initial_revision = state.sailforce->revision;
    std::size_t changed_angles = 0;
    const auto catalog = modellkatalog(path.parent_path());
    const auto inspection = readModelInspection(catalog, model.id, model.fassung);
    const auto geometry = leseSegelKonfiguration(catalog, *inspection);
    for (const auto& sail : json.at("sailforce").at("segel")) {
        require(!sail.at("streifen").empty() && !sail.at("grenzlinien_modell_m").empty(),
            "Reale Streifen oder Grenzen fehlen");
        const auto id = sail.at("element_id").get<std::string>();
        const auto reference = std::find_if(geometry.referenzen.begin(),
            geometry.referenzen.end(),
            [&](const SegelReferenz& value) { return value.element_id == id; });
        if (reference != geometry.referenzen.end() &&
            reference->gruppenbewegung &&
            reference->gruppenbewegung->art == Bewegungsart::drehen) {
            state = sailforce_stellwinkel(state,id,-20.);
            ++changed_angles;
        } else {
            rejected([&] { (void)sailforce_stellwinkel(state,id,-20.); });
        }
    }
    const auto changed = Json::parse(zustand_json(model,state,"test"));
    require(changed.at("sailforce").at("revision").get<std::size_t>() ==
        initial_revision + changed_angles,
        "Stellungsrevision fehlt");
    for (std::size_t i = 0; i < expected; ++i) {
        const auto& sail = json.at("sailforce").at("segel").at(i);
        const auto& lines = sail.at("grenzlinien_zuordnung");
        require(lines.size() == sail.at("grenzlinien_modell_m").size(),
            "Grenzlinienkennungen fehlen");
        require(lines == changed.at("sailforce").at("segel").at(i).at("grenzlinien_zuordnung"),
            "Stellwinkel veraendert Grenzlinienidentitaet");
        std::vector<std::size_t> ids;
        for (const auto& strip : sail.at("streifen"))
            ids.push_back(strip.at("id").get<std::size_t>());
        for (const auto& line : lines) {
            require(!line.at("streifen_unten_id").is_null() ||
                !line.at("streifen_oben_id").is_null(), "Grenze ohne Nachbarstreifen");
            for (const auto* key : {"streifen_unten_id","streifen_oben_id"})
                if (!line.at(key).is_null())
                    require(std::find(ids.begin(),ids.end(),line.at(key).get<std::size_t>()) != ids.end(),
                        "Grenze verweist auf fremden Streifen");
        }
    }
    require(Json::parse(zustand_json(model,initial,"test")) == json, "Historisches Ergebnis veraendert");
    require(Json::parse(snapshot_json(*frozen,"test"))==json,
        "Stellwinkel veraendert bereits berechneten Sailforce-Snapshot");
    for (const auto& reference : geometry.referenzen) {
        require(reference.intrinsische_xz_spiegelung, "Aero-Spiegelebene nicht uebernommen");
        if (!reference.gruppenbewegung ||
            reference.gruppenbewegung->art != Bewegungsart::drehen) {
            rejected([&] { (void)segel_stellung(geometry,reference.element_id,-20.); });
            continue;
        }
        const auto placement = segel_stellung(geometry,reference.element_id,-20.);
        const auto rendered = stelleSegelStellwinkelVorschauDar(geometry,reference.element_id,-20.);
        require(rendered.status == SegelErgebnisStatus::Vorschau && !rendered.meshes.empty(),
            "Historische Segelgeometrie nicht darstellbar");
        const auto raw = std::find_if(geometry.grundmeshes.begin(),geometry.grundmeshes.end(),
            [&](const InspectionMesh& mesh) { return mesh.element_id == reference.element_id; });
        require(raw != geometry.grundmeshes.end(), "Intrinsisches Segelmesh fehlt");
        const auto& actual = rendered.meshes.front().points_m;
        require(actual.size() == raw->points_m.size(), "Spiegelung veraendert Netzgroesse");
        for (std::size_t p = 0; p < actual.size(); ++p) {
            auto point = raw->points_m[p];
            if (placement.form_gespiegelt) point[1] = -point[1];
            close(actual[p],placement.segel_nach_modell.modellpunkt_nach_welt(point),
                "Segelstellungsmesh");
        }
        const auto group = getGroupMovementPreview(*inspection, reference.gruppen_id);
        require(group.has_value(), "JSON-Segelgruppe liefert keine Bewegungsdefinition");
        const auto posed_group = getGroupMovementPreview(*inspection, reference.gruppen_id,
            group->reference_value - 20.);
        require(posed_group.has_value(), "JSON-Gruppenstellung kann nicht berechnet werden");
        const auto requested = std::find_if(posed_group->poses.begin(), posed_group->poses.end(),
            [](const InspectionMovementPose& pose) { return pose.label == "requested"; });
        require(requested != posed_group->poses.end(), "Angeforderte JSON-Gruppenpose fehlt");
        const auto group_sail = std::find_if(requested->meshes.begin(), requested->meshes.end(),
            [&](const InspectionMesh& mesh) { return mesh.element_id == reference.element_id; });
        require(group_sail != requested->meshes.end() &&
            group_sail->points_m.size() == actual.size(),
            "JSON-Gruppenpose enthaelt das Segelmesh nicht vollstaendig");
        // Compare rotation without tack mirroring, which uses a separate mesh reflection path.
        const auto positive_group = getGroupMovementPreview(*inspection, reference.gruppen_id,
            group->reference_value + 20.);
        require(positive_group.has_value(), "Positive JSON-Gruppenstellung kann nicht berechnet werden");
        const auto positive_requested = std::find_if(positive_group->poses.begin(),
            positive_group->poses.end(), [](const InspectionMovementPose& pose) {
                return pose.label == "requested";
            });
        require(positive_requested != positive_group->poses.end(),
            "Positive angeforderte JSON-Gruppenpose fehlt");
        const auto positive_group_sail = std::find_if(positive_requested->meshes.begin(),
            positive_requested->meshes.end(), [&](const InspectionMesh& mesh) {
                return mesh.element_id == reference.element_id;
            });
        const auto positive_rendered = stelleSegelStellwinkelVorschauDar(
            geometry, reference.element_id, 20.);
        require(positive_group_sail != positive_requested->meshes.end() &&
            positive_rendered.status == SegelErgebnisStatus::Vorschau &&
            !positive_rendered.meshes.empty() &&
            positive_group_sail->points_m.size() == positive_rendered.meshes.front().points_m.size(),
            "Positive Segelpose ist nicht zwischen JSON-Gruppe und Segelvorschau verfuegbar");
        for (std::size_t p = 0; p < positive_group_sail->points_m.size(); ++p)
            close(positive_group_sail->points_m[p],
                positive_rendered.meshes.front().points_m[p], "Positive JSON-Gruppenpose");
    }
    std::vector<std::future<std::string>> readers;
    for (int i = 0; i < 4; ++i)
        readers.push_back(std::async(std::launch::async,[&] {
            return zustand_json(model,initial,"test");
        }));
    (void)forces(model,state);
    for (auto& reader : readers)
        require(Json::parse(reader.get()) == json, "Asynchroner historischer Snapshot inkonsistent");
    auto still = initial;
    WindKonfiguration quiet;
    quiet.referenzgeschwindigkeit_m_s = .01;
    quiet.referenzhoehe_m = 100.;
    still.wind = std::make_shared<const WindKontext>(WindKontext{quiet,0.,model.fassung});
    still.geschwindigkeit_welt_m_s = {-.01,0.,0.};
    const auto zero = forces(model,still);
    close(zero.sailforce->kraft_modell_n,{});
    close(zero.sailforce->moment_s_modell_nm,{});
    require(zero.sailforce->sdp_gesamt.status == SailforceSdpStatus::nicht_bestimmbar,
        "Aktives Segel bei scheinbarem Nullwind wurde deaktiviert");
    rejected([&] { (void)sailforce_stellwinkel(state,"unbekannt",10.); });
    rejected([&] { (void)gleichgewicht(model,state); });
    auto missing = state;
    missing.sailforce.reset();
    rejected([&] { (void)dynamikschritt(model,missing,.001); });
    auto wrong = model;
    wrong.fassung += "-falsch";
    rejected([&] { (void)sailforce_auswertung(wrong,state,model.schwerpunkt_m); });
    auto foreign = dynamische_fremdkraft_start(model,state,
        {true,{7.,-4.,2.},{1.,0.,1.}});
    const auto combined = forces(model,foreign);
    const auto only_aero = forces(model,state);
    auto combined_expected = only_aero.kraft_welt_n;
    const auto f = state.pose.modellvektor_nach_welt({7.,-4.,2.});
    for (std::size_t i = 0; i < 3; ++i) combined_expected[i] += f[i];
    close(combined.kraft_welt_n,combined_expected);
    require(combined.sailforce->sdp_gesamt.summen.querkraft_n ==
        only_aero.sailforce->sdp_gesamt.summen.querkraft_n, "Fremdkraft im Aero-SDP");
    const auto next = dynamikschritt(model,foreign,.001);
    require(next.sailforce == foreign.sailforce && next.schritt == 1, "Dynamikkontext nicht erhalten");
    auto invalid_wind = state;
    WindKonfiguration too_fast;
    too_fast.referenzgeschwindigkeit_m_s = 100.;
    too_fast.referenzhoehe_m = .01;
    invalid_wind.wind = std::make_shared<const WindKontext>(
        WindKontext{too_fast,0.,model.fassung});
    rejected([&] { (void)dynamikschritt(model,invalid_wind,.001); });
    const auto stage_fail = dynamische_fremdkraft_start(model,initial,
        {true,{1e8,0.,0.},model.schwerpunkt_m});
    (void)forces(model,stage_fail);
    rejected([&] { (void)dynamikschritt(model,stage_fail,.01); });
    require(Json::parse(zustand_json(model,initial,"test")) == json,
        "Fehler in Integratorzwischenstufe veraendert letzten gueltigen Zustand");
    auto stationary_input = initial;
    WindKonfiguration light_wind;
    light_wind.herkunft_welt_grad = 65.;
    light_wind.referenzgeschwindigkeit_m_s = .01;
    stationary_input.wind = std::make_shared<const WindKontext>(
        WindKontext{light_wind,0.,model.fassung});
    StationaereGleichgewichtKonfiguration stationary_config;
    stationary_config.aktiv = false;
    const auto stationary = stationaeres_gleichgewicht(model,stationary_input,
        stationary_config,{},.1,.1,1);
    require(stationary.status != "ungueltig", "Stationaerer sailforce-Pfad ungueltig");
    const auto stationary_forces = forces(model,stationary.zustand);
    close(stationary.residuum_kraft_welt_n,stationary_forces.kraft_welt_n);
    close(stationary.residuum_moment_modell_nm,stationary_forces.moment_modell_nm);
    stationary_config.aktiv = true;
    stationary_config.kurvenfahrt = true;
    const auto turning = stationaeres_gleichgewicht(model,stationary_input,
        stationary_config,{},.1,.1,1);
    require(turning.status != "ungueltig", "Stationaerer Kurvenfahrtpfad mit sailforce ungueltig");
    require(turning.zustand.sailforce &&
        turning.zustand.sailforce->konfiguration.segel.size() ==
            initial.sailforce->konfiguration.segel.size(),
        "Stationaerer Fallback verlor den Sailforce-Kontext fuer die Segeldarstellung");
    const auto turning_forces = forces(model,turning.zustand);
    close(turning.aeussere_kraft_welt_n,turning_forces.kraft_welt_n);
    close(turning.aeusseres_moment_modell_nm,turning_forces.moment_modell_nm);
    const auto movable_sail = std::find_if(initial.sailforce->konfiguration.segel.begin(),
        initial.sailforce->konfiguration.segel.end(), [&](const SailforceSegelwahl& choice) {
            const auto reference = std::find_if(geometry.referenzen.begin(),
                geometry.referenzen.end(), [&](const SegelReferenz& value) {
                    return value.element_id == choice.element_id;
                });
            return reference != geometry.referenzen.end() &&
                reference->gruppenbewegung &&
                reference->gruppenbewegung->art == Bewegungsart::drehen;
        });
    if (movable_sail != initial.sailforce->konfiguration.segel.end()) {
        const auto stale = sailforce_stellwinkel(stationary.zustand,
            movable_sail->element_id,1.);
        rejected([&] { (void)zustand_json(model,stale,"test","",nullptr,&stationary); });
    }
    auto disabled_start = base;
    SailforceKonfiguration disabled;
    for (const auto& s : initial.sailforce->konfiguration.segel)
        disabled.segel.push_back({s.element_id,false,0.});
    disabled_start = sailforce_start(model,disabled_start,path,disabled);
    disabled_start.wind.reset();
    const auto inactive = forces(model,disabled_start);
    close(inactive.kraft_welt_n,baseline.kraft_welt_n);
    close(inactive.moment_modell_nm,baseline.moment_modell_nm);
    require(inactive.sailforce->sdp_gesamt.status == SailforceSdpStatus::inaktiv,
        "Deaktivierung nicht unabhaengig");
    const auto output = std::filesystem::temp_directory_path() /
        ("sailforce-"+model.id+"-"+std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count())+".jsonl");
    Laufoptionen options;
    options.start = base;
    options.start.wind.reset();
    options.dynamisch = true;
    options.dauer_s = .001;
    options.dt_s = .001;
    options.sailforce = disabled;
    const auto report = simulieren(model,path,output,options);
    require(report.status == "abgeschlossen" && report.zustaende == 2,
        "Explizite sailforce-Laufoptionen nicht exportierbar");
    std::ifstream recorded(output);
    std::string line;
    while (std::getline(recorded,line)) {
        const auto snapshot = Json::parse(line);
        require(snapshot.at("vertragsversion") == "8" &&
            snapshot.at("sailforce").at("sdp_gesamt").at("status") == "inaktiv",
            "Laufoptionen-Aktivierung oder Ergebnisversion verloren");
    }
    recorded.close();
    require(std::filesystem::remove(output),"Testexport nicht entfernbar");
}
Json benchmark(const std::filesystem::path& path, std::size_t steps)
{
    std::cout << path.u8string() << ": Vorbereitung" << std::endl;
    const auto prep_begin = std::chrono::steady_clock::now();
    const auto model = lade_modell(path);
    auto state = start(model,path);
    auto seed = state;
    seed.sailforce.reset();
    seed.sailforce_erforderlich = false;
    const auto equilibrium = gleichgewicht(model,seed);
    require(equilibrium.status == "konvergiert","Kein gueltiger hydrostatischer Benchmarkstart");
    state.pose = equilibrium.zustand.pose;
    state = hydrodynamik_start(model,state,{});
    Modellaufbereitung runtime(model);
    const auto prep_end = std::chrono::steady_clock::now();
    const auto preparation_memory = speicher();
    std::cout << "Vorbereitung: " <<
        std::chrono::duration<double>(prep_end-prep_begin).count() << " s" << std::endl;
    Json profile;
    if (steps == 100) {
        const auto aero_begin = std::chrono::steady_clock::now();
        for (std::size_t i = 0; i < steps; ++i)
            (void)sailforce_auswertung(model,state,model.schwerpunkt_m);
        profile["aero_auswertung_ms"] = std::chrono::duration<double,std::milli>(
            std::chrono::steady_clock::now()-aero_begin).count()/static_cast<double>(steps);
        auto no_sail = state;
        no_sail.sailforce.reset();
        no_sail.sailforce_erforderlich = false;
        const auto base_begin = std::chrono::steady_clock::now();
        for (std::size_t i = 0; i < steps; ++i) {
            no_sail = runtime.schritt(no_sail,.01);
            (void)runtime.auswertung(no_sail);
        }
        profile["schritt_ohne_segel_ms"] = std::chrono::duration<double,std::milli>(
            std::chrono::steady_clock::now()-base_begin).count()/static_cast<double>(steps);
        Modellaufbereitung reader(model);
        double capture_ms=0.,full_json_ms=0.,snapshot_json_ms=0.;
        for (std::size_t i=0;i<20;++i) {
            auto probe=state;
            auto position=probe.pose.position_m();
            position[0]+=static_cast<double>(i)*.001;
            probe.pose=Pose{position,probe.pose.orientierung_xyzw()};
            (void)runtime.auswertung(probe);
            const auto capture_begin=std::chrono::steady_clock::now();
            const auto snapshot=runtime.snapshot(probe);
            capture_ms+=std::chrono::duration<double,std::milli>(
                std::chrono::steady_clock::now()-capture_begin).count();
            const auto full_begin=std::chrono::steady_clock::now();
            const auto full=reader.json(probe,"profil");
            full_json_ms+=std::chrono::duration<double,std::milli>(
                std::chrono::steady_clock::now()-full_begin).count();
            const auto snapshot_begin=std::chrono::steady_clock::now();
            const auto serialized=snapshot_json(*snapshot,"profil");
            snapshot_json_ms+=std::chrono::duration<double,std::milli>(
                std::chrono::steady_clock::now()-snapshot_begin).count();
            require(full==serialized,"Snapshotprofil veraendert serialisierten Zustand");
        }
        profile["snapshot_cacheuebernahme_ms"]=capture_ms/20.;
        profile["kraftneuberechnung_und_json_ms"]=full_json_ms/20.;
        profile["snapshot_json_ms"]=snapshot_json_ms/20.;
        std::cout << profile.dump() << std::endl;
    }
    std::vector<double> samples;
    samples.reserve(steps);
#ifdef SEGELPHYSIK_TEST_INSTRUMENTATION
    testmessungen_zuruecksetzen();
    testmessungen_aktivieren(steps==100);
#endif
    const auto begin = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < steps; ++i) {
        const auto step = std::chrono::steady_clock::now();
        state = runtime.schritt(state,.01);
        const auto result = runtime.auswertung(state);
        require(result.sailforce != nullptr,"Benchmark ohne Segelphysik");
        samples.push_back(std::chrono::duration<double,std::milli>(
            std::chrono::steady_clock::now()-step).count());
        if ((i+1) % 1000 == 0) std::cout << i+1 << " Schritte" << std::endl;
    }
    const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();
#ifdef SEGELPHYSIK_TEST_INSTRUMENTATION
    testmessungen_aktivieren(false);
    for (const auto& messwert:testmessungen())
        profile["core_phasen"][messwert.first]={{"anzahl",messwert.second.anzahl},
            {"summe_ms",messwert.second.sekunden*1000.}};
    if (steps==100) std::cout << profile.at("core_phasen").dump() << std::endl;
#endif
    require(state.schritt == steps && std::abs(state.zeit_s-static_cast<double>(steps)*.01) < 1e-8,
        "Benchmark hat nicht alle gueltigen Zeitschritte abgeschlossen");
    const Json raw_samples = samples;
    std::sort(samples.begin(),samples.end());
    return {{"modell",model.id},{"schritte",state.schritt},{"simulationszeit_s",state.zeit_s},
        {"wandzeit_s",seconds},{"echtzeit_bestanden",seconds <= static_cast<double>(steps)*.01},
        {"vorbereitung_s",std::chrono::duration<double>(prep_end-prep_begin).count()},
        {"speicher_nach_vorbereitung",preparation_memory},{"speicher_nach_lauf",speicher()},
        {"konfiguration",{{"dt_s",.01},{"wind_m_s",3.},{"herkunft_grad",65.},
            {"wind_referenzhoehe_m",10.},{"luftdichte_kg_m3",1.225},
            {"kinematische_viskositaet_m2_s",1.5e-5},{"hydrodynamik",true},
            {"alle_segel_aktiv",true},{"stellwinkel_grad",0.},{"fremdkraft_aktiv",false}}},
        {"schrittzeiten_ms",raw_samples},{"rueckstand_s",std::max(0.,seconds-state.zeit_s)},
        {"profil",profile},
        {"schritt_ms",{{"median",samples[steps/2]},{"p95",samples[steps*95/100]},{"maximum",samples.back()}}},
        {"endzustand",Json::parse(zustand_json(model,state,"benchmark"))}};
}
}
int main(int argc, char** argv)
{
    try {
        require(argc == 2 || argc == 4,"Erwartet Modellwurzel [--benchmark Ergebnisdatei]");
        const auto root = std::filesystem::u8path(argv[1]);
        if (argc == 4) {
            const std::string mode = argv[2];
            require(mode == "--benchmark" || mode == "--profile","Unbekannter Modus");
            const std::size_t steps = mode == "--benchmark" ? 6000 : 100;
            Json result = Json::array();
            bool passed = true;
            for (const auto* boat : {"420","folkeboot","optimist"}) {
                auto row = benchmark(root / boat / "modells_data.json",steps);
                passed = passed && row.at("echtzeit_bestanden").get<bool>();
                std::cout << boat << ": " << row.at("wandzeit_s") << " s" << std::endl;
                result.push_back(std::move(row));
            }
            std::ofstream output(std::filesystem::u8path(argv[3]));
            if (!output) throw std::runtime_error("Benchmarkdatei nicht schreibbar");
            output << result.dump(2);
            output.close();
            require(!output.fail(),"Benchmarkdatei nicht vollstaendig geschrieben");
            return passed ? 0 : 1;
        }
        contract(root / "420" / "modells_data.json",2);
        contract(root / "folkeboot" / "modells_data.json",2);
        contract(root / "optimist" / "modells_data.json",1);
        std::cout << "Reale sailforce-Integration bestanden\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
