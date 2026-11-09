#include "segelphysik/simulation.hpp"
#include "segelphysik/aufbereitung.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <cmath>

int main(int argc,char* argv[])
{
    if(argc!=4) return 2;
    const auto base=std::filesystem::current_path()/
        ("segel-core-lauf-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    const auto output=base.string()+".jsonl";
    int status=0;
    try {
#ifdef SEGELPHYSIK_TEST_INSTRUMENTATION
        segelphysik::testmessungen_aktivieren(true);
#endif
        segelphysik::Laufoptionen o;
        o.ergebnis_json_format = segelphysik::ErgebnisJsonFormat::Kompatibel;
        o.jsonl=true;
        bool gueltig=false;
        const auto validierung=segelphysik::pruefung_json(argv[1],gueltig);
        if(!gueltig || validierung.find("\"fehler\":[]")==std::string::npos)
            throw std::runtime_error("Gueltiges Modellpruefergebnis fehlt");
        const auto ungueltig=segelphysik::pruefung_json("nicht-vorhandenes-modells_information.json",gueltig);
        if(gueltig || ungueltig.find("CORE-MOD-0005")==std::string::npos)
            throw std::runtime_error("Strukturierter Modellfehler fehlt");
        const auto modell = segelphysik::lade_modell(argv[1]);
        const auto dynamic_v6_path = base.string() + "-default-v6.jsonl";
        segelphysik::Laufoptionen default_dynamic;
        default_dynamic.dynamisch = true;
        default_dynamic.dauer_s = .01;
        default_dynamic.dt_s = .01;
        const auto default_dynamic_report = segelphysik::simulieren(
            argv[1], dynamic_v6_path, default_dynamic);
        if(default_dynamic_report.status != "abgeschlossen")
            throw std::runtime_error("Dynamischer V6-Defaultlauf fehlgeschlagen");
        std::ifstream dynamic_v6_file(dynamic_v6_path);
        std::string dynamic_v6_line;
        std::getline(dynamic_v6_file, dynamic_v6_line);
        const auto dynamic_v6_json = nlohmann::json::parse(dynamic_v6_line);
        dynamic_v6_file.close();
        std::filesystem::remove(dynamic_v6_path);
        if(dynamic_v6_json.at("vertragsversion") != "6" ||
            dynamic_v6_json.at("kursdrehrate_status") != "gueltig" ||
            !dynamic_v6_json.contains("kursdrehrate_rad_s"))
            throw std::runtime_error("Dynamischer V6-Defaultvertrag unvollstaendig");
        const auto dynamic_force_path=base.string()+"-dynamic-force.jsonl";
        segelphysik::Laufoptionen dynamic_force_run;
        dynamic_force_run.dynamisch=true;
        dynamic_force_run.dauer_s=.01;
        dynamic_force_run.dt_s=.01;
        segelphysik::DynamischeFremdkraftKonfiguration dynamic_force;
        dynamic_force.aktiv=true;
        dynamic_force.kraft_modellfest_n={100.,0.,0.};
        dynamic_force.angriffspunkt_modell_m={0.,.1,0.};
        dynamic_force_run.dynamische_fremdkraft=dynamic_force;
        const auto dynamic_force_report=segelphysik::simulieren(
            argv[1],dynamic_force_path,dynamic_force_run);
        if(dynamic_force_report.status!="abgeschlossen" ||
            dynamic_force_report.zustaende!=2)
            throw std::runtime_error("Dynamischer Fremdkraft-Batchlauf fehlgeschlagen: "+
                dynamic_force_report.diagnose);
        {
            std::ifstream force_file(dynamic_force_path);
            std::string first_force_line,last_force_line;
            std::getline(force_file,first_force_line);
            std::getline(force_file,last_force_line);
            const auto first_force_json=nlohmann::json::parse(first_force_line);
            const auto last_force_json=nlohmann::json::parse(last_force_line);
            if(first_force_json.at("vertragsversion")!="7" ||
                !first_force_json.at("dynamische_fremdkraft").at("aktiv").get<bool>() ||
                last_force_json.at("dynamische_fremdkraft").at("auswertung_schritt")!=1)
                throw std::runtime_error("Dynamischer Fremdkraftkontext fehlt im Batch-Snapshot");
            const auto count=std::count_if(last_force_json.at("vektoren").begin(),
                last_force_json.at("vektoren").end(),[](const auto& contribution) {
                    return contribution.at("id")=="dynamisch.fremdkraft";
                });
            if(count!=1)
                throw std::runtime_error("Batch-Ergebnis muss genau einen Fremdkraftbeitrag enthalten");
            for (const auto* snapshot : {&first_force_json, &last_force_json})
                if(snapshot->at("bilanzstatus")!="vollstaendig" ||
                    !snapshot->at("fehlende_beitraege").empty() ||
                    std::find(snapshot->at("erwartete_beitraege").begin(),
                        snapshot->at("erwartete_beitraege").end(),
                        "dynamisch.fremdkraft")==snapshot->at("erwartete_beitraege").end())
                    throw std::runtime_error("Dynamische Fremdkraft im Bilanzinventar falsch zugeordnet");
            for (const auto* snapshot : {&first_force_json, &last_force_json}) {
                const auto& sail = snapshot->at("druckpunkte").at("ersatz_segel");
                if(sail.at("status")!="gueltig" || sail.at("quelle")!="fremdkraft" ||
                    sail.at("position_modell_m").get<segelphysik::Vektor3>()!=
                        dynamic_force.angriffspunkt_modell_m)
                    throw std::runtime_error("Dynamischer Ersatz-SDP fehlt oder hat falschen Ansatzpunkt");
            }
        }
        std::filesystem::remove(dynamic_force_path);
        {
            const auto original = segelphysik::dynamische_fremdkraft_start(
                modell, segelphysik::Zustand{}, dynamic_force);
            const auto original_json = segelphysik::zustand_json(modell, original, "laufend");
            auto replacement = dynamic_force;
            replacement.kraft_modellfest_n = {25., 10., -2.};
            replacement.angriffspunkt_modell_m = {.3, -.2, 1.};
            const auto changed = segelphysik::dynamische_fremdkraft_aendern(
                modell, original, replacement);
            segelphysik::Modellaufbereitung prepared(modell);
            prepared.auswertung(original);
            const auto next = segelphysik::dynamikschritt(modell, changed, .01);
            const auto prepared_next = prepared.schritt(changed, .01);
            if(prepared.json(prepared_next, "laufend")!=
                segelphysik::zustand_json(modell, next, "laufend"))
                throw std::runtime_error("Kraftwechsel muss Aufbereitung exakt invalidieren");
            const auto unchanged = segelphysik::dynamikschritt(modell, original, .01);
            if(segelphysik::zustand_json(modell, original, "laufend")!=original_json ||
                changed.dynamische_fremdkraft==original.dynamische_fremdkraft ||
                next.dynamische_fremdkraft->konfiguration.kraft_modellfest_n!=replacement.kraft_modellfest_n ||
                next.geschwindigkeit_welt_m_s==unchanged.geschwindigkeit_welt_m_s ||
                next.omega_modell_rad_s==unchanged.omega_modell_rad_s)
                throw std::runtime_error("Laufende Fremdkraftaenderung muss Physik aendern, Historie erhalten");
            auto invalid = replacement;
            invalid.kraft_modellfest_n[0]=std::numeric_limits<double>::quiet_NaN();
            bool rejected=false;
            try { segelphysik::dynamische_fremdkraft_aendern(modell, next, invalid); }
            catch(const std::invalid_argument&) { rejected=true; }
            if(!rejected) throw std::runtime_error("Nichtendliche Fremdkraftaenderung akzeptiert");
        }
        segelphysik::Zustand stationary_start;
        stationary_start.pose = segelphysik::Pose{{3., -4., 1.},
            {0., 0., std::sin(.4), std::cos(.4)}};
        segelphysik::StationaereGleichgewichtKonfiguration stationary_config;
        stationary_config.aktiv = false;
        auto stationary = segelphysik::stationaeres_gleichgewicht(modell,
            stationary_start, stationary_config, segelphysik::HydrodynamikKonfiguration{});
        if(stationary.status != "konvergiert" ||
            stationary.residuum_kraft_welt_n[0] != 0. ||
            stationary.residuum_kraft_welt_n[1] != 0. ||
            stationary.residuum_kraft_welt_n[2] > .05 ||
            stationary.residuum_moment_modell_nm[0] > .05 ||
            stationary.residuum_moment_modell_nm[1] > .05 ||
            stationary.residuum_moment_modell_nm[2] > .05 ||
            stationary.zustand.pose.position_m()[0] != 3. ||
            stationary.zustand.pose.position_m()[1] != -4. ||
            stationary.zustand.geschwindigkeit_welt_m_s[2] != 0. ||
            stationary.zustand.omega_modell_rad_s != segelphysik::Vektor3{} ||
            !stationary.referenzkurs_verwendet)
            throw std::runtime_error("Nullastgleichgewicht mit Referenzkurs unvollstaendig: " +
                stationary.status + " " + stationary.diagnose + " F=" +
                std::to_string(stationary.residuum_kraft_welt_n[0]) + "," +
                std::to_string(stationary.residuum_kraft_welt_n[1]) + "," +
                std::to_string(stationary.residuum_kraft_welt_n[2]) + " M=" +
                std::to_string(stationary.residuum_moment_modell_nm[0]) + "," +
                std::to_string(stationary.residuum_moment_modell_nm[1]) + "," +
                std::to_string(stationary.residuum_moment_modell_nm[2]) + " ref=" +
                std::to_string(stationary.referenzkurs_verwendet));
        const auto stationary_json = nlohmann::json::parse(segelphysik::zustand_json(
            modell, stationary.zustand, stationary.status, stationary.diagnose,
            nullptr, &stationary, segelphysik::ErgebnisJsonFormat::Kompatibel));
        if(stationary_json.at("vertragsversion") != "5" ||
            !stationary_json.at("stationaer").at("konvergiert").get<bool>() ||
            !stationary_json.at("stationaer").at("referenzkurs_verwendet").get<bool>() ||
            stationary_json.at("stationaer").at("flutungswiederholungen") != 0 ||
            stationary_json.at("stationaer").at("residuum_kraft_welt_n").size() != 3 ||
            stationary_json.at("stationaer").at("residuum_moment_modell_nm").size() != 3)
            throw std::runtime_error("Stationaerer v5-Ergebnisvertrag unvollstaendig");
        stationary_config.aktiv = true;
        stationary_config.kraft_welt_n[0] = 10.;
        const auto loaded_stationary = segelphysik::stationaeres_gleichgewicht(modell,
            stationary_start, stationary_config, segelphysik::HydrodynamikKonfiguration{});
        if(loaded_stationary.status != "konvergiert" ||
            std::hypot(loaded_stationary.zustand.geschwindigkeit_welt_m_s[0],
                loaded_stationary.zustand.geschwindigkeit_welt_m_s[1]) <= 0. ||
            std::abs(loaded_stationary.residuum_kraft_welt_n[0]) > .05 ||
            std::abs(loaded_stationary.residuum_kraft_welt_n[1]) > .05 ||
            std::abs(loaded_stationary.residuum_kraft_welt_n[2]) > .05 ||
            std::abs(loaded_stationary.residuum_moment_modell_nm[0]) > .05 ||
            std::abs(loaded_stationary.residuum_moment_modell_nm[1]) > .05 ||
            std::abs(loaded_stationary.residuum_moment_modell_nm[2]) > .05)
            throw std::runtime_error("Horizontale Fremdkraft nicht in allen sechs Residuen ausgeglichen: " +
                loaded_stationary.status + " " + loaded_stationary.diagnose);

        const auto analytic_model = segelphysik::lade_modell(argv[2]);
        segelphysik::Zustand analytic_start;
        segelphysik::StationaereGleichgewichtKonfiguration analytic_config;
        analytic_config.aktiv = true;
        analytic_config.modellfest = false;
        analytic_config.kraft_welt_n = {1500., 0., 0.};
        analytic_config.angriffspunkt_modell_m = {0., 0., -.4};
        const auto analytic_result = segelphysik::stationaeres_gleichgewicht(
            analytic_model, analytic_start, analytic_config,
            segelphysik::HydrodynamikKonfiguration{});
        const double analytic_speed = std::hypot(
            analytic_result.zustand.geschwindigkeit_welt_m_s[0],
            analytic_result.zustand.geschwindigkeit_welt_m_s[1]);
        const double expected_speed = std::sqrt(2. * 1500. /
            (1000. * 1. * 2.4));
        if(analytic_result.status != "konvergiert" ||
            std::abs(analytic_speed - expected_speed) > .1 ||
            std::abs(analytic_result.residuum_kraft_welt_n[0]) > .05 ||
            std::abs(analytic_result.residuum_kraft_welt_n[1]) > .05 ||
            std::abs(analytic_result.residuum_kraft_welt_n[2]) > .05 ||
            std::abs(analytic_result.residuum_moment_modell_nm[0]) > .05 ||
            std::abs(analytic_result.residuum_moment_modell_nm[1]) > .05 ||
            std::abs(analytic_result.residuum_moment_modell_nm[2]) > .05)
            throw std::runtime_error("Analytische F=c*v^2-Referenz verfehlt: " +
                analytic_result.status + " " + analytic_result.diagnose +
                " v=" + std::to_string(analytic_speed) + " F=" +
                std::to_string(analytic_result.residuum_kraft_welt_n[0]) + "," +
                std::to_string(analytic_result.residuum_kraft_welt_n[1]) + "," +
                std::to_string(analytic_result.residuum_kraft_welt_n[2]) + " M=" +
                std::to_string(analytic_result.residuum_moment_modell_nm[0]) + "," +
                std::to_string(analytic_result.residuum_moment_modell_nm[1]) + "," +
                std::to_string(analytic_result.residuum_moment_modell_nm[2]));
        auto max_abs = [](const segelphysik::Vektor3& values) {
            return std::max({std::abs(values[0]), std::abs(values[1]), std::abs(values[2])});
        };
        std::cout << std::setprecision(10)
            << "Stationaer analytisch: v=" << analytic_speed
            << " m/s, max|F|=" << max_abs(analytic_result.residuum_kraft_welt_n)
            << " N, max|M|=" << max_abs(analytic_result.residuum_moment_modell_nm)
            << " Nm, ftol=" << analytic_result.krafttoleranz_n
            << " N, mtol=" << analytic_result.momenttoleranz_nm << " Nm\n";
        auto boundary_config = analytic_config;
        boundary_config.kraft_welt_n = {1e9, 0., 0.};
        const auto boundary_result = segelphysik::stationaeres_gleichgewicht(
            analytic_model, analytic_start, boundary_config,
            segelphysik::HydrodynamikKonfiguration{});
        if (boundary_result.status != "suchgrenze")
            throw std::runtime_error("Endliche Suchgrenze nicht explizit: "+
                boundary_result.status+" "+boundary_result.diagnose);
        segelphysik::Laufoptionen analytic_options;
        analytic_options.ergebnis_json_format = segelphysik::ErgebnisJsonFormat::Kompatibel;
        analytic_options.stationaer = analytic_config;
        analytic_options.hydrodynamik = segelphysik::HydrodynamikKonfiguration{};
        const auto analytic_run = segelphysik::simulieren(argv[2], output, analytic_options);
        if(analytic_run.status != "konvergiert" || analytic_run.zustaende != 1)
            throw std::runtime_error("Analytischer stationaerer JSON-Lauf fehlgeschlagen: " +
                analytic_run.status + " " + analytic_run.diagnose);
        {
            std::ifstream analytic_file(output);
            std::string analytic_line;
            std::getline(analytic_file, analytic_line);
            const auto json = nlohmann::json::parse(analytic_line);
            const double speed = std::hypot(
                json.at("geschwindigkeit_welt_m_s")[0].get<double>(),
                json.at("geschwindigkeit_welt_m_s")[1].get<double>());
            if(json.at("vertragsversion") != "5" ||
                !json.at("stationaer").at("konvergiert").get<bool>() ||
                std::abs(speed - expected_speed) > .1 ||
                json.at("stationaer").at("kraft_bezugssystem") != "welt" ||
                json.at("stationaer").at("residuum_moment_modell_nm").size() != 3)
                throw std::runtime_error("Analytische GUI-Integrationsfixture verfehlt Ergebnisvertrag");
            const auto force = std::find_if(json.at("vektoren").begin(),
                json.at("vektoren").end(), [](const nlohmann::json& item) {
                    return item.at("id") == "stationaer.fremdkraft";
                });
            if (force == json.at("vektoren").end())
                throw std::runtime_error("Exzentrische stationaere Fremdkraft fehlt im Beitragsvertrag");
            const auto point = force->at("punkt_m");
            const auto cg = json.at("moment_bezug_modell_m");
            const auto f = force->at("vektor");
            const auto moment = force->at("moment_s_nm");
            const double rx = point[0].get<double>() - cg[0].get<double>();
            const double ry = point[1].get<double>() - cg[1].get<double>();
            const double rz = point[2].get<double>() - cg[2].get<double>();
            const double expected_my = rz*f[0].get<double>()-rx*f[2].get<double>();
            if (std::abs(expected_my) < 1. ||
                std::abs(moment[0].get<double>() - (ry*f[2].get<double>()-rz*f[1].get<double>())) > 1e-8 ||
                std::abs(moment[1].get<double>() - expected_my) > 1e-8 ||
                std::abs(moment[2].get<double>() - (rx*f[1].get<double>()-ry*f[0].get<double>())) > 1e-8)
                throw std::runtime_error("Moment der exzentrischen stationaeren Fremdkraft weicht vom Hebelarm ab");
            std::cout << std::setprecision(10) << "Exzentrische Kraft: Moment um S=["
                << moment[0].get<double>() << ',' << moment[1].get<double>() << ','
                << moment[2].get<double>() << "] Nm\n";
        }
        std::filesystem::remove(output);

        auto shifted_hydro = segelphysik::HydrodynamikKonfiguration{};
        shifted_hydro.fluidgeschwindigkeit_welt_m_s = {3., -2., 0.};
        auto shifted_start = analytic_start;
        shifted_start.geschwindigkeit_welt_m_s = {3., -2., 0.};
        const auto galilei_result = segelphysik::stationaeres_gleichgewicht(
            analytic_model, shifted_start, analytic_config, shifted_hydro);
        const auto relative_base = analytic_result.zustand.geschwindigkeit_welt_m_s;
        const auto relative_shifted = galilei_result.zustand.geschwindigkeit_welt_m_s;
        if (galilei_result.status != "konvergiert" ||
            std::abs((relative_shifted[0]-3.)-relative_base[0]) > 1e-4 ||
            std::abs((relative_shifted[1]+2.)-relative_base[1]) > 1e-4 ||
            std::abs(relative_shifted[2]) > 1e-12 ||
            std::abs(galilei_result.residuum_kraft_welt_n[0]) > .05 ||
            std::abs(galilei_result.residuum_kraft_welt_n[1]) > .05 ||
            std::abs(galilei_result.residuum_kraft_welt_n[2]) > .05 ||
            std::abs(galilei_result.residuum_moment_modell_nm[0]) > .05 ||
            std::abs(galilei_result.residuum_moment_modell_nm[1]) > .05 ||
            std::abs(galilei_result.residuum_moment_modell_nm[2]) > .05)
            throw std::runtime_error("Galilei-verschobener stationaerer Lauf veraendert Relativloesung: "+
                galilei_result.status+" "+galilei_result.diagnose);
        std::cout << std::setprecision(10)
            << "Galilei: delta Relativgeschwindigkeit="
            << std::hypot((relative_shifted[0]-3.)-relative_base[0],
                (relative_shifted[1]+2.)-relative_base[1]) << " m/s\n";
        const auto base_state_json = nlohmann::json::parse(segelphysik::zustand_json(
            analytic_model, analytic_result.zustand, analytic_result.status,
            analytic_result.diagnose, nullptr, &analytic_result));
        const auto shifted_state_json = nlohmann::json::parse(segelphysik::zustand_json(
            analytic_model, galilei_result.zustand, galilei_result.status,
            galilei_result.diagnose, nullptr, &galilei_result));
        std::size_t hydrodynamic_loads_compared = 0;
        for (const auto& base_contribution : base_state_json.at("vektoren")) {
            if (base_contribution.at("quelle") != "hydrodynamik") continue;
            const auto shifted_contribution = std::find_if(
                shifted_state_json.at("vektoren").begin(),
                shifted_state_json.at("vektoren").end(),
                [&](const nlohmann::json& item) {
                    return item.at("id") == base_contribution.at("id");
                });
            if (shifted_contribution == shifted_state_json.at("vektoren").end())
                throw std::runtime_error("Galilei-Vergleich verlor einen Hydrodynamikbeitrag");
            for (std::size_t i = 0; i < 3; ++i)
                if (std::abs(base_contribution.at("vektor")[i].get<double>() -
                    shifted_contribution->at("vektor")[i].get<double>()) > .05 ||
                    std::abs(base_contribution.at("moment_s_nm")[i].get<double>() -
                    shifted_contribution->at("moment_s_nm")[i].get<double>()) > .05)
                    throw std::runtime_error("Gleiche relative Anstroemung veraendert hydrodynamische Last");
            ++hydrodynamic_loads_compared;
        }
        if (hydrodynamic_loads_compared == 0)
            throw std::runtime_error("Galilei-Test vergleicht keine Hydrodynamiklast");
        auto model_fixed_config = analytic_config;
        model_fixed_config.modellfest = true;
        auto model_fixed_start = analytic_start;
        model_fixed_start.pose = segelphysik::Pose{{}, {0., 0.,
            std::sin(3.14159265358979323846/4.), std::cos(3.14159265358979323846/4.)}};
        const auto model_fixed_result = segelphysik::stationaeres_gleichgewicht(
            analytic_model, model_fixed_start, model_fixed_config,
            segelphysik::HydrodynamikKonfiguration{});
        if (model_fixed_result.status != "konvergiert" ||
            std::abs(model_fixed_result.zustand.geschwindigkeit_welt_m_s[0] +
                analytic_result.zustand.geschwindigkeit_welt_m_s[1]) > .1 ||
            std::abs(model_fixed_result.zustand.geschwindigkeit_welt_m_s[1] -
                analytic_result.zustand.geschwindigkeit_welt_m_s[0]) > .1 ||
            std::abs(model_fixed_result.residuum_kraft_welt_n[0]) > .05 ||
            std::abs(model_fixed_result.residuum_kraft_welt_n[1]) > .05 ||
            std::abs(model_fixed_result.residuum_kraft_welt_n[2]) > .05 ||
            std::abs(model_fixed_result.residuum_moment_modell_nm[0]) > .05 ||
            std::abs(model_fixed_result.residuum_moment_modell_nm[1]) > .05 ||
            std::abs(model_fixed_result.residuum_moment_modell_nm[2]) > .05)
            throw std::runtime_error("Modellfeste Kraft folgt der gedrehten Pose nicht: "+
                model_fixed_result.status+" "+model_fixed_result.diagnose);

        segelphysik::StationaeresGleichgewichtErgebnis frame_result = analytic_result;
        frame_result.konfiguration.modellfest = true;
        frame_result.konfiguration.kraft_welt_n = {1500., 0., 0.};
        frame_result.konfiguration.angriffspunkt_modell_m = {0., 0., -.4};
        frame_result.zustand.pose = segelphysik::Pose{{}, {0., 0.,
            std::sin(3.14159265358979323846/4.), std::cos(3.14159265358979323846/4.)}};
        frame_result.schwerpunkt_modell_m = analytic_model.schwerpunkt_m;
        const auto frame_json = nlohmann::json::parse(segelphysik::zustand_json(
            analytic_model, frame_result.zustand, "nicht_konvergiert", frame_result.diagnose,
            nullptr, &frame_result));
        const auto& frame_contributions = frame_json.at("vektoren");
        const auto frame_force = std::find_if(frame_contributions.begin(),
            frame_contributions.end(), [](const nlohmann::json& item) {
                return item.at("id") == "stationaer.fremdkraft";
            });
        if (frame_force == frame_contributions.end() ||
            frame_json.at("stationaer").at("kraft_bezugssystem") != "modell" ||
            std::abs(frame_force->at("vektor")[0].get<double>()-1500.) > 1e-8 ||
            std::abs(frame_force->at("vektor")[1].get<double>()) > 1e-8 ||
            std::abs(frame_force->at("moment_s_nm")[1].get<double>()-
                ((-.4-analytic_model.schwerpunkt_m[2])*1500.)) > 1e-7)
            throw std::runtime_error("Modellfeste Kraft-/Hebelarmtransformation weicht von Referenz ab");

        const auto bewegungsmodell = segelphysik::lade_modell(argv[3]);
        segelphysik::StationaereGleichgewichtKonfiguration multi_setting;
        multi_setting.stellungen = {{1, "Ruder", 20.}, {2, "Schwert", 50.}};
        const auto multi_setting_result = segelphysik::stationaeres_gleichgewicht(
            bewegungsmodell, segelphysik::Zustand{}, multi_setting,
            segelphysik::HydrodynamikKonfiguration{});
        const auto neutral_setting_result = segelphysik::stationaeres_gleichgewicht(
            bewegungsmodell, segelphysik::Zustand{},
            segelphysik::StationaereGleichgewichtKonfiguration{},
            segelphysik::HydrodynamikKonfiguration{});
        for (const auto& expected : multi_setting.stellungen) {
            const auto found = std::find_if(multi_setting_result.zustand.bewegungsstellungen.begin(),
                multi_setting_result.zustand.bewegungsstellungen.end(),
                [&](const segelphysik::Bewegungsstellung& value) {
                    return value.element_id == expected.element_id;
                });
            if (found == multi_setting_result.zustand.bewegungsstellungen.end() ||
                found->wert != expected.wert)
                throw std::runtime_error("Stationaerer Solver hat mehrere feste Stellungen nicht uebernommen");
        }
        const auto rudder_neutral = segelphysik::bewegungstransformation(
            bewegungsmodell, "Ruder", {{"Ruder", 0.}});
        const auto rudder_fixed = segelphysik::bewegungstransformation(
            bewegungsmodell, "Ruder", {{"Ruder", 20.}});
        const auto board_neutral = segelphysik::bewegungstransformation(
            bewegungsmodell, "Schwert", {{"Schwert", 100.}});
        const auto board_fixed = segelphysik::bewegungstransformation(
            bewegungsmodell, "Schwert", {{"Schwert", 50.}});
        if (rudder_neutral == rudder_fixed || board_neutral == board_fixed)
            throw std::runtime_error("Stationaere Ruder-/Schwertstellungen veraendern die Core-Geometrie nicht");
        if (multi_setting_result.schwerpunkt_modell_m ==
            neutral_setting_result.schwerpunkt_modell_m)
            throw std::runtime_error("Stationaere Mehrfachstellung transformiert den physischen Gesamtschwerpunkt nicht");
        if (multi_setting_result.status == "ungueltig" ||
            multi_setting_result.status == "konvergiert" &&
            (std::abs(multi_setting_result.residuum_kraft_welt_n[0]) > .1 ||
             std::abs(multi_setting_result.residuum_kraft_welt_n[1]) > .1 ||
             std::abs(multi_setting_result.residuum_kraft_welt_n[2]) > .1 ||
             std::abs(multi_setting_result.residuum_moment_modell_nm[0]) > .1 ||
             std::abs(multi_setting_result.residuum_moment_modell_nm[1]) > .1 ||
             std::abs(multi_setting_result.residuum_moment_modell_nm[2]) > .1))
            throw std::runtime_error("Stationaere Mehrfachstellung meldet ungueltigen Erfolg");
        std::cout << "Mehrfachstellungen 420: status=" << multi_setting_result.status
            << ", iter=" << multi_setting_result.iterationen << '\n';
        const auto bewegungspose = segelphysik::Pose{{0., 0., 0.}, {0., 0., 0., 1.}};
        const auto bewegbare_kraefte_0 = segelphysik::kraefte(bewegungsmodell,
            bewegungspose, nullptr, 1000., 9.81, {}, nullptr, {}, {},
            {{"Schwert", 0.}});
        const auto bewegbare_kraefte_100 = segelphysik::kraefte(bewegungsmodell,
            bewegungspose, nullptr, 1000., 9.81, {}, nullptr, {}, {},
            {{"Schwert", 100.}});
        const auto gewicht_0 = std::find_if(bewegbare_kraefte_0.beitraege.begin(),
            bewegbare_kraefte_0.beitraege.end(),
            [](const segelphysik::Beitrag& value) { return value.id == "Schwert:gewicht"; });
        const auto gewicht_100 = std::find_if(bewegbare_kraefte_100.beitraege.begin(),
            bewegbare_kraefte_100.beitraege.end(),
            [](const segelphysik::Beitrag& value) { return value.id == "Schwert:gewicht"; });
        if(gewicht_0 == bewegbare_kraefte_0.beitraege.end() ||
            gewicht_100 == bewegbare_kraefte_100.beitraege.end() ||
            gewicht_0->punkt_m == gewicht_100->punkt_m)
            throw std::runtime_error("Feste Schwertstellung transformiert die physische Masse nicht");
        stationary_config.kraft_welt_n = {};
        stationary_config.kraft_welt_n[2] = 1e6;
        const auto unbalanced = segelphysik::stationaeres_gleichgewicht(modell,
            stationary_start, stationary_config, segelphysik::HydrodynamikKonfiguration{});
        if(unbalanced.status == "konvergiert")
            throw std::runtime_error("Nicht ausgeglichene Fremdkraft wurde als konvergiert gemeldet");
        stationary_config.kraft_welt_n[2] = std::numeric_limits<double>::quiet_NaN();
        const auto invalid_stationary = segelphysik::stationaeres_gleichgewicht(modell,
            stationary_start, stationary_config, segelphysik::HydrodynamikKonfiguration{});
        if(invalid_stationary.status != "ungueltig" ||
            invalid_stationary.diagnose.find("CORE-SIM-0001") == std::string::npos)
            throw std::runtime_error("Nicht endliche stationaere Fremdkraft nicht abgelehnt");
        std::atomic_bool stationary_stop{true};
        stationary_config.kraft_welt_n = {};
        const auto cancelled_stationary = segelphysik::stationaeres_gleichgewicht(modell,
            stationary_start, stationary_config, segelphysik::HydrodynamikKonfiguration{},
            .05, .05, 100, &stationary_stop);
        if(cancelled_stationary.status != "abgebrochen")
            throw std::runtime_error("Stationaerer Abbruchstatus fehlt");
        stationary_config.aktiv = false;
        o.stationaer = stationary_config;
        auto stationary_run = segelphysik::simulieren(argv[1], output, o);
        if(stationary_run.status != "konvergiert" || stationary_run.zustaende != 1)
            throw std::runtime_error("Stationaerer Laufpfad fehlgeschlagen: " +
                stationary_run.status + " " + stationary_run.diagnose);
        {
            std::ifstream stationary_file(output);
            std::string stationary_line;
            std::getline(stationary_file, stationary_line);
            const auto json = nlohmann::json::parse(stationary_line);
            if(json.at("vertragsversion") != "5" ||
                !json.contains("stationaer") ||
                json.at("stationaer").at("aktiv").get<bool>() ||
                json.at("stationaer").at("status") != "konvergiert")
                throw std::runtime_error("Laufoptionen stationaer erzeugt keinen expliziten v5-Kontext");
        }
        std::filesystem::remove(output);
        o.stationaer.reset();
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
        r=segelphysik::simulieren("nicht-vorhandenes-modells_information.json",output,o);
        if(r.status!="fehlgeschlagen"||r.diagnose.empty()||r.gesamtlaufzeit_s<0.)
            throw std::runtime_error("Fehlerlauf nicht gemessen");
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';status=1;}
    std::filesystem::remove(output);
    return status;
}
