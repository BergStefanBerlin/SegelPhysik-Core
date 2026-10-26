#include "segelphysik/simulation.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <stdexcept>

namespace {
template<typename T>
double volumenprobe(std::size_t n,double offset)
{
    T sum=0;
    const T o=static_cast<T>(offset);
    for(std::size_t i=0;i<n;++i) {
        const T a=o+static_cast<T>(.01), b=o+static_cast<T>(.02);
        sum+=(a-o)*(b-o)*static_cast<T>(.03);
    }
    return static_cast<double>(sum)/static_cast<double>(n);
}
}
int main(int argc,char* argv[])
{
    if(argc!=2) {std::cerr<<"Aufruf: core_benchmark MODELL\n";return 2;}
    try {
        const auto m=segelphysik::lade_modell(argv[1]);
        const segelphysik::Pose pose{{},{.01,.02,0.,1.}};
        std::vector<double> times;
        double guard=0;
        for(int warm=0;warm<20;++warm) guard+=segelphysik::hydrostatik(m,pose).volumen_m3;
        constexpr int count=500;
        for(int rep=0;rep<9;++rep) {
            const auto begin=std::chrono::steady_clock::now();
            for(int i=0;i<count;++i) guard+=segelphysik::hydrostatik(m,pose).volumen_m3;
            times.push_back(std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count());
        }
        std::sort(times.begin(),times.end());
        const auto state=segelphysik::zustand_json(m,segelphysik::Zustand{},"laufend");
        auto formatzeit=[&](bool jsonl) {
            std::vector<double> sample;
            for(int rep=0;rep<9;++rep) {
                const auto begin=std::chrono::steady_clock::now();
                std::string out;out.reserve((state.size()+1)*1000+2);
                if(!jsonl) out+='[';
                for(int i=0;i<1000;++i) {
                    if(!jsonl&&i) out+=',';
                    out+=state;
                    if(jsonl) out+='\n';
                }
                if(!jsonl) out+=']';
                guard+=static_cast<double>(out.size())*1e-15;
                sample.push_back(std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count());
            }
            std::sort(sample.begin(),sample.end());return sample[4];
        };
        const double jsonlzeit=formatzeit(true),jsonzeit=formatzeit(false);
        const double f=volumenprobe<float>(1000,1000.);
        const double d=volumenprobe<double>(1000,1000.);
        const double expected=.01*.02*.03;
        auto formattest=[&](bool einfach) {
            const auto begin=std::chrono::steady_clock::now();
            for(int i=0;i<1000;++i) guard+=einfach?
                volumenprobe<float>(1000,1000.+i*.01):volumenprobe<double>(1000,1000.+i*.01);
            return std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();
        };
        const double floatzeit=formattest(true),doublezeit=formattest(false);
        std::cout<<std::setprecision(17)
            <<"{\"modell_fassung\":\""<<m.fassung<<"\",\"wiederholungen\":9,\"auswertungen_pro_lauf\":500,"
            <<"\"median_s\":"<<times[4]<<",\"min_s\":"<<times.front()<<",\"max_s\":"<<times.back()
            <<",\"auswertungen_pro_s\":"<<count/times[4]
            <<",\"float_probe_relfehler\":"<<std::abs(f-expected)/expected
            <<",\"double_probe_relfehler\":"<<std::abs(d-expected)/expected
            <<",\"float_probe_s\":"<<floatzeit<<",\"double_probe_s\":"<<doublezeit
            <<",\"jsonl_1000_zustaende_median_s\":"<<jsonlzeit
            <<",\"json_array_1000_zustaende_median_s\":"<<jsonzeit
            <<",\"checksum\":"<<guard<<"}\n";
        if(!std::isfinite(guard)) throw std::runtime_error("Nicht endlicher Benchmark");
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
    return 0;
}
