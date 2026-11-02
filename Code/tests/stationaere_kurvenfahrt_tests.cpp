#include "segelphysik/simulation.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <iostream>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace {
using namespace segelphysik;
void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}
Vektor3 plus(const Vektor3& a, const Vektor3& b)
{
    return {a[0]+b[0],a[1]+b[1],a[2]+b[2]};
}
Vektor3 minus(const Vektor3& a, const Vektor3& b)
{
    return {a[0]-b[0],a[1]-b[1],a[2]-b[2]};
}
Vektor3 scale(const Vektor3& a, double v)
{
    return {a[0]*v,a[1]*v,a[2]*v};
}
double dot(const Vektor3& a, const Vektor3& b)
{
    return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
}
Vektor3 cross(const Vektor3& a, const Vektor3& b)
{
    return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};
}
QuaternionXyzw multiply(const QuaternionXyzw& a, const QuaternionXyzw& b)
{
    return {a[3]*b[0]+a[0]*b[3]+a[1]*b[2]-a[2]*b[1],
        a[3]*b[1]-a[0]*b[2]+a[1]*b[3]+a[2]*b[0],
        a[3]*b[2]+a[0]*b[1]-a[1]*b[0]+a[2]*b[3],
        a[3]*b[3]-a[0]*b[0]-a[1]*b[1]-a[2]*b[2]};
}
Pose pose(double roll, double pitch, double yaw)
{
    const QuaternionXyzw qx{std::sin(roll/2.),0.,0.,std::cos(roll/2.)};
    const QuaternionXyzw qy{0.,std::sin(pitch/2.),0.,std::cos(pitch/2.)};
    const QuaternionXyzw qz{0.,0.,std::sin(yaw/2.),std::cos(yaw/2.)};
    return {{0.,0.,0.},multiply(multiply(qz,qy),qx)};
}
Mesh box()
{
    return {{{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},
             {-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}},
            {{0,2,1},{0,3,2},{4,5,6},{4,6,7},{0,1,5},{0,5,4},
             {1,2,6},{1,6,5},{2,3,7},{2,7,6},{3,0,4},{3,4,7}}};
}
Modell test_model(bool added_mass)
{
    Modell m;
    m.id="steady-turning-reference";
    m.fassung="steady-turning-v1";
    m.masse_kg=4000.;
    m.schwerpunkt_m={0.,0.,0.};
    m.traegheit_kg_m2={5000.,0.,0.,0.,7000.,0.,0.,0.,9000.};
    Element hull;
    hull.id="hull";
    hull.masse_kg=m.masse_kg;
    hull.schwerpunkt_m=m.schwerpunkt_m;
    hull.traegheit_kg_m2=m.traegheit_kg_m2;
    hull.volumen=box();
    m.elemente.push_back(hull);
    if (added_mass) {
        HydrodynamischeTraegheitModell values;
        values.zusatzmasse={{"a11",300.},{"a22",500.},{"a33",700.},
            {"a44",800.},{"a55",900.},{"a66",1100.}};
        values.daempfung={{"zeta_rollen",0.},{"zeta_stampfen",0.},
            {"zeta_tauchen",0.},{"gieren_linear",0.},{"gieren_quadratisch",0.}};
        m.hydrodynamische_traegheit=values;
    }
    return m;
}
StationaereGleichgewichtKonfiguration matched_load(const Modell& model,
    const Pose& attitude, double speed, double rate, const HydrodynamikKonfiguration& config)
{
    const Vektor3 velocity_body{speed,0.,0.};
    const Vektor3 velocity_world=attitude.modellvektor_nach_welt(velocity_body);
    const Vektor3 omega_world{0.,0.,rate};
    const Vektor3 omega_body=attitude.weltvektor_nach_modell(omega_world);
    HydrodynamikKontext context;
    context.konfiguration=config;
    context.modell_fassung=model.fassung;
    if (config.zusatzmasse_daempfung)
        context.traegheit=std::make_shared<const HydrodynamischeTraegheit>(
            berechne_hydrodynamische_traegheit(model,attitude,config.rho_kg_m3,9.81));
    const auto internal=kraefte(model,attitude,nullptr,config.rho_kg_m3,9.81,{},
        &context,velocity_world,omega_body);
    const auto acceleration_world=cross(omega_world,velocity_world);
    Vektor3 added_momentum{};
    std::array<double,9> inertia=model.traegheit_kg_m2;
    if (context.traegheit) {
        for (std::size_t i=0;i<3;++i) {
            added_momentum[i]=context.traegheit->zusatzmasse[i].wert*velocity_body[i];
            inertia[3*i+i]+=context.traegheit->zusatzmasse[3+i].wert;
        }
    }
    const Vektor3 added_force_world=attitude.modellvektor_nach_welt(
        cross(omega_body,added_momentum));
    const Vektor3 desired_force_world=minus(
        plus(scale(acceleration_world,model.masse_kg),added_force_world),internal.kraft_welt_n);
    const Vektor3 h{
        inertia[0]*omega_body[0]+inertia[1]*omega_body[1]+inertia[2]*omega_body[2],
        inertia[3]*omega_body[0]+inertia[4]*omega_body[1]+inertia[5]*omega_body[2],
        inertia[6]*omega_body[0]+inertia[7]*omega_body[1]+inertia[8]*omega_body[2]};
    const Vektor3 desired_moment=minus(cross(omega_body,h),internal.moment_modell_nm);
    const Vektor3 force_body=attitude.weltvektor_nach_modell(desired_force_world);
    const double force_squared=dot(force_body,force_body);
    require(force_squared>1e-8,"analytic forcing is degenerate");
    require(std::abs(dot(force_body,desired_moment))<1e-5,
        "single-force moment target must be perpendicular to force");
    const Vektor3 point_offset=scale(cross(force_body,desired_moment),1./force_squared);
    StationaereGleichgewichtKonfiguration result;
    result.aktiv=true;
    result.modellfest=true;
    result.kraft_welt_n=force_body;
    result.angriffspunkt_modell_m=plus(model.schwerpunkt_m,point_offset);
    result.kurvenfahrt=true;
    return result;
}
void test_curved_solution(double rate, double yaw, bool added_mass)
{
    const auto model=test_model(added_mass);
    const auto attitude=pose(.2,0.,yaw);
    HydrodynamikKonfiguration hydro;
    hydro.zusatzmasse_daempfung=added_mass;
    auto config=matched_load(model,attitude,2.,rate,hydro);
    Zustand start;
    start.pose=attitude;
    start.geschwindigkeit_welt_m_s=attitude.modellvektor_nach_welt({2.,0.,0.});
    start.omega_modell_rad_s=attitude.weltvektor_nach_modell({0.,0.,rate});
    const auto result=stationaeres_gleichgewicht(model,start,config,hydro,.05,.05,30);
    require(result.status=="konvergiert",result.diagnose.c_str());
    require(result.kurvenfahrt && result.konfiguration.kurvenfahrt,"turning mode lost");
    require(std::abs(result.kursdrehrate_rad_s-rate)<1e-10,"signed course rate changed");
    require(result.loesungstyp=="kurvenfahrt","wrong solution type");
    require(result.radius_m && std::abs(*result.radius_m-5.)<1e-9,"water-relative radius incorrect");
    const double speed=std::hypot(result.zustand.geschwindigkeit_welt_m_s[0],
        result.zustand.geschwindigkeit_welt_m_s[1]);
    require(std::abs(speed-2.)<=.1,"water-relative speed outside reference tolerance");
    const auto& expected_orientation=attitude.orientierung_xyzw();
    const auto& actual_orientation=result.zustand.pose.orientierung_xyzw();
    double orientation_dot=0.;
    for (std::size_t i=0;i<4;++i)
        orientation_dot+=expected_orientation[i]*actual_orientation[i];
    const double angle_error=2.*std::acos(std::clamp(std::abs(orientation_dot),0.,1.));
    require(angle_error<=.1*3.14159265358979323846/180.,
        "roll/pitch reference outside angular tolerance");
    require(std::abs(result.zustand.geschwindigkeit_welt_m_s[2])<=.001,
        "centre-of-mass vertical speed outside tolerance");
    const double roll_rate=result.omega_modell_rad_s[0];
    const double pitch_rate=result.omega_modell_rad_s[1]*
            std::cos(.2)-result.omega_modell_rad_s[2]*std::sin(.2);
    require(std::abs(roll_rate)<=.01*3.14159265358979323846/180. &&
        std::abs(pitch_rate)<=.01*3.14159265358979323846/180.,
        "roll/pitch rates outside angular-rate tolerance");
    for (double value:result.residuum_bewegung_kraft_welt_n)
        require(std::abs(value)<=.05,"force motion residual outside tolerance");
    for (double value:result.residuum_bewegung_moment_modell_nm)
        require(std::abs(value)<=.05,"moment motion residual outside tolerance");
    require(std::abs(result.omega_modell_rad_s[1])>1e-3,
        "tilted course turn must not be copied to model Z component");
}
void test_json_contract()
{
    Modell model;
    model.id="json-rate";
    model.fassung="json-rate-v1";
    model.masse_kg=1.;
    Element mass;
    mass.id="mass";
    mass.masse_kg=1.;
    model.elemente.push_back(mass);
    Zustand state;
    state.pose=pose(.3,.4,.7);
    state.omega_modell_rad_s=state.pose.weltvektor_nach_modell({0.,0.,2.});
    const auto dynamic=nlohmann::json::parse(zustand_json(model,state,"laufend",{},nullptr,nullptr,
        ErgebnisJsonFormat::V6));
    require(dynamic.at("vertragsversion")=="6","V6 contract version missing");
    require(dynamic.at("kursdrehrate_status")=="gueltig","dynamic course rate not valid");
    require(std::abs(dynamic.at("kursdrehrate_rad_s").get<double>()-2.)<1e-12,
        "tilted positive RHR heading rate incorrect");
    const auto default_format=nlohmann::json::parse(zustand_json(model,state,"laufend"));
    require(default_format.at("vertragsversion")=="6" &&
        default_format.at("kursdrehrate_status")=="gueltig",
        "default result format is not V6");
    const auto old=nlohmann::json::parse(zustand_json(model,state,"laufend",{},nullptr,nullptr,
        ErgebnisJsonFormat::Kompatibel));
    require(old.at("vertragsversion")=="2" && !old.contains("kursdrehrate_rad_s"),
        "legacy JSON projection changed");
    state.pose=pose(0.,3.14159265358979323846/2.,0.);
    const auto singular=nlohmann::json::parse(zustand_json(model,state,"laufend",{},nullptr,nullptr,
        ErgebnisJsonFormat::V6));
    require(singular.at("kursdrehrate_rad_s").is_null() &&
        singular.at("kursdrehrate_status")=="nicht_bestimmbar",
        "vertical forward axis must report undefined heading rate");
    StationaeresGleichgewichtErgebnis stationary;
    stationary.zustand=state;
    stationary.zustand=state;
    stationary.status="konvergiert";
    stationary.kurvenfahrt=true;
    stationary.konfiguration.kurvenfahrt=true;
    stationary.loesungstyp="reine_drehung";
    stationary.kursdrehrate_rad_s=.5;
    stationary.aeussere_kraft_welt_n={11.,12.,13.};
    stationary.aeusseres_moment_modell_nm={21.,22.,23.};
    stationary.residuum_bewegung_kraft_welt_n={.01,.02,.03};
    stationary.residuum_bewegung_moment_modell_nm={.04,.05,.06};
    stationary.radius_m.reset();
    const auto v6=nlohmann::json::parse(zustand_json(model,state,"konvergiert",{},nullptr,
        &stationary,ErgebnisJsonFormat::V6));
    const auto& json_stationary=v6.at("stationaer");
    require(json_stationary.at("kurvenfahrt").get<bool>() &&
        json_stationary.at("loesungstyp")=="reine_drehung" &&
        json_stationary.at("radius_m").is_null() &&
        json_stationary.at("aeussere_kraft_welt_n")[0]==11. &&
        json_stationary.at("residuum_kraft_bewegung_welt_n")[0]==.01,
        "V6 stationary result contract incomplete");
    bool refused=false;
    try { (void)zustand_json(model,state,"konvergiert",{},nullptr,&stationary,
        ErgebnisJsonFormat::Kompatibel); }
    catch (const std::invalid_argument&) { refused=true; }
    require(refused,"legacy V5 must not silently encode a turning result");
}
void test_failures()
{
    const auto model=test_model(false);
    HydrodynamikKonfiguration hydro;
    hydro.zusatzmasse_daempfung=false;
    const auto attitude=pose(.2,0.,0.);
    auto config=matched_load(model,attitude,2.,.4,hydro);
    Zustand start;
    start.pose=attitude;
    start.geschwindigkeit_welt_m_s=attitude.modellvektor_nach_welt({2.,0.,0.});
    start.omega_modell_rad_s=attitude.weltvektor_nach_modell({0.,0.,.4});
    std::atomic_bool stop{true};
    const auto aborted=stationaeres_gleichgewicht(model,start,config,hydro,.05,.05,30,&stop);
    require(aborted.status=="abgebrochen","curve solver ignored cancellation");
    auto world_force=config;
    world_force.modellfest=false;
    const auto incompatible=stationaeres_gleichgewicht(model,start,world_force,hydro,.05,.05,3);
    require(incompatible.status=="last_invariant","non-invariant world force not explicitly rejected");
    auto incorrect_start=start;
    incorrect_start.geschwindigkeit_welt_m_s=attitude.modellvektor_nach_welt({1.,0.,0.});
    const auto singular=stationaeres_gleichgewicht(model,incorrect_start,config,hydro,.05,.05,1);
    require(singular.status=="rangdefizit",
        "singular turning residual map must not report convergence");
    auto flood_model=model;
    flood_model.elemente.front().flutrand_m={{0.,0.,-.5}};
    const auto flood_config=matched_load(flood_model,attitude,2.,.4,hydro);
    const auto flooded=stationaeres_gleichgewicht(flood_model,start,flood_config,hydro,.05,.05,30);
    require(flooded.flutungswiederholungen==1 &&
        std::find(flooded.zustand.geflutet.begin(),flooded.zustand.geflutet.end(),"hull")!=
            flooded.zustand.geflutet.end() &&
        flooded.status!="konvergiert",
        "flooding retry must be reported without inventing a solution");
    config.kraft_welt_n[0]=std::numeric_limits<double>::quiet_NaN();
    const auto invalid=stationaeres_gleichgewicht(model,start,config,hydro,.05,.05,3);
    require(invalid.status=="ungueltig","non-finite curve input not rejected");
}
}
int main(int argc, char* argv[])
{
    try {
        test_curved_solution(.4,0.,false);
        test_curved_solution(-.4,0.,false);
        test_curved_solution(.4,.7,true);
        test_json_contract();
        test_failures();
        require(argc==2 || argc==3,"persistent turning model fixture path missing");
        const auto imported=lade_modell(argv[1]);
        const auto reference=test_model(false);
        require(std::abs(imported.masse_kg-reference.masse_kg)<1e-10,
            "imported fixture mass differs");
        for(std::size_t i=0;i<9;++i)
            require(std::abs(imported.traegheit_kg_m2[i]-reference.traegheit_kg_m2[i])<1e-9,
                "imported fixture inertia differs");
        HydrodynamikKonfiguration hydro;
        const auto fixture_directory=std::filesystem::path(argv[1]).parent_path();
        std::ifstream case_file(fixture_directory/"kreisfall.json");
        const auto fixture=nlohmann::json::parse(case_file);
        StationaereGleichgewichtKonfiguration load;
        const auto& input=fixture.at("stationaer");
        load.aktiv=input.at("aktiv");
        load.modellfest=input.at("modellfest");
        load.kurvenfahrt=input.at("kurvenfahrt");
        load.kraft_welt_n=input.at("kraft_welt_n").get<Vektor3>();
        load.angriffspunkt_modell_m=input.at("angriffspunkt_modell_m").get<Vektor3>();
        Zustand start;
        const auto& initial=fixture.at("start");
        start.pose=Pose{initial.at("position_m").get<Vektor3>(),
            initial.at("orientierung_xyzw").get<QuaternionXyzw>()};
        start.geschwindigkeit_welt_m_s=initial.at("geschwindigkeit_welt_m_s").get<Vektor3>();
        start.omega_modell_rad_s=initial.at("omega_modell_rad_s").get<Vektor3>();
        const auto result=stationaeres_gleichgewicht(imported,start,load,hydro,.05,.05,30);
        require(result.status=="konvergiert",result.diagnose.c_str());
        require(std::abs(result.kursdrehrate_rad_s-.4)<1e-10 &&
            result.radius_m && std::abs(*result.radius_m-5.)<1e-9,
            "persistent fixture circle reference changed");
        const std::filesystem::path output="stationaere-kurvenreferenz.jsonl";
        Laufoptionen options;
        options.start=start;
        options.stationaer=load;
        const auto report=simulieren(argv[1],output,options);
        require(report.status=="konvergiert",report.diagnose.c_str());
        std::ifstream result_file(output);
        std::string line;
        std::getline(result_file,line);
        const auto json=nlohmann::json::parse(line);
        result_file.close();
        std::filesystem::remove(output);
        require(json.at("vertragsversion")=="6" &&
            json.at("stationaer").at("loesungstyp")=="kurvenfahrt" &&
            std::abs(json.at("kursdrehrate_rad_s").get<double>()-.4)<1e-10,
            "persistent fixture simulation V6 result changed");
        if(argc==3) {
            const auto real_model=lade_modell(argv[2]);
            for(double angle:{0.,1.,-1.,12.,-12.,24.,-24.,36.,-36.}) {
                StationaereGleichgewichtKonfiguration config;
                config.aktiv=true;
                config.modellfest=true;
                config.kurvenfahrt=true;
                config.kraft_welt_n={100.,0.,0.};
                config.angriffspunkt_modell_m={1.,0.,0.};
                if(std::abs(angle)>1.) {
                    config.kraft_welt_n={150.,10.,0.};
                    config.angriffspunkt_modell_m={-.05,0.,3.};
                }
                std::size_t id=1;
                for(const auto& element:real_model.elemente) if(element.bewegung)
                    config.stellungen.push_back({id++,element.id,
                        element.bewegung->ruderwinkel?angle:100.});
                const auto before=std::chrono::steady_clock::now();
                const auto real=stationaeres_gleichgewicht(real_model,Zustand{},
                    config,HydrodynamikKonfiguration{},.05,.05,100);
                const double seconds=std::chrono::duration<double>(
                    std::chrono::steady_clock::now()-before).count();
                std::cout<<"420 "<<angle<<" deg: "<<real.status<<", r="
                    <<real.kursdrehrate_rad_s<<" rad/s, iterations="<<real.iterationen
                    <<", seconds="<<seconds<<", F=";
                for(double value:real.residuum_bewegung_kraft_welt_n) std::cout<<value<<",";
                std::cout<<" M=";
                for(double value:real.residuum_bewegung_moment_modell_nm) std::cout<<value<<",";
                std::cout<<" "<<real.diagnose<<'\n';
                require(real.status=="konvergiert","420 fixed-rudder turning case did not converge");
                for(double value:real.residuum_bewegung_kraft_welt_n)
                    require(std::isfinite(value) && std::abs(value)<=.05,"420 force residual outside tolerance");
                for(double value:real.residuum_bewegung_moment_modell_nm)
                    require(std::isfinite(value) && std::abs(value)<=.05,"420 moment residual outside tolerance");
                require(real.iterationen<30,"420 turning iteration regression");
                if(std::abs(angle)>1.)
                    require(real.kursdrehrate_rad_s*angle<0.,"large-rudder turning branch has wrong sign");
            }
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
