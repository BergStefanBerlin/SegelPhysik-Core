#include "segelphysik/pose.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace segelphysik {
namespace {

template <std::size_t N>
bool endlich(const std::array<double, N>& werte)
{
    return std::all_of(werte.begin(), werte.end(),
        [](double wert) { return std::isfinite(wert); });
}

Vektor3 drehen(const std::array<double, 9>& rotation, const Vektor3& vektor,
              bool invers)
{
    if (!endlich(vektor)) {
        throw std::invalid_argument("CORE-POSE-0003: Punkt oder Vektor muss endlich sein.");
    }
    Vektor3 ergebnis{};
    for (std::size_t i = 0; i < 3; ++i) {
        for (std::size_t j = 0; j < 3; ++j) {
            ergebnis[i] += rotation[invers ? 3 * j + i : 3 * i + j] * vektor[j];
        }
    }
    if (!endlich(ergebnis)) {
        throw std::overflow_error("CORE-POSE-0003: Ueberlauf bei der Rotation.");
    }
    return ergebnis;
}

}

Pose::Pose(Vektor3 position_m, QuaternionXyzw orientierung_xyzw)
    : position_(position_m), orientierung_(orientierung_xyzw), rotation_{}
{
    if (!endlich(position_)) {
        throw std::invalid_argument("CORE-POSE-0001: Position muss endlich sein.");
    }
    if (!endlich(orientierung_)) {
        throw std::invalid_argument("CORE-POSE-0002: Quaternion muss endlich sein.");
    }
    double skala = 0.;
    for (double wert : orientierung_) {
        skala = std::max(skala, std::abs(wert));
    }
    if (skala == 0.) {
        throw std::invalid_argument("CORE-POSE-0002: Quaternion darf nicht null sein.");
    }
    // Scaling before the norm avoids overflow and preserves tiny valid inputs.
    for (double& wert : orientierung_) {
        wert /= skala;
    }
    const double norm = std::hypot(std::hypot(orientierung_[0], orientierung_[1]),
                                   std::hypot(orientierung_[2], orientierung_[3]));
    for (double& wert : orientierung_) {
        wert /= norm;
    }
    const auto& q = orientierung_;
    const double x = q[0], y = q[1], z = q[2], w = q[3];
    rotation_ = {
        1. - 2. * (y * y + z * z), 2. * (x * y - z * w), 2. * (x * z + y * w),
        2. * (x * y + z * w), 1. - 2. * (x * x + z * z), 2. * (y * z - x * w),
        2. * (x * z - y * w), 2. * (y * z + x * w), 1. - 2. * (x * x + y * y)
    };
}

const Vektor3& Pose::position_m() const noexcept
{
    return position_;
}

const QuaternionXyzw& Pose::orientierung_xyzw() const noexcept
{
    return orientierung_;
}

Vektor3 Pose::modellpunkt_nach_welt(const Vektor3& punkt_m) const
{
    auto ergebnis = modellvektor_nach_welt(punkt_m);
    for (std::size_t i = 0; i < 3; ++i) {
        ergebnis[i] += position_[i];
    }
    if (!endlich(ergebnis)) {
        throw std::overflow_error("CORE-POSE-0003: Ueberlauf bei der Translation.");
    }
    return ergebnis;
}

Vektor3 Pose::modellvektor_nach_welt(const Vektor3& vektor) const
{
    return drehen(rotation_, vektor, false);
}

Vektor3 Pose::weltpunkt_nach_modell(const Vektor3& punkt_m) const
{
    if (!endlich(punkt_m)) {
        throw std::invalid_argument("CORE-POSE-0003: Punkt muss endlich sein.");
    }
    Vektor3 verschoben{};
    for (std::size_t i = 0; i < 3; ++i) {
        verschoben[i] = punkt_m[i] - position_[i];
    }
    if (!endlich(verschoben)) {
        throw std::overflow_error("CORE-POSE-0003: Ueberlauf bei der inversen Translation.");
    }
    return weltvektor_nach_modell(verschoben);
}

Vektor3 Pose::weltvektor_nach_modell(const Vektor3& vektor) const
{
    return drehen(rotation_, vektor, true);
}

}
