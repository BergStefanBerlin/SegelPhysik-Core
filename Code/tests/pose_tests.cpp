#include "segelphysik/pose.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

void gleich(const segelphysik::Vektor3& ist, const segelphysik::Vektor3& soll)
{
    for (std::size_t i = 0; i < 3; ++i) {
        if (!std::isfinite(ist[i]) || std::abs(ist[i] - soll[i]) > 3e-14) {
            throw std::runtime_error("Pose-Transformation stimmt nicht mit der Referenz ueberein.");
        }
    }
}

template <typename Fehler, typename Funktion>
void erwartet(Funktion funktion, const std::string& id)
{
    try {
        funktion();
    } catch (const Fehler& fehler) {
        if (std::string(fehler.what()).find(id) == std::string::npos) {
            throw std::runtime_error("Falsche Fehler-ID.");
        }
        return;
    }
    throw std::runtime_error("Erwarteter Fehler wurde nicht gemeldet.");
}

void pruefen()
{
    using segelphysik::Pose;
    const Pose identitaet{{}, {0., 0., 0., 1.}};
    gleich(identitaet.modellpunkt_nach_welt({1., 2., 3.}), {1., 2., 3.});
    const double s = std::sqrt(.5);
    const Pose pose{{10., 20., 30.}, {0., 0., s, s}};
    gleich(pose.modellpunkt_nach_welt({1., 2., 3.}), {8., 21., 33.});
    gleich(pose.modellvektor_nach_welt({1., 2., 3.}), {-2., 1., 3.});
    gleich(pose.weltpunkt_nach_modell({8., 21., 33.}), {1., 2., 3.});
    gleich(pose.weltvektor_nach_modell({-2., 1., 3.}), {1., 2., 3.});
    gleich(pose.modellvektor_nach_welt({}), {});
    gleich(pose.position_m(), {10., 20., 30.});

    const Pose rechts{{}, {0., 0., 1., 0.}};
    gleich(rechts.modellvektor_nach_welt({1., 0., 0.}), {-1., 0., 0.});
    const Pose x_rotation{{}, {s, 0., 0., s}};
    gleich(x_rotation.modellvektor_nach_welt({0., 1., 0.}), {0., 0., 1.});
    const Pose y_rotation{{}, {0., s, 0., s}};
    gleich(y_rotation.modellvektor_nach_welt({0., 0., 1.}), {1., 0., 0.});

    for (double skala : {1e-300, 1., 1e300, -1e300}) {
        const Pose normiert{{}, {0., 0., skala, skala}};
        gleich(normiert.modellvektor_nach_welt({1., 2., 3.}), {-2., 1., 3.});
        const auto& q = normiert.orientierung_xyzw();
        if (std::abs(std::hypot(q[2], q[3]) - 1.) > 1e-15) {
            throw std::runtime_error("Quaternion nicht normiert.");
        }
    }
    const Pose beliebig{{1., 2., 3.}, {.2, -.3, .4, .5}};
    gleich(beliebig.weltpunkt_nach_modell(
        beliebig.modellpunkt_nach_welt({4., 5., 6.})), {4., 5., 6.});
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    erwartet<std::invalid_argument>([] { Pose{{}, {}}; }, "CORE-POSE-0002");
    erwartet<std::invalid_argument>([&] { Pose{{nan, 0., 0.}, {0., 0., 0., 1.}}; },
                                     "CORE-POSE-0001");
    erwartet<std::invalid_argument>([&] { Pose{{}, {0., inf, 0., 1.}}; },
                                     "CORE-POSE-0002");
    erwartet<std::invalid_argument>([&] { Pose{{}, {0., 0., nan, 1.}}; },
                                     "CORE-POSE-0002");
    erwartet<std::invalid_argument>([&] { pose.modellvektor_nach_welt({inf, 0., 0.}); },
                                     "CORE-POSE-0003");
    erwartet<std::invalid_argument>([&] { pose.weltpunkt_nach_modell({nan, 0., 0.}); },
                                     "CORE-POSE-0003");
    const double maximum = std::numeric_limits<double>::max();
    const Pose gross{{maximum, 0., 0.}, {0., 0., 0., 1.}};
    erwartet<std::overflow_error>([&] { gross.modellpunkt_nach_welt({maximum, 0., 0.}); },
                                  "CORE-POSE-0003");
    erwartet<std::overflow_error>([&] { gross.weltpunkt_nach_modell({-maximum, 0., 0.}); },
                                  "CORE-POSE-0003");
}

}

int main()
{
    try {
        pruefen();
    } catch (const std::exception& fehler) {
        std::cerr << fehler.what() << '\n';
        return 1;
    }
    return 0;
}
