#include "segelphysik/hydrostatik.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace segelphysik {
namespace {
constexpr double PI = 3.14159265358979323846;

// alpha0, beta0, gamma0 des Ellipsoids (Lamb); logarithmische Trapezregel.
std::array<double, 3> lamb_integrale(double a, double b, double c)
{
    const std::array<double, 3> s2{a * a, b * b, c * c};
    const double skala = std::max({s2[0], s2[1], s2[2]});
    std::array<double, 3> result{};
    const int n = 6000;
    const double u0 = -30., u1 = 30., h = (u1 - u0) / n;
    for (int i = 0; i <= n; ++i) {
        const double lam = skala * std::exp(u0 + i * h);
        const double delta = std::sqrt((s2[0] + lam) * (s2[1] + lam) * (s2[2] + lam));
        const double gewicht = (i == 0 || i == n) ? .5 : 1.;
        for (std::size_t k = 0; k < 3; ++k)
            result[k] += gewicht * h * lam / ((s2[k] + lam) * delta);
    }
    for (auto& v : result) v *= a * b * c;
    return result;
}

double rotationsfaktor(double p2, double q2, double alpha_p, double alpha_q)
{
    // Rotation um die dritte Achse zwischen Achsen mit Quadraten p2, q2.
    const double dif = p2 - q2;
    if (std::abs(dif) <= 1e-9 * std::max(p2, q2)) return 0.;
    return dif * dif * (alpha_q - alpha_p) / (5. * (2. * dif + (p2 + q2) * (alpha_p - alpha_q)));
}

struct Segment { Vektor3 w0, w1, m0, m1; };

Vektor3 minus(const Vektor3& a, const Vektor3& b) { return {a[0]-b[0], a[1]-b[1], a[2]-b[2]}; }

double gesamtvolumen(const Mesh& mesh)
{
    double v = 0.;
    for (const auto& t : mesh.dreiecke) {
        const auto& a = mesh.punkte.at(t[0]);
        const auto& b = mesh.punkte.at(t[1]);
        const auto& c = mesh.punkte.at(t[2]);
        v += (a[0]*(b[1]*c[2]-b[2]*c[1]) - a[1]*(b[0]*c[2]-b[2]*c[0]) + a[2]*(b[0]*c[1]-b[1]*c[0])) / 6.;
    }
    return v;
}

TraegheitsWert geschaetzt(double wert, const char* methode, const char* quelle, const char* unsicherheit)
{
    return {wert, "geschaetzt", methode, quelle, unsicherheit};
}

void pruefen(double wert, const char* name)
{
    if (!std::isfinite(wert) || wert < 0.)
        throw std::invalid_argument(std::string("CORE-SIM-0002: Hydrodynamische Traegheit ungueltig: ") + name);
}
}

std::array<double, 6> ellipsoid_zusatzmasse(double a, double b, double c, double v, double rho)
{
    if (!(a > 0. && b > 0. && c > 0. && v > 0. && rho > 0.) ||
        !std::isfinite(a + b + c + v + rho))
        throw std::invalid_argument("CORE-SIM-0002: Ellipsoidparameter ungueltig.");
    const auto al = lamb_integrale(a, b, c);
    std::array<double, 6> r{};
    for (std::size_t i = 0; i < 3; ++i) r[i] = al[i] / (2. - al[i]) * rho * v;
    r[3] = rotationsfaktor(b * b, c * c, al[1], al[2]) * rho * v;
    r[4] = rotationsfaktor(c * c, a * a, al[2], al[0]) * rho * v;
    r[5] = rotationsfaktor(a * a, b * b, al[0], al[1]) * rho * v;
    return r;
}

