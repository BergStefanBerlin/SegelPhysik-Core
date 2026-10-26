#pragma once

#include <array>

namespace segelphysik {

using Vektor3 = std::array<double, 3>;
using QuaternionXyzw = std::array<double, 4>;

class Pose {
public:
    Pose(Vektor3 position_m, QuaternionXyzw orientierung_xyzw);

    const Vektor3& position_m() const noexcept;
    const QuaternionXyzw& orientierung_xyzw() const noexcept;
    Vektor3 modellpunkt_nach_welt(const Vektor3& punkt_m) const;
    Vektor3 modellvektor_nach_welt(const Vektor3& vektor) const;
    Vektor3 weltpunkt_nach_modell(const Vektor3& punkt_m) const;
    Vektor3 weltvektor_nach_modell(const Vektor3& vektor) const;

private:
    Vektor3 position_;
    QuaternionXyzw orientierung_;
    std::array<double, 9> rotation_;
};

}
