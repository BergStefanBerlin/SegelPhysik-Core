#include "segelphysik/aufbereitung.hpp"
#include <chrono>
#include <cstdint>
#include <iostream>

using namespace segelphysik;
using Uhr=std::chrono::steady_clock;
double ms(Uhr::time_point begin) {return std::chrono::duration<double,std::milli>(Uhr::now()-begin).count();}
int main(int argc,char** argv)
{
    if(argc!=5) return 2;
    const std::string fall=argv[2];
    const int schritte=std::stoi(argv[3]);
    const bool ereignisse=std::string(argv[4])=="events";
    for(int rep=-1;rep<5;++rep) {
        nlohmann::json bericht={{"scenario",fall},{"steps",schritte},
            {"dt_s",.01},{"settings",ereignisse?"events":"fixed"},{"repeat",rep}};
        const auto gesamt=Uhr::now();
        try {
            auto begin=Uhr::now();
            const auto quelle=lade_modell(argv[1]);
            bericht["model"]=quelle.id;
            bericht["fassung"]=quelle.fassung;
            bericht["load_ms"]=ms(begin);
            begin=Uhr::now();
#ifndef CORE_PERFORMANCE_BASELINE
            Modellaufbereitung cache(quelle);
            const auto& m=cache.modell();
#else
            const auto& m=quelle;
#endif
            Zustand s;
            const bool hydro=fall.find("hydro")!=std::string::npos;
            const bool drive=fall.find("drive")!=std::string::npos;
            if(hydro) s=hydrodynamik_start(m,s,{});
            if(drive) s=testantrieb_start(m,s,{},true);
            if(fall.find("foreign")!=std::string::npos) {
                DynamischeFremdkraftKonfiguration c;
                c.aktiv=fall.find("_on")!=std::string::npos;
                c.kraft_modellfest_n={10.,0.,0.};
                c.angriffspunkt_modell_m=m.schwerpunkt_m;
                s=dynamische_fremdkraft_start(m,s,c);
            }
            std::string bewegen;
            for(const auto& e:m.elemente) if(e.bewegung) {bewegen=e.id;break;}
            bericht["setup_ms"]=ms(begin);
            double schritt_ms=0.,json_ms=0.;
            std::uint64_t digest=14695981039346656037ull;
            std::string letzte;
            for(int i=0;i<=schritte;++i) {
                if(i>0) {
                    std::vector<Bewegungsereignis> events;
                    if(ereignisse && !bewegen.empty() && (i==schritte/2 || i==schritte/2+1))
                        events.push_back({static_cast<std::size_t>(i),bewegen,50.});
                    begin=Uhr::now();
#ifndef CORE_PERFORMANCE_BASELINE
                    s=cache.schritt(s,.01,{},events);
#else
                    s=dynamikschritt(m,s,.01,{},events);
#endif
                    schritt_ms+=ms(begin);
                }
                begin=Uhr::now();
#ifndef CORE_PERFORMANCE_BASELINE
                letzte=cache.json(s,"laufend");
#else
                letzte=zustand_json(m,s,"laufend");
#endif
                json_ms+=ms(begin);
                for(unsigned char c:letzte) {digest^=c;digest*=1099511628211ull;}
            }
            bericht["steps_ms"]=schritt_ms; bericht["json_ms"]=json_ms;
            begin=Uhr::now();
#ifndef CORE_PERFORMANCE_BASELINE
            const auto k=cache.auswertung(s);
#else
            const auto k=kraefte(m,s.pose,s.testantrieb.get(),1000.,9.81,s.geflutet,
                s.hydrodynamik.get(),s.geschwindigkeit_welt_m_s,s.omega_modell_rad_s,
                s.bewegungsstellungen,s.dynamische_fremdkraft.get());
#endif
            bericht["force_ms"]=ms(begin);
            bericht["force_n"]=k.kraft_welt_n; bericht["moment_nm"]=k.moment_modell_nm;
            bericht["trajectory_fnv1a64"]=std::to_string(digest);
            bericht["states"]=schritte+1;
            bericht["status"]="ok";
            if(rep==0) bericht["final"]=nlohmann::json::parse(letzte);
        } catch(const std::exception& e) {bericht["status"]="invalid";bericht["diagnosis"]=e.what();}
        bericht["total_ms"]=ms(gesamt);
        if(rep>=0) std::cout<<bericht.dump()<<'\n';
    }
}