HydrodynamischeTraegheit berechne_hydrodynamische_traegheit(
    const Modell& modell, const Pose& pose, double rho, double g)
{
    HydrodynamischeTraegheit ht;
    const Vektor3 cg = modell.schwerpunkt_m;
    const Vektor3 cg_welt = pose.modellpunkt_nach_welt(cg);

    // Wasserlinie der Rumpfelemente bei z_welt = 0 (Orientierung: Normale rechts der Richtung).
    std::vector<Segment> segmente;
    double v_huelle = 0., zmin = std::numeric_limits<double>::infinity(), zmax = -zmin;
    std::array<double, 3> bmin{zmin, zmin, zmin}, bmax{zmax, zmax, zmax};
    double vb_welt = 0., zb_welt_summe = 0.;
    bool rumpf_vorhanden = false;
    for (const auto& e : modell.elemente) {
        if (e.hydrodynamik_rolle != HydrodynamikRolle::rumpf || !e.volumen) continue;
        rumpf_vorhanden = true;
        v_huelle += std::max(0., gesamtvolumen(*e.volumen));
        const auto vw = wasseranteil(*e.volumen, pose);
        if (vw.schwerpunkt_m) {
            vb_welt += vw.volumen_m3;
            zb_welt_summe += vw.volumen_m3 * pose.modellpunkt_nach_welt(*vw.schwerpunkt_m)[2];
        }
        for (const auto& p : e.volumen->punkte) {
            for (std::size_t i = 0; i < 3; ++i) {
                bmin[i] = std::min(bmin[i], p[i]);
                bmax[i] = std::max(bmax[i], p[i]);
            }
            const double z = pose.modellpunkt_nach_welt(p)[2];
            zmin = std::min(zmin, z);
            zmax = std::max(zmax, z);
        }
        for (const auto& t : e.volumen->dreiecke) {
            std::array<Vektor3, 3> m{e.volumen->punkte.at(t[0]), e.volumen->punkte.at(t[1]),
                                     e.volumen->punkte.at(t[2])};
            std::array<Vektor3, 3> w{pose.modellpunkt_nach_welt(m[0]), pose.modellpunkt_nach_welt(m[1]),
                                     pose.modellpunkt_nach_welt(m[2])};
            std::array<Vektor3, 2> pw{}, pm{};
            std::size_t n = 0;
            for (std::size_t i = 0; i < 3 && n < 2; ++i) {
                const std::size_t j = (i + 1) % 3;
                if ((w[i][2] >= 0.) == (w[j][2] >= 0.)) continue;
                const double s = w[i][2] / (w[i][2] - w[j][2]);
                for (std::size_t k = 0; k < 3; ++k) {
                    pw[n][k] = w[i][k] + s * (w[j][k] - w[i][k]);
                    pm[n][k] = m[i][k] + s * (m[j][k] - m[i][k]);
                }
                ++n;
            }
            if (n != 2) continue;
            const auto nrm = [&] {
                const auto u = minus(w[1], w[0]), v = minus(w[2], w[0]);
                return Vektor3{u[1]*v[2]-u[2]*v[1], u[2]*v[0]-u[0]*v[2], u[0]*v[1]-u[1]*v[0]};
            }();
            const double dx = -nrm[1], dy = nrm[0];
            const double ls = (pw[1][0]-pw[0][0])*dx + (pw[1][1]-pw[0][1])*dy;
            if (ls == 0.) continue;
            if (ls > 0.) segmente.push_back({pw[0], pw[1], pm[0], pm[1]});
            else segmente.push_back({pw[1], pw[0], pm[1], pm[0]});
        }
    }
    if (!rumpf_vorhanden)
        ht.diagnosen.push_back("Kein Rumpfelement: keine Rumpf-Zusatzmasse");

    const double v_ref = std::min(modell.masse_kg / rho, v_huelle);
    const bool wasserlinie = !segmente.empty();
    double flaeche = 0., ixx = 0., iyy = 0.;
    double lmin = std::numeric_limits<double>::infinity(), lmax = -lmin, wmin = lmin, wmax = -lmin, zm = 0.;
    std::string basis;
    std::array<double, 6> a_hull{};
    Vektor3 zentrum = cg;
    if (!rumpf_vorhanden) {
        basis = "kein Rumpf";
        ht.gieren_quadratisch = geschaetzt(0., "ohne Rumpf nicht bestimmt",
            "Messung U-C-000067/U-C-000071", "unbestimmt");
    } else if (wasserlinie) {
        const auto bx = pose.modellvektor_nach_welt({1., 0., 0.});
        const double hn = std::hypot(bx[0], bx[1]);
        const double hx = hn > 1e-9 ? bx[0] / hn : 1., hy = hn > 1e-9 ? bx[1] / hn : 0.;
        double sx = 0., sy = 0.;
        std::size_t zahl = 0;
        for (const auto& s : segmente) {
            const double x0 = s.w0[0]*hx + s.w0[1]*hy, y0 = -s.w0[0]*hy + s.w0[1]*hx;
            const double x1 = s.w1[0]*hx + s.w1[1]*hy, y1 = -s.w1[0]*hy + s.w1[1]*hx;
            flaeche += .5 * (x0*y1 - x1*y0);
            const double dy = y1 - y0, dx = x1 - x0;
            sx += .5 * dy * (x0*x0 + x0*x1 + x1*x1) / 3.;
            sy += -.5 * dx * (y0*y0 + y0*y1 + y1*y1) / 3.;
            ixx += -dx * (y0*y0*y0 + y0*y0*y1 + y0*y1*y1 + y1*y1*y1) / 12.;
            iyy += dy * (x0*x0*x0 + x0*x0*x1 + x0*x1*x1 + x1*x1*x1) / 12.;
            for (const auto& p : {s.m0, s.m1}) {
                lmin = std::min(lmin, p[0]); lmax = std::max(lmax, p[0]);
                wmin = std::min(wmin, p[1]); wmax = std::max(wmax, p[1]);
                zm += p[2];
                ++zahl;
            }
        }
        if (flaeche > 1e-12) {
            ixx -= flaeche * (sy / flaeche) * (sy / flaeche);
            iyy -= flaeche * (sx / flaeche) * (sx / flaeche);
        } else {
            flaeche = 0.;
        }
        zm /= static_cast<double>(zahl);
        const double laenge = lmax - lmin, breite = wmax - wmin, tiefgang = -zmin;
        if (!(laenge > 0. && breite > 0. && tiefgang > 0.))
            throw std::invalid_argument("CORE-SIM-0002: Wasserlinie des Rumpfes degeneriert.");
        a_hull = ellipsoid_zusatzmasse(laenge / 2., breite / 2., tiefgang, v_ref, rho);
        zentrum = {(lmin + lmax) / 2., (wmin + wmax) / 2., zm};
        basis = "Doppelkoerper-Halbellipsoid (Lamb) aus Wasserlinie, Tiefgang und Verdraengung";
        ht.gieren_quadratisch = geschaetzt(
            .1 * rho * tiefgang * std::pow(laenge, 4) / 64., "L^4-Scheibenformel, Faktor 0,1",
            "Messung U-C-000067/U-C-000071", "Faktor 3");
    } else {
        const double a = (bmax[0] - bmin[0]) / 2., b = (bmax[1] - bmin[1]) / 2., c = (bmax[2] - bmin[2]) / 2.;
        if (!(a > 0. && b > 0. && c > 0.))
            throw std::invalid_argument("CORE-SIM-0002: Rumpfhuelle degeneriert.");
        a_hull = ellipsoid_zusatzmasse(a, b, c, std::max(v_ref, 1e-12), rho);
        zentrum = {(bmin[0] + bmax[0]) / 2., (bmin[1] + bmax[1]) / 2., (bmin[2] + bmax[2]) / 2.};
        basis = "Vollellipsoid aus Huellkoerper (nicht an der Wasserlinie)";
        ht.gieren_quadratisch = geschaetzt(0., "ohne Wasserlinie nicht bestimmt",
            "Messung U-C-000067/U-C-000071", "unbestimmt");
    }
    const char* quelle_ref = "Lamb, Hydrodynamics Art. 114; Newman, Marine Hydrodynamics 4.";
    std::array<double, 6> a = a_hull;
    // Steiner-Verschiebung auf den Gesamtschwerpunkt.
    const double dx = zentrum[0] - cg[0], dy = zentrum[1] - cg[1], dz = zentrum[2] - cg[2];
    a[3] += a_hull[1] * dz * dz + a_hull[2] * dy * dy;
    a[4] += a_hull[0] * dz * dz + a_hull[2] * dx * dx;
    a[5] += a_hull[1] * dx * dx + a_hull[0] * dy * dy;

    // Ruder und Schwert: Plattennaeherung fuer die Querrichtung.
    for (const auto& e : modell.elemente) {
        if (!e.hydrodynamik_rolle || *e.hydrodynamik_rolle == HydrodynamikRolle::rumpf || !e.volumen) continue;
        std::array<double, 3> fmin{std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity(),
            std::numeric_limits<double>::infinity()}, fmax{-fmin[0], -fmin[1], -fmin[2]};
        double wz0 = std::numeric_limits<double>::infinity(), wz1 = -wz0;
        for (const auto& p : e.volumen->punkte) {
            for (std::size_t i = 0; i < 3; ++i) {
                fmin[i] = std::min(fmin[i], p[i]);
                fmax[i] = std::max(fmax[i], p[i]);
            }
            const double z = pose.modellpunkt_nach_welt(p)[2];
            wz0 = std::min(wz0, z);
            wz1 = std::max(wz1, z);
        }
        const double spann = std::min(wz1, 0.) - wz0;
        const double sehne = fmax[0] - fmin[0];
        if (spann <= 0. || sehne <= 0.) continue;
        const double ap = rho * PI / 4. * sehne * sehne * spann;
        const double px = (fmin[0] + fmax[0]) / 2. - cg[0];
        const double pz = (fmin[2] + fmax[2]) / 2. - cg[2];
        a[1] += ap;
        a[3] += ap * pz * pz;
        a[5] += ap * px * px;
    }

    const char* hull_unsicher = "Faktor 1,5 (Naeherung durch Ellipsoid)";
    for (std::size_t i = 0; i < 6; ++i) {
        ht.zusatzmasse[i] = geschaetzt(a[i], basis.c_str(), quelle_ref, hull_unsicher);
    }
    ht.zeta_rollen = geschaetzt(.10, "zusaetzliches Lehr-Daempfungsmass", "Messung U-C-000067/U-C-000071", "Faktor 2");
    ht.zeta_stampfen = geschaetzt(.35, "zusaetzliches Lehr-Daempfungsmass", "Messung U-C-000067/U-C-000071", "Faktor 2");
    ht.zeta_tauchen = geschaetzt(.30, "zusaetzliches Lehr-Daempfungsmass", "Messung U-C-000067/U-C-000071", "Faktor 2");
    ht.gieren_linear = geschaetzt(.1, "Anteil der Zusatztraegheit pro Sekunde", "Messung U-C-000067/U-C-000071", "Faktor 3");

    if (modell.hydrodynamische_traegheit) {
        const auto& blk = *modell.hydrodynamische_traegheit;
        static const char* namen[6] = {"a11", "a22", "a33", "a44", "a55", "a66"};
        for (std::size_t i = 0; i < 6; ++i) {
            const auto f = blk.zusatzmasse.find(namen[i]);
            if (f == blk.zusatzmasse.end()) continue;
            ht.zusatzmasse[i] = {f->second, "modell", "Modellblock Hydrodynamische_Traegheit", "Modelldatei", "vom Modell"};
        }
        auto ueberschreiben = [&](const char* key, TraegheitsWert& w) {
            const auto f = blk.daempfung.find(key);
            if (f != blk.daempfung.end())
                w = {f->second, "modell", "Modellblock Hydrodynamische_Traegheit", "Modelldatei", "vom Modell"};
        };
        ueberschreiben("zeta_rollen", ht.zeta_rollen);
        ueberschreiben("zeta_stampfen", ht.zeta_stampfen);
        ueberschreiben("zeta_tauchen", ht.zeta_tauchen);
        ueberschreiben("gieren_linear", ht.gieren_linear);
        ueberschreiben("gieren_quadratisch", ht.gieren_quadratisch);
    }
    for (std::size_t i = 0; i < 6; ++i) pruefen(ht.zusatzmasse[i].wert, "zusatzmasse");
    for (const auto* w : {&ht.zeta_rollen, &ht.zeta_stampfen, &ht.zeta_tauchen, &ht.gieren_linear,
                          &ht.gieren_quadratisch})
        pruefen(w->wert, "daempfung");

    // Rueckstellsteifigkeiten (Wasserlinienflaeche, Metazentrum).
    const double vb = vb_welt;
    const double zb = vb > 0. ? zb_welt_summe / vb : cg_welt[2];
    const double kompensation = rho * g * vb * (zb - cg_welt[2]);
    ht.c33_n_m = rho * g * flaeche;
    ht.c44_nm_rad = rho * g * ixx + kompensation;
    ht.c55_nm_rad = rho * g * iyy + kompensation;
    const auto& I = modell.traegheit_kg_m2;
    ht.effektive_masse_kg = {modell.masse_kg + a[0], modell.masse_kg + a[1], modell.masse_kg + a[2]};
    ht.effektive_traegheit_kg_m2 = {I[0] + a[3], I[4] + a[4], I[8] + a[5]};
    auto b_lin = [&](double zeta, double c, double m_eff, const char* name) {
        if (c <= 0.) {
            ht.diagnosen.push_back(std::string("Rueckstellung ") + name + " nicht positiv: lineare Daempfung 0");
            return 0.;
        }
        return 2. * zeta * std::sqrt(c * m_eff);
    };
    ht.b_rollen_nms = b_lin(ht.zeta_rollen.wert, ht.c44_nm_rad, ht.effektive_traegheit_kg_m2[0], "Rollen");
    ht.b_stampfen_nms = b_lin(ht.zeta_stampfen.wert, ht.c55_nm_rad, ht.effektive_traegheit_kg_m2[1], "Stampfen");
    ht.b_tauchen_ns_m = b_lin(ht.zeta_tauchen.wert, ht.c33_n_m, ht.effektive_masse_kg[2], "Tauchen");
    return ht;
}
}
