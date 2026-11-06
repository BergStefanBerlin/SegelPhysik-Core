#include "segelphysik/modell.hpp"
#include "segelphysik/messung.hpp"
#include "modell_intern.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <limits>
#include <locale>
#include <map>
#include <numeric>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace segelphysik {
namespace {

class Fingerabdruck {
public:
    void datei(const std::string& inhalt)
    {
        std::uint64_t laenge = static_cast<std::uint64_t>(inhalt.size());
        for (unsigned i = 0; i < 8; ++i) {
            byte(static_cast<unsigned char>(laenge & 255));
            laenge >>= 8;
        }
        for (unsigned char c : inhalt) byte(c);
    }
    std::string text() const
    {
        std::ostringstream ausgabe;
        ausgabe.imbue(std::locale::classic());
        ausgabe << "fnv1a64:" << std::hex << std::setfill('0') << std::setw(16) << wert_;
        return ausgabe.str();
    }
private:
    std::uint64_t wert_ = UINT64_C(14695981039346656037);
    void byte(unsigned char c) { wert_ = (wert_ ^ c) * UINT64_C(1099511628211); }
};

[[noreturn]] void fehler(int nummer, const std::string& ort, const std::string& text)
{
    throw std::runtime_error("CORE-MOD-000" + std::to_string(nummer) + " [" + ort + "]: " + text);
}

std::string lesen(const std::filesystem::path& datei, const std::string& ort)
{
    std::ifstream strom(datei, std::ios::binary);
    if (!strom) fehler(5, ort, "Datei nicht lesbar: " + datei.u8string());
    std::string text{std::istreambuf_iterator<char>(strom), std::istreambuf_iterator<char>()};
    if (strom.bad()) fehler(5, ort, "Lesefehler: " + datei.u8string());
    return text;
}

struct Json {
    enum class Typ { null, boolesch, zahl, text, liste, objekt };
    Typ typ = Typ::null;
    bool bool_wert{};
    double zahl = 0.;
    std::string text;
    std::vector<Json> liste;
    std::map<std::string, Json> objekt;
};

class Parser {
public:
    Parser(const std::string& text, std::string ort) : text_(text), ort_(std::move(ort)) {}
    Json lesen()
    {
        Json wert = wert_lesen(0);
        leerraum();
        if (pos_ != text_.size()) ungueltig("Zeichen nach JSON-Ende");
        return wert;
    }
private:
    const std::string& text_;
    std::string ort_;
    std::size_t pos_ = 0;

    [[noreturn]] void ungueltig(const std::string& text) const
    {
        fehler(1, ort_ + ", Byte " + std::to_string(pos_ + 1), text);
    }
    void leerraum()
    {
        while (pos_ < text_.size() &&
               (text_[pos_] == ' ' || text_[pos_] == '\t' ||
                text_[pos_] == '\r' || text_[pos_] == '\n')) ++pos_;
    }
    bool nehmen(char c)
    {
        if (pos_ < text_.size() && text_[pos_] == c) { ++pos_; return true; }
        return false;
    }
    void erwarten(char c)
    {
        if (!nehmen(c)) ungueltig(std::string("Erwartet: ") + c);
    }
    unsigned hex()
    {
        unsigned wert = 0;
        for (int i = 0; i < 4; ++i) {
            if (pos_ == text_.size()) ungueltig("Unvollstaendiger Unicode-Escape");
            const char c = text_[pos_++];
            wert *= 16;
            if (c >= '0' && c <= '9') wert += static_cast<unsigned>(c - '0');
            else if (c >= 'a' && c <= 'f') wert += static_cast<unsigned>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') wert += static_cast<unsigned>(c - 'A' + 10);
            else ungueltig("Ungueltiger Unicode-Escape");
        }
        return wert;
    }
    static void utf8(std::string& ausgabe, unsigned c)
    {
        if (c < 0x80) ausgabe += static_cast<char>(c);
        else if (c < 0x800) {
            ausgabe += static_cast<char>(0xc0 | (c >> 6));
            ausgabe += static_cast<char>(0x80 | (c & 63));
        } else if (c < 0x10000) {
            ausgabe += static_cast<char>(0xe0 | (c >> 12));
            ausgabe += static_cast<char>(0x80 | ((c >> 6) & 63));
            ausgabe += static_cast<char>(0x80 | (c & 63));
        } else {
            ausgabe += static_cast<char>(0xf0 | (c >> 18));
            ausgabe += static_cast<char>(0x80 | ((c >> 12) & 63));
            ausgabe += static_cast<char>(0x80 | ((c >> 6) & 63));
            ausgabe += static_cast<char>(0x80 | (c & 63));
        }
    }
    std::string string_lesen()
    {
        erwarten('"');
        std::string ausgabe;
        while (pos_ < text_.size()) {
            const unsigned char c = static_cast<unsigned char>(text_[pos_++]);
            if (c == '"') return ausgabe;
            if (c < 32) ungueltig("Steuerzeichen in Zeichenkette");
            if (c == '\\') {
                if (pos_ == text_.size()) ungueltig("Unvollstaendiger Escape");
                switch (text_[pos_++]) {
                case '"': ausgabe += '"'; break;
                case '\\': ausgabe += '\\'; break;
                case '/': ausgabe += '/'; break;
                case 'b': ausgabe += '\b'; break;
                case 'f': ausgabe += '\f'; break;
                case 'n': ausgabe += '\n'; break;
                case 'r': ausgabe += '\r'; break;
                case 't': ausgabe += '\t'; break;
                case 'u': {
                    unsigned wert = hex();
                    if (wert >= 0xd800 && wert <= 0xdbff) {
                        if (!nehmen('\\') || !nehmen('u')) ungueltig("Fehlendes UTF-16-Surrogat");
                        const unsigned unten = hex();
                        if (unten < 0xdc00 || unten > 0xdfff) ungueltig("Ungueltiges UTF-16-Surrogat");
                        wert = 0x10000 + ((wert - 0xd800) << 10) + unten - 0xdc00;
                    } else if (wert >= 0xdc00 && wert <= 0xdfff) ungueltig("Isoliertes UTF-16-Surrogat");
                    utf8(ausgabe, wert);
                    break;
                }
                default: ungueltig("Unbekannter Escape");
                }
            } else if (c < 128) ausgabe += static_cast<char>(c);
            else {
                unsigned wert = 0, anzahl = 0, minimum = 0;
                if (c >= 0xc2 && c <= 0xdf) { wert = c & 31; anzahl = 1; minimum = 0x80; }
                else if (c >= 0xe0 && c <= 0xef) { wert = c & 15; anzahl = 2; minimum = 0x800; }
                else if (c >= 0xf0 && c <= 0xf4) { wert = c & 7; anzahl = 3; minimum = 0x10000; }
                else ungueltig("Ungueltiges UTF-8");
                for (unsigned i = 0; i < anzahl; ++i) {
                    if (pos_ == text_.size()) ungueltig("Unvollstaendiges UTF-8");
                    const auto folge = static_cast<unsigned char>(text_[pos_++]);
                    if ((folge & 0xc0) != 0x80) ungueltig("Ungueltiges UTF-8");
                    wert = (wert << 6) | (folge & 63);
                }
                if (wert < minimum || wert > 0x10ffff || (wert >= 0xd800 && wert <= 0xdfff))
                    ungueltig("Ungueltiger UTF-8-Codepunkt");
                utf8(ausgabe, wert);
            }
        }
        ungueltig("Nicht abgeschlossene Zeichenkette");
    }
    Json wert_lesen(unsigned tiefe)
    {
        if (tiefe > 128) ungueltig("JSON-Verschachtelung zu tief");
        leerraum();
        if (pos_ == text_.size()) ungueltig("Fehlender JSON-Wert");
        Json wert;
        if (text_[pos_] == '"') {
            wert.typ = Json::Typ::text;
            wert.text = string_lesen();
        } else if (nehmen('{')) {
            wert.typ = Json::Typ::objekt;
            leerraum();
            if (nehmen('}')) return wert;
            do {
                leerraum();
                const std::string schluessel = string_lesen();
                leerraum();
                erwarten(':');
                Json kind = wert_lesen(tiefe + 1);
                if (!wert.objekt.emplace(schluessel, std::move(kind)).second)
                    ungueltig("Doppelter Objektschluessel: " + schluessel);
                leerraum();
                if (nehmen('}')) return wert;
                erwarten(',');
            } while (true);
        } else if (nehmen('[')) {
            wert.typ = Json::Typ::liste;
            leerraum();
            if (nehmen(']')) return wert;
            do {
                wert.liste.push_back(wert_lesen(tiefe + 1));
                leerraum();
                if (nehmen(']')) return wert;
                erwarten(',');
            } while (true);
        } else if (text_[pos_] == '-' || (text_[pos_] >= '0' && text_[pos_] <= '9')) {
            wert.typ = Json::Typ::zahl;
            const std::size_t start = pos_;
            nehmen('-');
            if (!nehmen('0')) ziffern();
            if (nehmen('.')) ziffern();
            if (nehmen('e') || nehmen('E')) {
                if (!nehmen('+')) nehmen('-');
                ziffern();
            }
            const auto ergebnis = std::from_chars(text_.data() + start, text_.data() + pos_, wert.zahl);
            if (ergebnis.ec != std::errc{} || ergebnis.ptr != text_.data() + pos_ ||
                !std::isfinite(wert.zahl)) ungueltig("Zahl nicht endlich/darstellbar");
        } else {
            const auto literal = [&](std::string_view text) {
                if (text_.compare(pos_, text.size(), text) != 0) ungueltig("Unbekannter JSON-Wert");
                pos_ += text.size();
            };
            if (text_[pos_] == 't') { literal("true"); wert.typ = Json::Typ::boolesch; wert.bool_wert = true; }
            else if (text_[pos_] == 'f') { literal("false"); wert.typ = Json::Typ::boolesch; }
            else if (text_[pos_] == 'n') literal("null");
            else ungueltig("Unbekannter JSON-Wert");
        }
        return wert;
    }
    void ziffern()
    {
        const auto start = pos_;
        while (pos_ < text_.size() && text_[pos_] >= '0' && text_[pos_] <= '9') ++pos_;
        if (pos_ == start) ungueltig("Ziffer erwartet");
    }
};

nlohmann::json json_wert(const Json& wert, bool pfade_ausblenden = true)
{
    switch (wert.typ) {
    case Json::Typ::null: return nullptr;
    case Json::Typ::boolesch: return wert.bool_wert;
    case Json::Typ::zahl:
        if (std::trunc(wert.zahl) == wert.zahl &&
            wert.zahl >= static_cast<double>(std::numeric_limits<std::int64_t>::min()) &&
            wert.zahl < static_cast<double>(std::numeric_limits<std::int64_t>::max()))
            return static_cast<std::int64_t>(wert.zahl);
        return wert.zahl;
    case Json::Typ::text:
        return !pfade_ausblenden || wert.text.find_first_of("\\/") == std::string::npos
            ? nlohmann::json(wert.text) : nlohmann::json("[Pfadreferenz ausgeblendet]");
    case Json::Typ::liste: {
        auto result = nlohmann::json::array();
        for (const auto& element : wert.liste) result.push_back(json_wert(element, pfade_ausblenden));
        return result;
    }
    case Json::Typ::objekt: {
        auto result = nlohmann::json::object();
        for (const auto& feld : wert.objekt) result[feld.first] = json_wert(feld.second, pfade_ausblenden);
        return result;
    }
    }
    return nullptr;
}

const Json* optional(const Json& wert, const std::string& feld)
{
    const auto fund = wert.objekt.find(feld);
    return fund == wert.objekt.end() ? nullptr : &fund->second;
}

const Json& pflicht(const Json& wert, const std::string& feld, const std::string& ort, int id = 2)
{
    const auto* kind = optional(wert, feld);
    if (!kind) fehler(id, ort, "Pflichtfeld fehlt: " + feld);
    return *kind;
}

void objekt(const Json& wert, std::initializer_list<const char*> felder,
            const std::string& ort, int id = 2)
{
    if (wert.typ != Json::Typ::objekt) fehler(id, ort, "Objekt erwartet");
    for (const auto& feld : wert.objekt) {
        if (std::none_of(felder.begin(), felder.end(),
                         [&](const char* erlaubt) { return feld.first == erlaubt; }))
            fehler(id, ort, "Unbekanntes Feld: " + feld.first);
    }
}

std::string text(const Json& wert, const std::string& ort, int id = 2)
{
    if (wert.typ != Json::Typ::text || wert.text.empty() || wert.text.find('\0') != std::string::npos)
        fehler(id, ort, "Nicht leere Zeichenkette ohne NUL erwartet");
    return wert.text;
}

// Darstellungsfarbe "#rrggbb" als RGB-Anteile 0..1; nullopt bei ungültigem Text.
std::optional<std::array<double, 3>> farbe_hex(const Json& wert)
{
    if (wert.typ != Json::Typ::text || wert.text.size() != 7 || wert.text[0] != '#') return std::nullopt;
    std::array<double, 3> rgb{};
    for (std::size_t i = 0; i < 3; ++i) {
        unsigned kanal = 0;
        const char* anfang = wert.text.data() + 1 + 2 * i;
        const auto [ende, ec] = std::from_chars(anfang, anfang + 2, kanal, 16);
        if (ec != std::errc{} || ende != anfang + 2) return std::nullopt;
        rgb[i] = kanal / 255.;
    }
    return rgb;
}

constexpr const char* OBERFLAECHEN[] = {"matt", "glaenzend", "metall", "chrom", "lack", "glas",
    "leuchtend", "schillernd", "facettiert", "drahtgitter", "perlmutt", "unsichtbar",
    "stoff", "holz", "metall_gebuerstet"};

// Darstellungsoberfläche; nullopt bei unbekanntem Wert.
std::optional<std::string> oberflaeche_text(const Json& wert)
{
    if (wert.typ != Json::Typ::text) return std::nullopt;
    for (const char* art : OBERFLAECHEN)
        if (wert.text == art) return wert.text;
    return std::nullopt;
}

double zahl(const Json& wert, const std::string& ort, bool positiv)
{
    if (wert.typ != Json::Typ::zahl || !std::isfinite(wert.zahl) || (positiv && wert.zahl <= 0.))
        fehler(4, ort, "Endliche" + std::string(positiv ? " positive" : "") + " Zahl erwartet");
    return wert.zahl;
}

Vektor3 vektor3_lesen(const Json& wert, const std::string& ort)
{
    if (wert.typ != Json::Typ::liste || wert.liste.size() != 3)
        fehler(2, ort, "Drei Zahlen erwartet");
    Vektor3 result{};
    for (std::size_t i = 0; i < result.size(); ++i)
        result[i] = zahl(wert.liste[i], ort, false);
    return result;
}

double norm(const Vektor3& wert)
{
    return std::sqrt(wert[0]*wert[0] + wert[1]*wert[1] + wert[2]*wert[2]);
}

Vektor3 normieren(Vektor3 wert, const std::string& ort)
{
    const double laenge = norm(wert);
    if (!std::isfinite(laenge) || laenge <= 1e-12)
        fehler(2, ort, "Vektor darf nicht null sein");
    for (auto& anteil : wert) anteil /= laenge;
    return wert;
}

Bewegungsdefinition bewegung_lesen(const Json& wert, const std::string& ort)
{
    if (wert.typ != Json::Typ::objekt) fehler(2, ort, "Objekt erwartet");
    Bewegungsdefinition result;
    const auto art = text(pflicht(wert, "art", ort), ort + ".art");
    if (art == "drehen") {
        objekt(wert, {"art", "achse_modell", "drehpunkt_modell_m", "mitbewegte_elemente"}, ort);
        result.art = Bewegungsart::drehen;
        result.achse_modell = normieren(
            vektor3_lesen(pflicht(wert, "achse_modell", ort), ort + ".achse_modell"),
            ort + ".achse_modell");
        result.drehpunkt_modell_m =
            vektor3_lesen(pflicht(wert, "drehpunkt_modell_m", ort), ort + ".drehpunkt_modell_m");
    } else if (art == "heben") {
        objekt(wert, {"art", "richtung_modell", "mitbewegte_elemente"}, ort);
        result.art = Bewegungsart::heben;
        result.richtung_modell = normieren(
            vektor3_lesen(pflicht(wert, "richtung_modell", ort), ort + ".richtung_modell"),
            ort + ".richtung_modell");
    } else {
        fehler(2, ort + ".art", "Erwartet: drehen oder heben");
    }
    if (const auto* elemente = optional(wert, "mitbewegte_elemente")) {
        if (elemente->typ != Json::Typ::liste)
            fehler(2, ort + ".mitbewegte_elemente", "Liste erwartet");
        std::set<std::string> ids;
        for (const auto& element : elemente->liste) {
            const auto id = text(element, ort + ".mitbewegte_elemente");
            if (!ids.insert(id).second)
                fehler(2, ort + ".mitbewegte_elemente", "Element-ID mehrfach angegeben: " + id);
            result.mitbewegte_elemente.push_back(id);
        }
    }
    return result;
}

std::string status_lesen(const Json& wert, const std::string& ort)
{
    const auto status = text(wert, ort, 3);
    if (status != "validiert" && status != "abgeleitet" && status != "geschaetzt")
        fehler(3, ort, "Erwartet: validiert, abgeleitet oder geschaetzt");
    return status;
}

HydrodynamikParameter parameter_lesen(const Json& wert, const std::string& ort)
{
    objekt(wert, {"wert", "einheit", "status", "quelle", "gueltigkeit", "unsicherheit"}, ort, 3);
    HydrodynamikParameter result;
    result.wert = zahl(pflicht(wert, "wert", ort, 3), ort + ".wert", false);
    result.einheit = text(pflicht(wert, "einheit", ort, 3), ort + ".einheit", 3);
    result.status = status_lesen(pflicht(wert, "status", ort, 3), ort + ".status");
    result.quelle = text(pflicht(wert, "quelle", ort, 3), ort + ".quelle", 3);
    result.gueltigkeit = text(pflicht(wert, "gueltigkeit", ort, 3), ort + ".gueltigkeit", 3);
    result.unsicherheit = text(pflicht(wert, "unsicherheit", ort, 3), ort + ".unsicherheit", 3);
    if (result.status == "geschaetzt" &&
        (result.quelle.find("Annahme") == std::string::npos &&
         result.quelle.find("Schaetzung") == std::string::npos &&
         result.quelle.find("Schätzung") == std::string::npos))
        fehler(3, ort + ".quelle", "Geschaetzte Werte muessen als Schaetzung/Annahme erkennbar belegt sein");
    return result;
}

HydrodynamikKraftmodell kraftmodell_lesen(const Json& wert, const std::string& ort)
{
    objekt(wert, {"methode", "parameter", "kennfeld", "widerstandsanteile",
        "gueltigkeit", "quelle"}, ort, 3);
    HydrodynamikKraftmodell result;
    result.methode = text(pflicht(wert, "methode", ort, 3), ort + ".methode", 3);
    result.gueltigkeit = text(pflicht(wert, "gueltigkeit", ort, 3), ort + ".gueltigkeit", 3);
    result.quelle = text(pflicht(wert, "quelle", ort, 3), ort + ".quelle", 3);
    const auto& parameter = pflicht(wert, "parameter", ort, 3);
    if (parameter.typ != Json::Typ::objekt) fehler(3, ort + ".parameter", "Objekt erwartet");
    for (const auto& entry : parameter.objekt)
        result.parameter.emplace(entry.first, parameter_lesen(entry.second, ort + ".parameter." + entry.first));
    const auto& kennfeld = pflicht(wert, "kennfeld", ort, 3);
    if (kennfeld.typ != Json::Typ::liste) fehler(3, ort + ".kennfeld", "Liste erwartet");
    for (std::size_t i = 0; i < kennfeld.liste.size(); ++i) {
        const auto& punkt = kennfeld.liste[i];
        const auto punktort = ort + ".kennfeld[" + std::to_string(i) + "]";
        objekt(punkt, {"anstellwinkel_rad", "reynolds_zahl", "cl", "cd", "status",
            "quelle", "gueltigkeit", "unsicherheit"}, punktort, 3);
        HydrodynamikKennfeldpunkt p;
        p.anstellwinkel_rad = zahl(pflicht(punkt, "anstellwinkel_rad", punktort, 3),
            punktort + ".anstellwinkel_rad", false);
        p.reynolds_zahl = zahl(pflicht(punkt, "reynolds_zahl", punktort, 3),
            punktort + ".reynolds_zahl", true);
        p.cl = zahl(pflicht(punkt, "cl", punktort, 3), punktort + ".cl", false);
        p.cd = zahl(pflicht(punkt, "cd", punktort, 3), punktort + ".cd", false);
        if (p.cd < 0.) fehler(3, punktort + ".cd", "Widerstandsbeiwert darf nicht negativ sein");
        p.status = status_lesen(pflicht(punkt, "status", punktort, 3), punktort + ".status");
        p.quelle = text(pflicht(punkt, "quelle", punktort, 3), punktort + ".quelle", 3);
        p.gueltigkeit = text(pflicht(punkt, "gueltigkeit", punktort, 3), punktort + ".gueltigkeit", 3);
        p.unsicherheit = text(pflicht(punkt, "unsicherheit", punktort, 3), punktort + ".unsicherheit", 3);
        if (p.status == "geschaetzt" &&
            (p.quelle.find("Annahme") == std::string::npos &&
             p.quelle.find("Schaetzung") == std::string::npos &&
             p.quelle.find("Schätzung") == std::string::npos))
            fehler(3, punktort + ".quelle", "Geschaetzte Kennfeldwerte muessen als Schaetzung/Annahme erkennbar belegt sein");
        if (std::any_of(result.kennfeld.begin(), result.kennfeld.end(), [&](const auto& prior) {
            return prior.anstellwinkel_rad == p.anstellwinkel_rad && prior.reynolds_zahl == p.reynolds_zahl;
        }))
            fehler(3, punktort, "Doppelter Kennfeldpunkt");
        result.kennfeld.push_back(std::move(p));
    }
    if (!result.kennfeld.empty()) {
        std::set<double> alpha_axis, reynolds_axis;
        for (const auto& point : result.kennfeld) {
            alpha_axis.insert(point.anstellwinkel_rad);
            reynolds_axis.insert(point.reynolds_zahl);
        }
        if (alpha_axis.size() > std::numeric_limits<std::size_t>::max() / reynolds_axis.size() ||
            alpha_axis.size() * reynolds_axis.size() != result.kennfeld.size())
            fehler(3, ort + ".kennfeld", "Kennfeld muss ein vollstaendiges Rechteck bilden");
        for (const double alpha : alpha_axis)
            for (const double reynolds : reynolds_axis)
                if (std::none_of(result.kennfeld.begin(), result.kennfeld.end(),
                    [&](const auto& point) {
                        return point.anstellwinkel_rad == alpha && point.reynolds_zahl == reynolds;
                    }))
                    fehler(3, ort + ".kennfeld", "Kennfeld muss ein vollstaendiges Rechteck bilden");
    }
    if (const auto* anteile = optional(wert, "widerstandsanteile")) {
        if (anteile->typ != Json::Typ::liste) fehler(3, ort + ".widerstandsanteile", "Liste erwartet");
        for (const auto& anteil : anteile->liste) {
            const auto name = text(anteil, ort + ".widerstandsanteile", 3);
            if (name != "gesamt" && name != "profil" && name != "druck" &&
                name != "hautreibung" && name != "induzierter")
                fehler(3, ort + ".widerstandsanteile", "Unbekannter Widerstandsanteil: " + name);
            if (std::find(result.widerstandsanteile.begin(), result.widerstandsanteile.end(), name) !=
                result.widerstandsanteile.end())
                fehler(3, ort + ".widerstandsanteile", "Widerstandsanteil mehrfach angegeben: " + name);
            result.widerstandsanteile.push_back(name);
        }
    }
    if (std::find(result.widerstandsanteile.begin(), result.widerstandsanteile.end(), "gesamt") !=
            result.widerstandsanteile.end() && result.widerstandsanteile.size() != 1)
        fehler(3, ort + ".widerstandsanteile",
            "Anteil gesamt darf nicht mit Teilanteilen kombiniert werden");
    if (!result.kennfeld.empty() && result.widerstandsanteile.empty())
        fehler(3, ort + ".widerstandsanteile",
            "Kennfeld muss enthaltene Widerstandsanteile explizit benennen");
    return result;
}

ElementHydrodynamikDaten hydrodynamikdaten_lesen(const Json& wert, const std::string& ort)
{
    objekt(wert, {"rolle", "modelle", "materialreferenz"}, ort, 3);
    ElementHydrodynamikDaten result;
    if (const auto* modelle = optional(wert, "modelle")) {
        objekt(*modelle, {"hautreibung", "druckwiderstand", "seitenkraft"}, ort + ".modelle", 3);
        for (const auto& entry : modelle->objekt) {
            const auto model_ort = ort + ".modelle." + entry.first;
            auto model = kraftmodell_lesen(entry.second, model_ort);
            if (entry.first == "hautreibung" &&
                model.methode != "ittc_1957_mit_laminarer_naeherung" &&
                model.methode != "ittc_1957")
                fehler(3, model_ort + ".methode", "Unbekannte Hautreibungs-Methode");
            if (entry.first == "druckwiderstand" &&
                model.methode != "geschaetzter_normaler_formwiderstand" &&
                model.methode != "platten_normal_drag")
                fehler(3, model_ort + ".methode", "Unbekannte Druckwiderstands-Methode");
            if (entry.first == "seitenkraft" &&
                model.methode != "geschaetzte_endliche_streckungspolare" &&
                model.methode != "polare_2d")
                fehler(3, model_ort + ".methode", "Unbekannte Seitenkraft-Methode");
            if (entry.first != "seitenkraft" &&
                (!model.kennfeld.empty() || !model.widerstandsanteile.empty()))
                fehler(3, model_ort,
                    "Kennfeld und Widerstandsanteile sind nur fuer seitenkraft zulaessig");
            const std::vector<std::string> erlaubte_parameter = entry.first == "druckwiderstand" ?
                std::vector<std::string>{"cd"} :
                entry.first == "seitenkraft" ?
                std::vector<std::string>{"streckung", "wirkungsgrad", "cd0", "cl_max"} :
                std::vector<std::string>{};
            for (const auto& parameter : model.parameter) {
                if (std::find(erlaubte_parameter.begin(), erlaubte_parameter.end(), parameter.first) ==
                    erlaubte_parameter.end())
                    fehler(3, model_ort + ".parameter." + parameter.first,
                        "Unbekannter oder nicht verwendeter Hydrodynamikparameter");
                if (parameter.second.einheit != "1")
                    fehler(3, model_ort + ".parameter." + parameter.first + ".einheit",
                        "Dieser Parameter erwartet die dimensionslose Einheit 1");
            }
            if (entry.first == "seitenkraft" &&
                ((model.methode == "polare_2d" && model.kennfeld.empty()) ||
                 (!model.kennfeld.empty() && model.methode != "polare_2d")))
                fehler(3, model_ort,
                    "polare_2d und nichtleeres Kennfeld muessen gemeinsam verwendet werden");
            result.modelle.emplace(entry.first, std::move(model));
        }
    }
    if (const auto* material = optional(wert, "materialreferenz"))
        result.materialreferenz = text(*material, ort + ".materialreferenz", 3);
    return result;
}

using L = long double;
using V = std::array<L, 3>;
using Matrix = std::array<L, 9>;

V vektor(const Vektor3& p) { return {p[0], p[1], p[2]}; }
V minus(const V& a, const V& b) { return {a[0]-b[0], a[1]-b[1], a[2]-b[2]}; }
L skalar(const V& a, const V& b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
V kreuz(const V& a, const V& b)
{
    return {a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]};
}
L norm(const V& p) { return std::hypot(p[0], p[1], p[2]); }

double endlich(L wert, const std::string& ort, int id = 4)
{
    if (!std::isfinite(wert) || std::abs(wert) > std::numeric_limits<double>::max())
        fehler(id, ort, "Numerischer Ueberlauf");
    return static_cast<double>(wert);
}

struct Box {
    V unten, oben;
};

Box box(const Mesh& mesh)
{
    Box b{vektor(mesh.punkte.front()), vektor(mesh.punkte.front())};
    for (const auto& p : mesh.punkte) for (std::size_t i = 0; i < 3; ++i) {
        b.unten[i] = std::min(b.unten[i], static_cast<L>(p[i]));
        b.oben[i] = std::max(b.oben[i], static_cast<L>(p[i]));
    }
    return b;
}

bool ueberlappt(const Box& a, const Box& b)
{
    for (std::size_t i = 0; i < 3; ++i)
        if (a.oben[i] < b.unten[i] || b.oben[i] < a.unten[i]) return false;
    return true;
}

bool innen_ueberlappt(const Box& a, const Box& b)
{
    for (std::size_t i = 0; i < 3; ++i)
        if (a.oben[i] <= b.unten[i] || b.oben[i] <= a.unten[i]) return false;
    return true;
}

double obj_zahl(const std::string& token, const std::string& ort)
{
    double wert = 0.;
    const char* start = token.data();
    if (!token.empty() && token.front() == '+') ++start;
    const auto ergebnis = std::from_chars(start, token.data() + token.size(), wert);
    if (ergebnis.ec != std::errc{} || ergebnis.ptr != token.data() + token.size() || !std::isfinite(wert))
        fehler(6, ort, "Ungueltige/nicht endliche OBJ-Zahl: " + token);
    return wert;
}

std::size_t obj_index(std::string_view token, std::size_t anzahl, const std::string& ort)
{
    long long index = 0;
    const auto ergebnis = std::from_chars(token.data(), token.data() + token.size(), index);
    if (ergebnis.ec != std::errc{} || ergebnis.ptr != token.data() + token.size() || index == 0)
        fehler(6, ort, "Ungueltiger OBJ-Index");
    if (index > 0 && static_cast<unsigned long long>(index) <= anzahl)
        return static_cast<std::size_t>(index - 1);
    if (index < 0 && index != std::numeric_limits<long long>::min() &&
        static_cast<unsigned long long>(-index) <= anzahl)
        return anzahl - static_cast<std::size_t>(-index);
    fehler(6, ort, "OBJ-Index ausserhalb des gueltigen Bereichs");
}

Mesh obj_lesen(const std::filesystem::path& datei, const std::string& kontext, Fingerabdruck& fassung,
              const std::optional<std::string>& auswahl = std::nullopt,
              std::vector<QuellMesh>* objekte = nullptr, bool darstellung = false)
{
    const std::string ort = kontext + ", Mesh " + datei.u8string();
    const auto inhalt = lesen(datei, ort);
    fassung.datei(inhalt);
    std::istringstream strom(inhalt);
    Mesh mesh;
    struct Objekt {
        std::set<std::size_t> punkte;
        std::vector<std::array<std::size_t, 3>> dreiecke;
    };
    std::map<std::string, Objekt> teile;
    std::string aktuell;
    std::string zeile;
    std::size_t nummer = 0, texturen = 0, normalen = 0;
    while (std::getline(strom, zeile)) {
        ++nummer;
        const std::string stelle = ort + ":" + std::to_string(nummer);
        zeile.erase(zeile.find('#') == std::string::npos ? zeile.size() : zeile.find('#'));
        std::istringstream tokens(zeile);
        tokens.imbue(std::locale::classic());
        std::string art, token;
        if (!(tokens >> art)) continue;
        if (darstellung && (art == "vt" || art == "vn")) continue;
        if (art == "v" || art == "vt" || art == "vn") {
            std::vector<double> werte;
            while ((!darstellung || werte.size() < 3) && (tokens >> token))
                werte.push_back(obj_zahl(token, stelle));
            if ((art != "vt" && werte.size() != 3) ||
                (art == "vt" && (werte.empty() || werte.size() > 3)))
                fehler(6, stelle, "OBJ-Koordinatenzahl nicht unterstuetzt (v/vn: 3, vt: 1..3)");
            if (art == "v") {
                if (!aktuell.empty()) teile.at(aktuell).punkte.insert(mesh.punkte.size());
                mesh.punkte.push_back({werte[0], werte[1], werte[2]});
            }
            else if (art == "vt") ++texturen;
            else ++normalen;
        } else if (art == "f") {
            std::vector<std::size_t> indizes;
            while (tokens >> token) {
                const std::size_t slash = token.find('/');
                indizes.push_back(obj_index(std::string_view(token).substr(0, slash), mesh.punkte.size(), stelle));
                if (slash != std::string::npos && !darstellung) {
                    const auto zweiter = token.find('/', slash + 1);
                    const auto uv = std::string_view(token).substr(slash + 1,
                        zweiter == std::string::npos ? zweiter : zweiter - slash - 1);
                    if (!uv.empty()) obj_index(uv, texturen, stelle);
                    else if (zweiter == std::string::npos) fehler(6, stelle, "Leerer Texturindex");
                    if (zweiter != std::string::npos)
                        obj_index(std::string_view(token).substr(zweiter + 1), normalen, stelle);
                }
            }
            if (indizes.size() < 3 || (!darstellung && indizes.size() != 3))
                fehler(6, stelle, "Nur OBJ-Dreiecke erlaubt; Polygone vorher explizit triangulieren");
            for (std::size_t i = 1; i + 1 < indizes.size(); ++i) {
                const std::array<std::size_t, 3> dreieck{indizes[0], indizes[i], indizes[i+1]};
                mesh.dreiecke.push_back(dreieck);
                if (!aktuell.empty()) {
                    auto& teil = teile.at(aktuell);
                    teil.dreiecke.push_back(dreieck);
                    teil.punkte.insert(dreieck.begin(), dreieck.end());
                } else if (objekte || auswahl) fehler(6, stelle, "Flaeche ohne explizites o-Objekt");
            }
        } else if (art == "o" && (objekte || auswahl)) {
            std::string extra;
            if (!(tokens >> aktuell) || (tokens >> extra))
                fehler(6, stelle, "Genau eine nichtleere o-Mesh-ID erwartet");
            if (!teile.emplace(aktuell, Objekt{}).second)
                fehler(6, stelle, "Mesh-ID mehrfach vergeben: " + aktuell);
        } else if (art != "o" && art != "g" && art != "s" && art != "usemtl" && art != "mtllib") {
            if (!darstellung) fehler(6, stelle, "Nicht unterstuetzter OBJ-Datensatz: " + art);
        }
    }
    if (mesh.punkte.empty() || mesh.dreiecke.empty()) fehler(6, ort, "Leeres Mesh");
    if (auswahl || objekte) {
        std::optional<Mesh> gewaehlt;
        for (const auto& [id, teil] : teile) {
            if (teil.dreiecke.empty()) fehler(6, ort, "Leeres o-Mesh: " + id);
            Mesh lokal;
            std::map<std::size_t, std::size_t> indizes;
            for (const auto index : teil.punkte) {
                indizes[index] = lokal.punkte.size();
                lokal.punkte.push_back(mesh.punkte[index]);
            }
            for (const auto& t : teil.dreiecke)
                lokal.dreiecke.push_back({indizes.at(t[0]), indizes.at(t[1]), indizes.at(t[2])});
            if (auswahl && id == *auswahl) gewaehlt = lokal;
            if (objekte) objekte->push_back({id, "OBJ.o:" + id, std::move(lokal)});
        }
        if (auswahl) {
            if (!gewaehlt) fehler(6, ort, "Mesh-ID fehlt: " + *auswahl);
            return std::move(*gewaehlt);
        }
    }
    return mesh;
}

struct Momente {
    L mass = 0.;
    V erste{};
    Matrix zweite{};
};

void simplex(Momente& m, const std::array<V, 3>& p, L gewicht, bool solid)
{
    m.mass += gewicht;
    V summe{};
    for (const auto& punkt : p) for (std::size_t i = 0; i < 3; ++i) summe[i] += punkt[i];
    for (std::size_t i = 0; i < 3; ++i) {
        m.erste[i] += gewicht * summe[i] / (solid ? 4.L : 3.L);
        for (std::size_t j = 0; j < 3; ++j) {
            L produkt = summe[i] * summe[j];
            for (const auto& punkt : p) produkt += punkt[i] * punkt[j];
            m.zweite[3*i+j] += gewicht * produkt / (solid ? 20.L : 12.L);
        }
    }
}

// The signed tetrahedra share a nearby reference, not the world origin.
Momente integrieren(const Mesh& mesh, const V& bezug, bool solid)
{
    Momente m;
    for (const auto& t : mesh.dreiecke) {
        const std::array<V, 3> p{minus(vektor(mesh.punkte[t[0]]), bezug),
                                minus(vektor(mesh.punkte[t[1]]), bezug),
                                minus(vektor(mesh.punkte[t[2]]), bezug)};
        const L gewicht = solid ? skalar(p[0], kreuz(p[1], p[2])) / 6.L :
            norm(kreuz(minus(p[1], p[0]), minus(p[2], p[0]))) / 2.L;
        simplex(m, p, gewicht, solid);
    }
    return m;
}

struct Kante {
    std::vector<std::size_t> flaechen;
    int richtung = 0;
};

// Detect non-topological intersections. Shared edges/vertices alone are legal.
bool schnitt(const std::array<V, 3>& a, const std::array<V, 3>& b)
{
    const auto test = [](const std::array<V, 3>& t, const std::array<V, 3>& u) {
        const V n = kreuz(minus(t[1], t[0]), minus(t[2], t[0]));
        const L n_laenge = norm(n);
        const L skala = std::max({norm(minus(t[1], t[0])), norm(minus(t[2], t[0])),
                                  norm(minus(u[1], u[0])), norm(minus(u[2], u[0]))});
        const L eps = 1e-12L * skala;
        const auto innen = [&](const V& p, bool strikt) {
            for (std::size_t k = 0; k < 3; ++k) {
                const V rand = minus(t[(k+1)%3], t[k]);
                const L seite = skalar(kreuz(rand, minus(p, t[k])), n) / n_laenge;
                if (strikt ? seite <= eps * norm(rand) : seite < -eps * norm(rand)) return false;
            }
            return true;
        };
        for (std::size_t k = 0; k < 3; ++k) {
            const V& p = u[k];
            const V& q = u[(k+1)%3];
            const L dp = skalar(minus(p, t[0]), n) / n_laenge;
            const L dq = skalar(minus(q, t[0]), n) / n_laenge;
            if (std::abs(dp) <= eps && innen(p, true)) return true;
            if (std::abs(dp) <= eps && innen(p, false) &&
                std::none_of(t.begin(), t.end(), [&](const V& ecke) { return norm(minus(p, ecke)) <= eps; }))
                return true;
            if ((dp > eps && dq < -eps) || (dp < -eps && dq > eps)) {
                const L f = dp / (dp - dq);
                V x{};
                for (std::size_t j = 0; j < 3; ++j) x[j] = p[j] + f * (q[j] - p[j]);
                if (innen(x, false)) return true;
            }
            if (std::abs(dp) <= eps && std::abs(dq) <= eps) {
                V mitte{};
                for (std::size_t j = 0; j < 3; ++j) mitte[j] = p[j] / 2.L + q[j] / 2.L;
                if (innen(mitte, true)) return true;
                // Coplanar edges crossing strictly inside both segments.
                for (std::size_t j = 0; j < 3; ++j) {
                    // Edges sharing an endpoint cannot cross strictly inside both segments;
                    // near-collinear pairs would otherwise trip the parameter tolerance.
                    if (p == t[j] || p == t[(j+1)%3] || q == t[j] || q == t[(j+1)%3]) continue;
                    const V r = minus(q, p), s = minus(t[(j+1)%3], t[j]);
                    const L nenner = skalar(kreuz(r, s), n) / n_laenge;
                    if (std::abs(nenner) <= eps * std::max(norm(r), norm(s))) continue;
                    const V delta = minus(t[j], p);
                    const L f = (skalar(kreuz(delta, s), n) / n_laenge) / nenner;
                    const L g = (skalar(kreuz(delta, r), n) / n_laenge) / nenner;
                    if (f > 1e-12L && f < 1.L-1e-12L && g > 1e-12L && g < 1.L-1e-12L)
                        return true;
                }
            }
        }
        return false;
    };
    return test(a, b) || test(b, a);
}

void validieren(const Mesh& mesh, bool geschlossen, const std::string& ort)
{
    const int material_id = geschlossen ? 6 : 8;
    std::map<std::pair<std::size_t, std::size_t>, Kante> kanten;
    std::set<std::array<Vektor3, 3>> flaechen;
    std::vector<std::vector<std::size_t>> nachbarn(mesh.dreiecke.size());
    std::vector<std::vector<std::size_t>> ecken(mesh.punkte.size());
    std::vector<Box> boxen;
    std::vector<std::array<V, 3>> geometrie;
    if (geschlossen) {
        std::set<Vektor3> positionen;
        for (const auto& p : mesh.punkte)
            if (!positionen.insert(p).second) fehler(6, ort, "Mehrfach indizierter Volumenpunkt");
    }
    for (std::size_t f = 0; f < mesh.dreiecke.size(); ++f) {
        const auto& t = mesh.dreiecke[f];
        std::array<Vektor3, 3> kanonisch{};
        std::array<V, 3> p{};
        for (std::size_t i = 0; i < 3; ++i) {
            if (t[i] >= mesh.punkte.size()) fehler(6, ort, "Dreiecksindex ausserhalb des Meshes");
            kanonisch[i] = mesh.punkte[t[i]];
            p[i] = vektor(kanonisch[i]);
            ecken[t[i]].push_back(f);
        }
        const V a = minus(p[1], p[0]), b = minus(p[2], p[0]);
        const L flaeche = norm(kreuz(a, b));
        const L skala = norm(a) * norm(b);
        if (!std::isfinite(flaeche) || !std::isfinite(skala) || skala == 0.L ||
            flaeche <= 1e-14L * skala)
            fehler(6, ort, "Degeneriertes/numerisch ungeeignetes Dreieck " + std::to_string(f));
        std::sort(kanonisch.begin(), kanonisch.end());
        if (!flaechen.insert(kanonisch).second) fehler(material_id, ort, "Doppelte Material-/Volumenflaeche");
        Box bnd{p[0], p[0]};
        for (const auto& punkt : p) for (std::size_t i = 0; i < 3; ++i) {
            bnd.unten[i] = std::min(bnd.unten[i], punkt[i]);
            bnd.oben[i] = std::max(bnd.oben[i], punkt[i]);
        }
        boxen.push_back(bnd);
        geometrie.push_back(p);
        for (std::size_t i = 0; i < 3; ++i) {
            const auto x = t[i], y = t[(i+1)%3];
            auto& k = kanten[std::minmax(x, y)];
            k.flaechen.push_back(f);
            k.richtung += x < y ? 1 : -1;
        }
    }
    for (const auto& paar : kanten) {
        const auto& k = paar.second;
        if (k.flaechen.size() > 2 || (geschlossen && k.flaechen.size() != 2) ||
            (k.flaechen.size() == 2 && k.richtung != 0))
            fehler(material_id, ort, "Offene/nicht-manifolde oder inkonsistent orientierte Kante");
        if (k.flaechen.size() == 2) {
            nachbarn[k.flaechen[0]].push_back(k.flaechen[1]);
            nachbarn[k.flaechen[1]].push_back(k.flaechen[0]);
        }
    }
    if (geschlossen) {
        for (std::size_t v = 0; v < ecken.size(); ++v) {
            if (ecken[v].empty()) fehler(6, ort, "Unbenutzter Volumenpunkt");
            std::set<std::size_t> offen(ecken[v].begin(), ecken[v].end());
            std::vector<std::size_t> stapel{*offen.begin()};
            offen.erase(stapel.back());
            while (!stapel.empty()) {
                const auto f = stapel.back(); stapel.pop_back();
                for (const auto n : nachbarn[f])
                    if (offen.erase(n)) stapel.push_back(n);
            }
            if (!offen.empty()) fehler(6, ort, "Nicht-manifolder Volumenpunkt");
        }
    }
    std::size_t achse = 0;
    const auto gesamtbox = box(mesh);
    for (std::size_t i = 1; i < 3; ++i)
        if (gesamtbox.oben[i] - gesamtbox.unten[i] >
            gesamtbox.oben[achse] - gesamtbox.unten[achse]) achse = i;
    std::vector<std::size_t> reihenfolge(geometrie.size());
    std::iota(reihenfolge.begin(), reihenfolge.end(), std::size_t{0});
    std::sort(reihenfolge.begin(), reihenfolge.end(), [&](std::size_t a, std::size_t b) {
        if (boxen[a].unten[achse] != boxen[b].unten[achse])
            return boxen[a].unten[achse] < boxen[b].unten[achse];
        return a < b;
    });
    std::vector<std::size_t> aktiv;
    for (const auto i : reihenfolge) {
        aktiv.erase(std::remove_if(aktiv.begin(), aktiv.end(), [&](std::size_t j) {
            return boxen[j].oben[achse] < boxen[i].unten[achse];
        }), aktiv.end());
        for (const auto j : aktiv)
            if (ueberlappt(boxen[i], boxen[j]) && schnitt(geometrie[i], geometrie[j]))
                fehler(material_id, ort, "Sich schneidende Meshflaechen");
        aktiv.push_back(i);
    }
    if (!geschlossen) return;
    std::vector<bool> gesehen(mesh.dreiecke.size(), false);
    std::vector<Box> komponenten;
    for (std::size_t start = 0; start < gesehen.size(); ++start) {
        if (gesehen[start]) continue;
        gesehen[start] = true;
        std::vector<std::size_t> stapel{start};
        const V bezug = geometrie[start][0];
        L volumen = 0.L;
        Box bnd = boxen[start];
        while (!stapel.empty()) {
            const auto f = stapel.back(); stapel.pop_back();
            const auto& p = geometrie[f];
            volumen += skalar(minus(p[0], bezug),
                kreuz(minus(p[1], bezug), minus(p[2], bezug))) / 6.L;
            for (std::size_t i = 0; i < 3; ++i) {
                bnd.unten[i] = std::min(bnd.unten[i], boxen[f].unten[i]);
                bnd.oben[i] = std::max(bnd.oben[i], boxen[f].oben[i]);
            }
            for (const auto n : nachbarn[f]) if (!gesehen[n]) {
                gesehen[n] = true; stapel.push_back(n);
            }
        }
        if (!std::isfinite(volumen) || volumen <= 0.L)
            fehler(6, ort, "Volumenkomponente nicht nach aussen orientiert oder ohne positives Volumen");
        for (const auto& andere : komponenten) if (innen_ueberlappt(bnd, andere))
            fehler(6, ort, "Ueberschneidungsfreiheit nicht nachgewiesen (konservative AABB-Pruefung)"
                "; getrennte Volumenkomponenten");
        komponenten.push_back(bnd);
    }
}

// Generalized winding number (Van Oosterom/Strackee); NaN if p lies on a vertex.
L windung(const Mesh& mesh, const V& p)
{
    L summe = 0.L;
    for (const auto& t : mesh.dreiecke) {
        const V a = minus(vektor(mesh.punkte[t[0]]), p);
        const V b = minus(vektor(mesh.punkte[t[1]]), p);
        const V c = minus(vektor(mesh.punkte[t[2]]), p);
        const L la = norm(a), lb = norm(b), lc = norm(c);
        if (la == 0.L || lb == 0.L || lc == 0.L) return std::numeric_limits<L>::quiet_NaN();
        const L zaehler = skalar(a, kreuz(b, c));
        const L nenner = la*lb*lc + skalar(a, b)*lc + skalar(b, c)*la + skalar(c, a)*lb;
        summe += 2.L * std::atan2(zaehler, nenner);
    }
    return summe / (4.L * 3.14159265358979323846L);
}

// True if a vertex of some connected component of a lies inside b. Ambiguous
// points (on or extremely close to b's surface) are skipped; if a component has
// no unambiguous vertex, nesting is assumed conservatively.
bool enthalten(const Mesh& a, const Mesh& b)
{
    std::vector<std::size_t> eltern(a.punkte.size());
    std::iota(eltern.begin(), eltern.end(), std::size_t{0});
    const auto wurzel = [&](std::size_t i) {
        while (eltern[i] != i) i = eltern[i] = eltern[eltern[i]];
        return i;
    };
    for (const auto& t : a.dreiecke)
        for (std::size_t i = 1; i < 3; ++i) eltern[wurzel(t[i])] = wurzel(t[0]);
    std::map<std::size_t, std::vector<std::size_t>> komponenten;
    for (const auto& t : a.dreiecke) for (const auto i : t) {
        auto& liste = komponenten[wurzel(i)];
        if (liste.size() < 16 && std::find(liste.begin(), liste.end(), i) == liste.end())
            liste.push_back(i);
    }
    const auto bbox = box(b);
    for (const auto& [_, punkte] : komponenten) {
        bool entschieden = false;
        for (const auto i : punkte) {
            const V p = vektor(a.punkte[i]);
            bool aussen = false;
            for (std::size_t k = 0; k < 3; ++k)
                if (p[k] < bbox.unten[k] || p[k] > bbox.oben[k]) aussen = true;
            if (aussen) { entschieden = true; break; }
            const L w = windung(b, p);
            if (!std::isfinite(w) || std::abs(w - std::round(w)) > .1L) continue;
            if (std::abs(w) > .5L) return true;
            entschieden = true;
            break;
        }
        if (!entschieden) return true;
    }
    return false;
}

// Two closed volume meshes overlap if their surfaces intersect or one is nested in the other.
bool volumen_durchdringen(const Mesh& a, const Mesh& b)
{
    const auto ba = box(a), bb = box(b);
    if (!innen_ueberlappt(ba, bb)) return false;
    Box gemeinsam{};
    for (std::size_t i = 0; i < 3; ++i) {
        gemeinsam.unten[i] = std::max(ba.unten[i], bb.unten[i]);
        gemeinsam.oben[i] = std::min(ba.oben[i], bb.oben[i]);
    }
    struct Eintrag { Box b; std::array<V, 3> p; int netz; };
    std::vector<Eintrag> eintraege;
    for (int n = 0; n < 2; ++n) {
        const Mesh& m = n == 0 ? a : b;
        for (const auto& t : m.dreiecke) {
            Eintrag e{{}, {vektor(m.punkte[t[0]]), vektor(m.punkte[t[1]]), vektor(m.punkte[t[2]])}, n};
            e.b = {e.p[0], e.p[0]};
            for (const auto& punkt : e.p) for (std::size_t i = 0; i < 3; ++i) {
                e.b.unten[i] = std::min(e.b.unten[i], punkt[i]);
                e.b.oben[i] = std::max(e.b.oben[i], punkt[i]);
            }
            if (ueberlappt(e.b, gemeinsam)) eintraege.push_back(e);
        }
    }
    std::sort(eintraege.begin(), eintraege.end(), [](const Eintrag& x, const Eintrag& y) {
        return x.b.unten[0] < y.b.unten[0];
    });
    std::array<std::vector<std::size_t>, 2> aktiv;
    for (std::size_t i = 0; i < eintraege.size(); ++i) {
        const auto& e = eintraege[i];
        auto& andere = aktiv[1 - e.netz];
        andere.erase(std::remove_if(andere.begin(), andere.end(), [&](std::size_t j) {
            return eintraege[j].b.oben[0] < e.b.unten[0];
        }), andere.end());
        for (const auto j : andere)
            if (ueberlappt(e.b, eintraege[j].b) && schnitt(e.p, eintraege[j].p)) return true;
        aktiv[e.netz].push_back(i);
    }
    return enthalten(a, b) || enthalten(b, a);
}

std::filesystem::path referenz(const Json& wert, const std::filesystem::path& basis,
                              const std::string& ort, int id = 2)
{
    const std::string name = text(wert, ort, id);
    const auto pfad = std::filesystem::u8path(name);
    if (pfad.is_absolute() || pfad.has_root_name() || pfad.has_root_directory())
        fehler(id, ort, "Dateireferenz muss relativ zur Modelldatei sein");
    return basis / pfad;
}

void masseneigenschaften(Element& e, const Mesh& mesh, bool solid, L dichte,
                         const std::string& ort)
{
    SEGEL_MESSUNG("masseneigenschaften");
    const auto b = box(mesh);
    V bezug{};
    for (std::size_t i = 0; i < 3; ++i) bezug[i] = b.unten[i]/2.L + b.oben[i]/2.L;
    const auto m = integrieren(mesh, bezug, solid);
    if (!std::isfinite(m.mass) || m.mass <= 0.L) fehler(4, ort, "Masseintegral nicht positiv/endlich");
    e.masse_kg = endlich(m.mass * dichte, ort);
    if (e.masse_kg <= 0.) fehler(4, ort, "Masse nicht darstellbar");
    V mitte{};
    for (std::size_t i = 0; i < 3; ++i) {
        mitte[i] = m.erste[i] / m.mass;
        e.schwerpunkt_m[i] = endlich(bezug[i] + mitte[i], ort);
    }
    Matrix zentral{};
    for (std::size_t i = 0; i < 3; ++i) for (std::size_t j = 0; j < 3; ++j)
        zentral[3*i+j] = (m.zweite[3*i+j] - m.mass * mitte[i] * mitte[j]) * dichte;
    const L spur = zentral[0] + zentral[4] + zentral[8];
    for (std::size_t i = 0; i < 3; ++i) for (std::size_t j = 0; j < 3; ++j) {
        const L wert = (i == j ? spur : 0.L) - zentral[3*i+j];
        if (i == j && wert < 0.L) fehler(4, ort, "Numerisch ungueltige Traegheit");
        e.traegheit_kg_m2[3*i+j] = endlich(wert, ort);
    }
}

Mesh kompakt(const Mesh& mesh)
{
    Mesh result;
    std::map<std::size_t, std::size_t> indices;
    for (const auto& face : mesh.dreiecke) {
        std::array<std::size_t, 3> output{};
        for (std::size_t i = 0; i < 3; ++i) {
            const auto entry = indices.emplace(face[i], result.punkte.size());
            if (entry.second) result.punkte.push_back(mesh.punkte.at(face[i]));
            output[i] = entry.first->second;
        }
        result.dreiecke.push_back(output);
    }
    return result;
}

Mesh geometrie_ableiten(const Mesh& source, const Json& definition, const std::string& ort)
{
    objekt(definition, {"art", "z_m", "steigung_x", "steigung_y"}, ort, 9);
    const auto art = text(pflicht(definition, "art", ort, 9), ort, 9);
    const auto& height = pflicht(definition, "z_m", ort, 9);
    if (height.typ != Json::Typ::zahl || !std::isfinite(height.zahl))
        fehler(9, ort, "Endliche Schnitthoehe z_m erforderlich");
    const double z = height.zahl;
    const auto slope = [&](const char* field) {
        const auto* value = optional(definition, field);
        if (!value) return 0.;
        if (value->typ != Json::Typ::zahl || !std::isfinite(value->zahl))
            fehler(9, ort, "Endliche Ebenensteigung erforderlich");
        return value->zahl;
    };
    const double sx = slope("steigung_x"), sy = slope("steigung_y");
    const auto distance = [&](const Vektor3& p) { return p[2]-z-sx*p[0]-sy*p[1]; };
    const auto bounds = box(source);
    const double epsilon = 1e-9 * static_cast<double>(
        std::max<L>(1., bounds.oben[2] - bounds.unten[2]));
    Mesh result;
    result.punkte = source.punkte;
    if (art == "ohne_deckflaeche") {
        if (sx != 0. || sy != 0.) fehler(9, ort, "Deckflaechen muessen horizontal sein");
        for (const auto& face : source.dreiecke)
            if (!std::all_of(face.begin(), face.end(), [&](std::size_t i) {
                return std::abs(source.punkte[i][2] - z) <= epsilon;
            })) result.dreiecke.push_back(face);
        if (result.dreiecke.empty() || result.dreiecke.size() == source.dreiecke.size())
            fehler(9, ort, "Deckflaechenauswahl entfernt keine oder alle Flaechen");
        return kompakt(result);
    }
    if (art != "z_schnitt") fehler(9, ort, "Unbekannte Geometrieableitung: " + art);
    validieren(source, true, ort + ".quelle");
    if (std::none_of(source.punkte.begin(), source.punkte.end(),
            [&](const Vektor3& p) { return distance(p) < -epsilon; }) ||
        std::none_of(source.punkte.begin(), source.punkte.end(),
            [&](const Vektor3& p) { return distance(p) > epsilon; }))
        fehler(9, ort, "Schnittebene muss das Quellvolumen tatsaechlich teilen");
    std::map<std::pair<std::size_t, std::size_t>, std::size_t> intersections;
    const auto intersection = [&](std::size_t a, std::size_t b) {
        if (std::abs(distance(source.punkte[a])) <= epsilon) return a;
        if (std::abs(distance(source.punkte[b])) <= epsilon) return b;
        const auto key = std::minmax(a, b);
        const auto found = intersections.find(key);
        if (found != intersections.end()) return found->second;
        const auto& p = source.punkte[key.first];
        const auto& q = source.punkte[key.second];
        const double t = distance(p) / (distance(p) - distance(q));
        const auto index = result.punkte.size();
        Vektor3 point{p[0] + t*(q[0]-p[0]), p[1] + t*(q[1]-p[1]), 0.};
        point[2] = z + sx*point[0] + sy*point[1];
        result.punkte.push_back(point);
        intersections.emplace(key, index);
        return index;
    };
    for (std::size_t i = 0; i < source.punkte.size(); ++i)
        if (std::abs(distance(source.punkte[i])) <= epsilon)
            result.punkte[i][2] = z + sx*source.punkte[i][0] + sy*source.punkte[i][1];
    for (const auto& face : source.dreiecke) {
        std::vector<std::size_t> polygon;
        for (std::size_t i = 0; i < 3; ++i) {
            const auto a = face[i], b = face[(i+1)%3];
            const bool inside_a = distance(source.punkte[a]) <= epsilon;
            const bool inside_b = distance(source.punkte[b]) <= epsilon;
            if (inside_a) polygon.push_back(a);
            if (inside_a != inside_b) polygon.push_back(intersection(a, b));
        }
        polygon.erase(std::unique(polygon.begin(), polygon.end()), polygon.end());
        if (polygon.size() > 1 && polygon.front() == polygon.back()) polygon.pop_back();
        for (std::size_t i = 1; i+1 < polygon.size(); ++i)
            result.dreiecke.push_back({polygon[0], polygon[i], polygon[i+1]});
    }
    struct Edge { std::size_t a{}, b{}, count{}; };
    std::map<std::pair<std::size_t, std::size_t>, Edge> edges;
    for (const auto& face : result.dreiecke)
        for (std::size_t i = 0; i < 3; ++i) {
            const auto a = face[i], b = face[(i+1)%3];
            auto& edge = edges[std::minmax(a, b)];
            edge.a = a; edge.b = b; ++edge.count;
        }
    std::map<std::size_t, std::size_t> next;
    for (const auto& [key, edge] : edges) if (edge.count == 1) {
        if (std::abs(distance(result.punkte[edge.a])) > epsilon ||
            std::abs(distance(result.punkte[edge.b])) > epsilon ||
            !next.emplace(edge.b, edge.a).second)
            fehler(9, ort, "Schnittrand ist kein eindeutiger ebener Ring");
    }
    if (next.empty()) fehler(9, ort, "Kein Schnittrand vorhanden");
    std::vector<std::size_t> ring;
    auto current = next.begin()->first;
    const auto start = current;
    do {
        ring.push_back(current);
        const auto found = next.find(current);
        if (found == next.end()) fehler(9, ort, "Offener Schnittrand");
        current = found->second;
        next.erase(found);
    } while (current != start);
    if (!next.empty()) fehler(9, ort, "Mehrere Schnittringe oder Loecher nicht unterstuetzt");
    const auto cross2 = [&](std::size_t a, std::size_t b, std::size_t c) {
        const auto& p = result.punkte[a];
        const auto& q = result.punkte[b];
        const auto& r = result.punkte[c];
        return (q[0]-p[0])*(r[1]-p[1]) - (q[1]-p[1])*(r[0]-p[0]);
    };
    // Ear clipping retains boundary vertices, including collinear intersections.
    while (ring.size() > 3) {
        bool clipped = false;
        for (std::size_t i = 0; i < ring.size(); ++i) {
            const auto a = ring[(i+ring.size()-1)%ring.size()];
            const auto b = ring[i], c = ring[(i+1)%ring.size()];
            if (cross2(a,b,c) <= epsilon*epsilon) continue;
            bool contains = false;
            for (const auto p : ring)
                if (p != a && p != b && p != c &&
                    cross2(a,b,p) >= -epsilon*epsilon &&
                    cross2(b,c,p) >= -epsilon*epsilon &&
                    cross2(c,a,p) >= -epsilon*epsilon) { contains = true; break; }
            if (contains) continue;
            result.dreiecke.push_back({a,b,c});
            ring.erase(ring.begin() + static_cast<std::ptrdiff_t>(i));
            clipped = true;
            break;
        }
        if (!clipped) fehler(9, ort, "Schnittdeckel nicht eindeutig triangulierbar");
    }
    if (ring.size() != 3 || cross2(ring[0],ring[1],ring[2]) <= epsilon*epsilon)
        fehler(9, ort, "Ungueltiger Schnittdeckel");
    result.dreiecke.push_back({ring[0],ring[1],ring[2]});
    result = kompakt(result);
    validieren(result, true, ort + ".ergebnis");
    return result;
}

void masse_lesen(Element& e, const Json& masse, const std::filesystem::path& basis,
                 const std::string& ort, Fingerabdruck& fassung,
                 const std::map<std::string, Mesh>* quellen = nullptr,
                 bool ableitungen = false)
{
    if (masse.typ != Json::Typ::objekt) fehler(4, ort, "Massenobjekt erwartet");
    const auto art = text(pflicht(masse, "art", ort, 4), ort, 4);
    e.massenart = art;
    if (art == "punktmasse") {
        objekt(masse, {"art", "masse_kg", "position_m"}, ort, 4);
        e.masse_kg = zahl(pflicht(masse, "masse_kg", ort, 4), ort, true);
        const auto& position = pflicht(masse, "position_m", ort, 4);
        if (position.typ != Json::Typ::liste || position.liste.size() != 3)
            fehler(4, ort, "position_m muss drei Zahlen enthalten");
        for (std::size_t i = 0; i < 3; ++i) e.schwerpunkt_m[i] = zahl(position.liste[i], ort, false);
        e.massenpunkte_m.push_back(e.schwerpunkt_m);
    } else if (art == "vollkoerper" || art == "schale") {
        const bool solid = art == "vollkoerper";
        if (solid) {
            if (ableitungen) objekt(masse, {"art", "dichte_kg_pro_m3", "mesh", "ableitung"}, ort, 4);
            else if (quellen) objekt(masse, {"art", "dichte_kg_pro_m3", "mesh"}, ort, 4);
            else objekt(masse, {"art", "dichte_kg_pro_m3", "datei"}, ort, 4);
        } else {
            if (ableitungen) objekt(masse, {"art", "dichte_kg_pro_m3", "wanddicke_m", "mesh", "ableitung"}, ort, 4);
            else if (quellen) objekt(masse, {"art", "dichte_kg_pro_m3", "wanddicke_m", "mesh"}, ort, 4);
            else objekt(masse, {"art", "dichte_kg_pro_m3", "wanddicke_m", "datei"}, ort, 4);
        }
        const double materialdichte = zahl(pflicht(masse, "dichte_kg_pro_m3", ort, 4), ort, true);
        e.dichte_kg_pro_m3 = materialdichte;
        L dichte = materialdichte;
        if (!solid) {
            e.wanddicke_m = zahl(pflicht(masse, "wanddicke_m", ort, 4), ort, true);
            dichte *= *e.wanddicke_m;
        }
        Mesh mesh;
        std::string mesh_ort = ort + ".masse";
        if (quellen) {
            const auto id = text(pflicht(masse, "mesh", ort, 4), ort, 4);
            const auto fund = quellen->find(id);
            if (fund == quellen->end()) fehler(6, ort, "Mesh-ID fehlt: " + id);
            mesh = fund->second;
            mesh_ort += ", Mesh " + id;
        } else {
            const auto datei = referenz(pflicht(masse, "datei", ort, 4), basis, ort, 4);
            mesh = obj_lesen(datei, ort + ".masse", fassung);
            mesh_ort += ", Mesh " + datei.u8string();
        }
        if (ableitungen)
            if (const auto* definition = optional(masse, "ableitung")) {
                if (solid && text(pflicht(*definition, "art", ort, 9), ort, 9) == "ohne_deckflaeche")
                    fehler(9, ort, "Offene Schalenableitung nicht fuer Vollkoerpermasse");
                mesh = geometrie_ableiten(mesh, *definition, mesh_ort + ".ableitung");
            }
        validieren(mesh, solid, mesh_ort);
        e.massenpunkte_m = mesh.punkte;
        e.massen_mesh = mesh;
        masseneigenschaften(e, mesh, solid, dichte, mesh_ort);
        if (!solid) {
            std::map<std::pair<std::size_t, std::size_t>, int> kanten;
            for (const auto& t : mesh.dreiecke)
                for (std::size_t i = 0; i < 3; ++i) ++kanten[std::minmax(t[i], t[(i+1)%3])];
            std::set<std::size_t> rand;
            for (const auto& [k, anzahl] : kanten)
                if (anzahl == 1) { rand.insert(k.first); rand.insert(k.second); }
            for (const auto i : rand) e.flutrand_m.push_back(mesh.punkte[i]);
        }
    } else fehler(4, ort, "Unbekannte Massenart: " + art);
}

void summieren(Modell& modell, const std::string& ort)
{
    L gesamt = 0.L;
    V bezug{};
    for (const auto& e : modell.elemente) if (e.masse_kg > 0.) { bezug = vektor(e.schwerpunkt_m); break; }
    V erste{};
    for (const auto& e : modell.elemente) {
        if (e.masse_kg == 0.) continue;
        gesamt += e.masse_kg;
        const auto d = minus(vektor(e.schwerpunkt_m), bezug);
        for (std::size_t i = 0; i < 3; ++i) erste[i] += e.masse_kg * d[i];
    }
    modell.masse_kg = endlich(gesamt, ort);
    if (gesamt == 0.L) return;
    V mitte{};
    for (std::size_t i = 0; i < 3; ++i) {
        mitte[i] = erste[i] / gesamt;
        modell.schwerpunkt_m[i] = endlich(bezug[i] + mitte[i], ort);
    }
    Matrix traegheit{};
    for (const auto& e : modell.elemente) {
        if (e.masse_kg == 0.) continue;
        const auto d = minus(minus(vektor(e.schwerpunkt_m), bezug), mitte);
        const L quadrat = skalar(d, d);
        for (std::size_t i = 0; i < 3; ++i) for (std::size_t j = 0; j < 3; ++j)
            traegheit[3*i+j] += e.traegheit_kg_m2[3*i+j] +
                e.masse_kg * ((i == j ? quadrat : 0.L) - d[i]*d[j]);
    }
    for (std::size_t i = 0; i < 9; ++i) modell.traegheit_kg_m2[i] = endlich(traegheit[i], ort);
}

}

namespace {

Modell lade_modell_aus_json(const std::filesystem::path& datei, const Json& root,
                            Fingerabdruck fassung,
                            const std::map<std::string, Mesh>* vorbereitet = nullptr)
{
    SEGEL_MESSUNG("modellvorbereitung");
    const std::string ort = datei.u8string();
    const auto version = text(pflicht(root, "schema_version", ort), ort);
    const bool gemeinsames_obj = version == "2" || version == "3";
    const bool einzelnnetz = version == "3";
    if (gemeinsames_obj) objekt(root, {"schema_version", "id", "name", "status", "entwurf", "gruppen",
        "Hydrodynamische_Traegheit", "geometrie", "konfiguration"}, ort);
    else {
        objekt(root, {"schema_version", "id", "name", "status", "entwurf", "gruppen",
            "Hydrodynamische_Traegheit", "konfiguration"}, ort);
        if (version != "0.1-entwurf")
            fehler(2, ort, "Unterstuetzte schema_version: 0.1-entwurf, 2, 3");
    }
    std::map<std::string, Mesh> quellen;
    if (gemeinsames_obj) {
        const auto& geometrie = pflicht(root, "geometrie", ort);
        objekt(geometrie, {"datei", "laengeneinheit"}, ort + ".geometrie");
        if (text(pflicht(geometrie, "laengeneinheit", ort), ort) != "m")
            fehler(2, ort, "geometrie.laengeneinheit muss m sein");
        const auto pfad = referenz(pflicht(geometrie, "datei", ort), datei.parent_path(), ort);
        if (pfad.extension() != ".obj") fehler(2, ort, "Eine OBJ-Datei erforderlich");
        if (vorbereitet) quellen = *vorbereitet;
        else {
            std::vector<QuellMesh> teile;
            obj_lesen(pfad, ort, fassung, std::nullopt, &teile);
            for (auto& teil : teile) quellen.emplace(teil.id, std::move(teil.mesh));
        }
    }
    auto quellmesh = [&](const Json& block, const std::string& stelle) -> Mesh {
        const auto id = text(pflicht(block, "mesh", stelle), stelle);
        const auto fund = quellen.find(id);
        if (fund == quellen.end()) fehler(6, stelle, "Mesh-ID fehlt: " + id);
        return fund->second;
    };
    for (const auto* feld : {"name", "status"})
        if (const auto* wert = optional(root, feld)) text(*wert, ort + "." + feld);
    if (const auto* entwurf = optional(root, "entwurf"))
        if (entwurf->typ != Json::Typ::objekt) fehler(2, ort, "entwurf muss ein Metadatenobjekt sein");
    Modell modell;
    modell.id = text(pflicht(root, "id", ort), ort + ".id");
    if (const auto* block = optional(root, "Hydrodynamische_Traegheit")) {
        const std::string blockort = ort + ".Hydrodynamische_Traegheit";
        objekt(*block, {"zusatzmasse", "daempfung"}, blockort);
        HydrodynamischeTraegheitModell traegheit;
        auto lesen_gruppe = [&](const char* feld, std::initializer_list<const char*> erlaubt,
                                std::map<std::string, double>& ziel) {
            const auto* gruppe = optional(*block, feld);
            if (!gruppe) return;
            const std::string gruppenort = blockort + "." + feld;
            objekt(*gruppe, erlaubt, gruppenort);
            for (const auto& eintrag : gruppe->objekt) {
                if (eintrag.second.typ != Json::Typ::zahl || !std::isfinite(eintrag.second.zahl))
                    fehler(2, gruppenort + "." + eintrag.first, "Endliche Zahl erwartet");
                const double wert = eintrag.second.zahl;
                if (wert < 0.) fehler(2, gruppenort + "." + eintrag.first, "Wert darf nicht negativ sein");
                ziel[eintrag.first] = wert;
            }
        };
        lesen_gruppe("zusatzmasse", {"a11", "a22", "a33", "a44", "a55", "a66"}, traegheit.zusatzmasse);
        lesen_gruppe("daempfung", {"zeta_rollen", "zeta_stampfen", "zeta_tauchen",
            "gieren_linear", "gieren_quadratisch"}, traegheit.daempfung);
        modell.hydrodynamische_traegheit = std::move(traegheit);
    }
    const auto& gruppen = pflicht(root, "gruppen", ort);
    if (gruppen.typ != Json::Typ::objekt) fehler(2, ort, "gruppen muss ein Objekt sein");
    std::set<std::string> ids;
    std::set<std::string> verwendete_meshes;
    std::map<std::string, std::string> mesh_eigentuemer;
    for (const auto& gruppe : gruppen.objekt) {
        const std::string gruppenort = ort + ", Gruppe " + gruppe.first;
        objekt(gruppe.second, {"elemente"}, gruppenort);
        const auto& elemente = pflicht(gruppe.second, "elemente", gruppenort);
        if (elemente.typ != Json::Typ::liste) fehler(2, gruppenort, "elemente muss eine Liste sein");
        for (std::size_t index = 0; index < elemente.liste.size(); ++index) {
            const auto& element = elemente.liste[index];
            const std::string stelle = gruppenort + ", elemente[" + std::to_string(index) + "]";
            objekt(element, {"id", "Darstellung", "Hydrostatik", "Hydrodynamik", "Bewegung",
                "setting", "Segel", "Montage"}, stelle);
            Element e;
            e.id = text(pflicht(element, "id", stelle, 3), stelle + ".id", 3);
            const std::string elementort = stelle + ", Element " + e.id;
            if (!ids.insert(e.id).second) fehler(3, elementort, "Element-ID mehrfach vergeben");
            const auto* darstellung = optional(element, "Darstellung");
            const auto* hydro = optional(element, "Hydrostatik");
            const auto* hydrodynamik = optional(element, "Hydrodynamik");
            const auto* bewegung = optional(element, "Bewegung");
            std::vector<std::string> element_meshes;
            const auto register_mesh = [&](const Json& block, const std::string& location) {
                const auto mesh_id = text(pflicht(block, "mesh", location), location);
                element_meshes.push_back(mesh_id);
            };
            if (!darstellung && !hydro) fehler(2, elementort, "Darstellung und/oder Hydrostatik erforderlich");
            if (darstellung) {
                if (gemeinsames_obj) {
                    if (einzelnnetz) objekt(*darstellung, {"mesh", "farbe", "oberflaeche", "ableitung"}, elementort + ".Darstellung");
                    else objekt(*darstellung, {"mesh", "farbe", "oberflaeche"}, elementort + ".Darstellung");
                    const auto source = quellmesh(*darstellung, elementort + ".Darstellung");
                    if (einzelnnetz)
                        if (const auto* definition = optional(*darstellung, "ableitung"))
                            geometrie_ableiten(source, *definition, elementort + ".Darstellung.ableitung");
                    if (einzelnnetz) register_mesh(*darstellung, elementort + ".Darstellung");
                } else {
                    objekt(*darstellung, {"datei", "farbe", "oberflaeche"}, elementort + ".Darstellung");
                    referenz(pflicht(*darstellung, "datei", elementort), datei.parent_path(), elementort);
                }
                if (const auto* farbe = optional(*darstellung, "farbe"); farbe && !farbe_hex(*farbe))
                    fehler(2, elementort + ".Darstellung.farbe", "Farbe als \"#rrggbb\" erwartet");
                if (const auto* art = optional(*darstellung, "oberflaeche"); art && !oberflaeche_text(*art))
                    fehler(2, elementort + ".Darstellung.oberflaeche",
                        "Oberflaeche matt, glaenzend, metall, chrom, lack, glas, leuchtend, "
                        "schillernd, facettiert, drahtgitter, perlmutt, unsichtbar, stoff, holz "
                        "oder metall_gebuerstet erwartet");
            }
            if (hydro) {
                objekt(*hydro, {"masse", "volumen"}, elementort + ".Hydrostatik");
                const auto& mass_block = pflicht(*hydro, "masse", elementort, 4);
                masse_lesen(e, mass_block, datei.parent_path(), elementort, fassung,
                    gemeinsames_obj ? &quellen : nullptr, einzelnnetz);
                if (einzelnnetz && optional(mass_block, "mesh"))
                    register_mesh(mass_block, elementort + ".Hydrostatik.masse");
                if (const auto* volumen = optional(*hydro, "volumen")) {
                    std::string volumenort = elementort + ".volumen";
                    if (gemeinsames_obj) {
                        if (einzelnnetz) objekt(*volumen, {"mesh", "ableitung"}, elementort + ".volumen");
                        else objekt(*volumen, {"mesh"}, elementort + ".volumen");
                        e.volumen = quellmesh(*volumen, elementort + ".volumen");
                        if (einzelnnetz)
                            if (const auto* definition = optional(*volumen, "ableitung")) {
                                if (text(pflicht(*definition, "art", volumenort, 9), volumenort, 9) != "z_schnitt")
                                    fehler(9, volumenort, "Volumenableitung muss ein geschlossener z_schnitt sein");
                                e.volumen = geometrie_ableiten(*e.volumen, *definition, volumenort + ".ableitung");
                            }
                        if (einzelnnetz) register_mesh(*volumen, volumenort);
                    } else {
                        objekt(*volumen, {"datei"}, elementort + ".volumen");
                        const auto pfad = referenz(pflicht(*volumen, "datei", elementort), datei.parent_path(), elementort);
                        e.volumen = obj_lesen(pfad, elementort + ".volumen", fassung);
                        volumenort += ", Mesh " + pfad.u8string();
                    }
                    validieren(*e.volumen, true, volumenort);
                    // A-C-00050: nested hulls/air bodies are legal unless they intersect or contain each other.
                    for (const auto& andere : modell.elemente)
                        if (andere.volumen && volumen_durchdringen(*e.volumen, *andere.volumen))
                            fehler(7, elementort,
                                "Verdraengungsvolumina durchdringen oder enthalten sich"
                                "; Vergleich mit Element " + andere.id);
                }
                if (!e.volumen) e.flutrand_m.clear();
            }
            if (einzelnnetz && !element_meshes.empty()) {
                const auto& source_id = element_meshes.front();
                if (std::any_of(element_meshes.begin(), element_meshes.end(),
                    [&](const std::string& id) { return id != source_id; }))
                    fehler(9, elementort,
                        "Schema 3 verlangt dieselbe Mesh-ID fuer alle Geometrieverwendungen");
                const auto owner = mesh_eigentuemer.emplace(source_id, e.id);
                if (!owner.second && owner.first->second != e.id)
                    fehler(9, elementort, "Ein Quellmesh darf nicht mehreren Elementen gehoeren");
                verwendete_meshes.insert(source_id);
            }
            if (hydrodynamik) {
                objekt(*hydrodynamik, {"rolle", "modelle", "materialreferenz"},
                    elementort + ".Hydrodynamik", 3);
                const auto rolle = text(pflicht(*hydrodynamik, "rolle", elementort, 3),
                    elementort + ".Hydrodynamik.rolle", 3);
                if (rolle == "rumpf") e.hydrodynamik_rolle = HydrodynamikRolle::rumpf;
                else if (rolle == "kiel") e.hydrodynamik_rolle = HydrodynamikRolle::kiel;
                else if (rolle == "ruder") e.hydrodynamik_rolle = HydrodynamikRolle::ruder;
                else fehler(3, elementort + ".Hydrodynamik.rolle",
                    "Erwartet: rumpf, kiel oder ruder");
                if (!e.volumen)
                    fehler(3, elementort + ".Hydrodynamik",
                        "Rolle benoetigt ein physisches Hydrostatik.volumen-Mesh");
                e.hydrodynamik_daten = hydrodynamikdaten_lesen(*hydrodynamik,
                    elementort + ".Hydrodynamik");
            }
            if (bewegung) {
                if (!e.volumen)
                    fehler(2, elementort + ".Bewegung",
                        "Bewegliches Element benoetigt ein physisches Hydrostatik.volumen-Mesh");
                e.bewegung = bewegung_lesen(*bewegung, elementort + ".Bewegung");
                e.bewegung->ruderwinkel =
                    e.hydrodynamik_rolle == HydrodynamikRolle::ruder;
                if (e.bewegung->ruderwinkel && e.bewegung->art != Bewegungsart::drehen)
                    fehler(2, elementort + ".Bewegung",
                        "Ein Ruderwinkel erfordert die Bewegungsart drehen");
            }
            modell.elemente.push_back(std::move(e));
        }
    }
    if (einzelnnetz && verwendete_meshes.size() != quellen.size())
        fehler(9, ort, "Schema 3 enthaelt nicht verwendete oder keinem Element zugeordnete OBJ-Netze");
    std::map<std::string, std::string> bewegungseigentuemer;
    for (auto& element : modell.elemente) {
        if (!element.bewegung) continue;
        const auto bewegungsort = ort + ", Element " + element.id + ".Bewegung";
        for (const auto& mitbewegt : element.bewegung->mitbewegte_elemente) {
            if (mitbewegt == element.id)
                fehler(2, bewegungsort, "Steuerndes Element darf nicht selbst als mitbewegt gelistet sein");
            const auto fund = std::find_if(modell.elemente.begin(), modell.elemente.end(),
                [&](const Element& kandidat) { return kandidat.id == mitbewegt; });
            if (fund == modell.elemente.end())
                fehler(2, bewegungsort, "Unbekanntes mitbewegtes Element: " + mitbewegt);
            if (fund->bewegung)
                fehler(2, bewegungsort, "Mitbewegtes Element hat eine eigene Bewegungsdefinition: " + mitbewegt);
            if (!bewegungseigentuemer.emplace(mitbewegt, element.id).second)
                fehler(2, bewegungsort, "Mitbewegtes Element ist mehreren Bewegungen zugeordnet: " + mitbewegt);
        }
        if (element.bewegung->art != Bewegungsart::heben) continue;
        double bewegtes_min = std::numeric_limits<double>::infinity();
        for (const auto& punkt : element.volumen->punkte) {
            const double wert = punkt[0]*element.bewegung->richtung_modell[0] +
                punkt[1]*element.bewegung->richtung_modell[1] +
                punkt[2]*element.bewegung->richtung_modell[2];
            bewegtes_min = std::min(bewegtes_min, wert);
        }
        double rumpf_min = std::numeric_limits<double>::infinity();
        for (const auto& rumpf : modell.elemente) {
            if (rumpf.hydrodynamik_rolle != HydrodynamikRolle::rumpf || !rumpf.volumen) continue;
            for (const auto& punkt : rumpf.volumen->punkte) {
                const double wert = punkt[0]*element.bewegung->richtung_modell[0] +
                    punkt[1]*element.bewegung->richtung_modell[1] +
                    punkt[2]*element.bewegung->richtung_modell[2];
                rumpf_min = std::min(rumpf_min, wert);
            }
        }
        if (!std::isfinite(rumpf_min) || !std::isfinite(bewegtes_min))
            fehler(2, bewegungsort, "Hubweg benoetigt bewegtes Mesh und hydrodynamisches Rumpfmesh");
        element.bewegung->hubweg_m = std::max(0., rumpf_min - bewegtes_min);
    }
    static const std::set<std::string> einfache_rumpfmodelle{
        "bleistift", "kegel", "kugel", "platte", "pyramide", "quader", "rolle", "unregelmaessig"};
    if (!gemeinsames_obj && einfache_rumpfmodelle.count(modell.id)) {
        if (modell.elemente.size() != 1 || !modell.elemente.front().volumen)
            fehler(3, ort, "Ein einfacher Referenzkoerper benoetigt genau ein physisches Volumenmesh");
        auto& element = modell.elemente.front();
        if (element.hydrodynamik_rolle &&
            element.hydrodynamik_rolle != HydrodynamikRolle::rumpf)
            fehler(3, ort + ", Element " + element.id + ".Hydrodynamik.rolle",
                "Einfache Referenzkoerper muessen als rumpf hydrodynamisch behandelt werden");
        element.hydrodynamik_rolle = HydrodynamikRolle::rumpf;
    }
    summieren(modell, ort);
    if (modell.masse_kg <= 0.)
        fehler(4, ort, "Keine Physik: kein Hydrostatikelement mit positiver Gesamtmasse; "
            "reine Darstellungsmodelle sind nicht simulierbar");
    modell.fassung = fassung.text();
    return modell;
}

}

const nlohmann::json* element_setting(const nlohmann::json& dokument,
                                     const std::string& element_id)
{
    const nlohmann::json* setting = nullptr;
    if (!dokument.contains("gruppen") || !dokument["gruppen"].is_object()) return nullptr;
    bool found = false;
    for (const auto& gruppe : dokument["gruppen"]) {
        if (!gruppe.is_object() || !gruppe.contains("elemente") || !gruppe["elemente"].is_array())
            continue;
        for (const auto& element : gruppe["elemente"]) {
            if (!element.is_object() || element.value("id", std::string{}) != element_id) continue;
            if (found) throw std::runtime_error("CORE-MOD-0002: Mehrdeutige Element-ID fuer setting");
            found = true;
            if (element.contains("setting")) setting = &element["setting"];
        }
    }
    return setting;
}

Pose setting_pose(const nlohmann::json& setting)
{
    if (!setting.is_object() || !setting.contains("translation_m") ||
        !setting.contains("rotation_xyzw") || !setting.contains("konfiguriert") ||
        !setting["konfiguriert"].is_boolean() ||
        !setting["translation_m"].is_array() || setting["translation_m"].size() != 3 ||
        !setting["rotation_xyzw"].is_array() || setting["rotation_xyzw"].size() != 4)
        throw std::runtime_error("CORE-MOD-0002: Ungueltiges Element-setting");
    for (const auto& field : {"translation_m", "rotation_xyzw"})
        for (const auto& value : setting[field])
            if (!value.is_number() || !std::isfinite(value.get<double>()))
                throw std::runtime_error("CORE-MOD-0002: Nichtendliche Element-Montagepose");
    const auto translation = setting["translation_m"].get<Vektor3>();
    const auto rotation = setting["rotation_xyzw"].get<QuaternionXyzw>();
    double length = 0.;
    for (const auto value : rotation) length += value * value;
    if (std::abs(length - 1.) > 1e-10)
        throw std::runtime_error("CORE-MOD-0002: Montagequaternion ist nicht normiert");
    return Pose(translation, rotation);
}

void validiere_element_settings(const nlohmann::json& dokument, const std::string& kontext)
{
    if (!dokument.contains("gruppen") || !dokument["gruppen"].is_object()) return;
    for (const auto& gruppe : dokument["gruppen"]) {
        if (!gruppe.is_object() || !gruppe.contains("elemente") || !gruppe["elemente"].is_array())
            continue;
        for (const auto& element : gruppe["elemente"]) {
            if (!element.is_object()) continue;
            const auto id = element.find("id");
            const auto ort = kontext + ", Element " +
                (id != element.end() && id->is_string() ? id->get<std::string>() : "?");
            if (element.contains("setting")) {
                try {
                    (void)setting_pose(element["setting"]);
                } catch (const std::runtime_error& error) {
                    fehler(2, ort + ".setting", error.what());
                }
                const auto& setting = element["setting"];
                for (const auto* legacy : {"intrinsische_quelle_id", "darstellungs_mesh_id",
                                          "lookup_id", "fock_versatz_cm", "segel"})
                    if (setting.contains(legacy))
                        fehler(2, ort + ".setting", "Redundantes Legacy-setting; Migration erforderlich");
                if (setting.contains("original_buglage")) {
                    if (!setting["original_buglage"].is_string())
                        fehler(2, ort + ".setting.original_buglage", "Original-Buglage muss Text sein");
                    const auto side = setting["original_buglage"].get<std::string>();
                    if (side != "unbekannt" && side != "backbord" && side != "steuerbord")
                        fehler(2, ort + ".setting.original_buglage", "Ungueltige Original-Buglage");
                }
                if (setting.contains("quellen_fingerprint") && !setting["quellen_fingerprint"].is_string())
                    fehler(2, ort + ".setting.quellen_fingerprint", "Ungueltiger setting-Quellfingerprint");
                if (!element.contains("Segel") &&
                    (setting.contains("original_buglage") || setting.contains("quellen_fingerprint")))
                    fehler(2, ort + ".setting", "Segel-setting ohne Segelquelle");
            }
            if (element.contains("Segel")) {
                const auto& source = element["Segel"];
                if (!source.is_object() || source.size() > 2 ||
                    !source.contains("quelle_datei") || !source["quelle_datei"].is_string() ||
                    source["quelle_datei"].get<std::string>().empty() ||
                    (source.contains("lookup_datei") &&
                     (!source["lookup_datei"].is_string() ||
                      source["lookup_datei"].get<std::string>().empty())))
                    fehler(2, ort + ".Segel", "Segel braucht Quelle und optional vorhandenen Lookup");
            }
        }
    }
}

Modell lade_modell(const std::filesystem::path& datei)
{
    const std::string ort = datei.u8string();
    const auto inhalt = lesen(datei, ort);
    Fingerabdruck fassung;
    fassung.datei(inhalt);
    const auto root = Parser(inhalt, ort).lesen();
    validiere_element_settings(nlohmann::json::parse(inhalt), ort);
    auto modell = lade_modell_aus_json(datei, root, std::move(fassung));
    modell.fassung = modell_fassung(datei);
    return modell;
}

Mesh lade_quellmesh(const std::filesystem::path& datei,
                   const std::optional<std::string>& objekt, bool darstellung)
{
    Fingerabdruck fassung;
    return obj_lesen(datei, "Quellgeometrie", fassung, objekt, nullptr, darstellung);
}

const nlohmann::json* geometrieableitung(const ModellBeschreibung& beschreibung,
    const std::string& element_id, const std::string& verwendung)
{
    const auto& document = beschreibung.dokument;
    if (document.value("schema_version", "") != "3") return nullptr;
    for (const auto& group : document.at("gruppen"))
        for (const auto& element : group.at("elemente")) {
            if (element.at("id") != element_id) continue;
            const auto* block = &element;
            if (verwendung == "Darstellung") {
                if (!block->contains("Darstellung")) return nullptr;
                block = &block->at("Darstellung");
            } else {
                if (!block->contains("Hydrostatik") ||
                    !block->at("Hydrostatik").contains(verwendung)) return nullptr;
                block = &block->at("Hydrostatik").at(verwendung);
            }
            return block->contains("ableitung") ? &block->at("ableitung") : nullptr;
        }
    return nullptr;
}

Mesh abgeleitetes_mesh(const Mesh& quelle, const nlohmann::json& definition)
{
    const auto encoded = definition.dump();
    return geometrie_ableiten(quelle, Parser(encoded, "Geometrieableitung").lesen(),
        "Geometrieableitung");
}

namespace {
const Json* text_feld(const Json& wert, const char* feld)
{
    if (wert.typ != Json::Typ::objekt) return nullptr;
    const auto* kind = optional(wert, feld);
    return kind && kind->typ == Json::Typ::text && !kind->text.empty() ? kind : nullptr;
}

std::optional<std::filesystem::path> relativ(const Json& objekt_wert, const char* feld,
                                             const std::filesystem::path& basis)
{
    const auto* wert = text_feld(objekt_wert, feld);
    if (!wert || wert->text.find('\0') != std::string::npos) return std::nullopt;
    const auto pfad = std::filesystem::u8path(wert->text);
    if (pfad.is_absolute() || pfad.has_root_name() || pfad.has_root_directory()) return std::nullopt;
    return basis / pfad;
}
}

namespace {

ModellBeschreibung beschreibung_aus_json(const std::filesystem::path& datei, const Json& root)
{
    const std::string ort = datei.u8string();
    if (root.typ != Json::Typ::objekt) fehler(2, ort, "Objekt erwartet");
    ModellBeschreibung result;
    result.dokument = json_wert(root);
    result.id = text(pflicht(root, "id", ort), ort + ".id");
    if (const auto* name = text_feld(root, "name")) result.name = name->text;
    else result.name = result.id;
    if (const auto* beschreibung = text_feld(root, "beschreibung")) result.beschreibung = beschreibung->text;
    const auto* gruppen = optional(root, "gruppen");
    if (!gruppen || gruppen->typ != Json::Typ::objekt) return result;
    const auto basis = datei.parent_path();
    const auto* version = text_feld(root, "schema_version");
    const bool shared_geometry_schema = version &&
        (version->text == "2" || version->text == "3");
    std::optional<std::filesystem::path> gemeinsame_datei;
    if (shared_geometry_schema) {
        if (const auto* geometrie = optional(root, "geometrie"))
            gemeinsame_datei = relativ(*geometrie, "datei", basis);
        if (gemeinsame_datei) {
            Fingerabdruck fassung;
            try { obj_lesen(*gemeinsame_datei, ort, fassung, std::nullopt, &result.quellmeshes); }
            catch (const std::exception&) { result.quellmeshes.clear(); }
        }
    }
    std::map<std::filesystem::path, std::string> alte_quellen;
    auto quelle_lesen = [&](const Json& block, bool darstellung) -> std::optional<std::string> {
        if (shared_geometry_schema) {
            const auto* mesh = text_feld(block, "mesh");
            return mesh ? std::optional<std::string>(mesh->text) : std::nullopt;
        }
        const auto pfad = relativ(block, "datei", basis);
        if (!pfad) return std::nullopt;
        const auto key = pfad->lexically_normal();
        const auto fund = alte_quellen.find(key);
        if (fund != alte_quellen.end()) return fund->second;
        const auto id = "quelle-" + std::to_string(alte_quellen.size() + 1);
        alte_quellen.emplace(key, id);
        try {
            result.quellmeshes.push_back({id, "OBJ:" + pfad->filename().u8string(),
                lade_quellmesh(*pfad, std::nullopt, darstellung)});
        } catch (const std::exception&) {}
        return id;
    };
    for (const auto& gruppe : gruppen->objekt) {
        result.gruppen.push_back(gruppe.first);
        const auto* elemente = optional(gruppe.second, "elemente");
        if (gruppe.second.typ != Json::Typ::objekt || !elemente || elemente->typ != Json::Typ::liste) continue;
        for (const auto& element : elemente->liste) {
            const auto* id = text_feld(element, "id");
            if (!id) continue;
            ElementBeschreibung e;
            e.id = id->text;
            e.gruppe = gruppe.first;
            if (const auto* darstellung = optional(element, "Darstellung")) {
                e.darstellung = shared_geometry_schema ? gemeinsame_datei : relativ(*darstellung, "datei", basis);
                e.darstellung_mesh = quelle_lesen(*darstellung, true);
                if (darstellung->typ == Json::Typ::objekt) {
                    if (const auto* farbe = optional(*darstellung, "farbe")) e.farbe_rgb = farbe_hex(*farbe);
                    if (const auto* art = optional(*darstellung, "oberflaeche"))
                        e.oberflaeche = oberflaeche_text(*art);
                }
                if (const auto* bewegung = optional(element, "Bewegung"))
                    e.bewegung = bewegung_lesen(*bewegung, ort + ", Element " + e.id + ".Bewegung");
                if (e.bewegung) {
                    const auto* hydrodynamik = optional(element, "Hydrodynamik");
                    const auto* rolle = hydrodynamik ? text_feld(*hydrodynamik, "rolle") : nullptr;
                    e.bewegung->ruderwinkel = rolle && rolle->text == "ruder";
                }
            }
            if (const auto* hydro = optional(element, "Hydrostatik")) {
                if (hydro->typ == Json::Typ::objekt) {
                    if (const auto* volumen = optional(*hydro, "volumen")) {
                        e.volumen = shared_geometry_schema ? gemeinsame_datei : relativ(*volumen, "datei", basis);
                        e.volumen_mesh = quelle_lesen(*volumen, false);
                    }
                    if (const auto* masse = optional(*hydro, "masse"))
                        e.masse_mesh = quelle_lesen(*masse, false);
                }
            }
            result.elemente.push_back(std::move(e));
        }
    }
    return result;
}

}

ModellBeschreibung lies_beschreibung(const std::filesystem::path& datei)
{
    const std::string ort = datei.u8string();
    const auto inhalt = lesen(datei, ort);
    const auto root = Parser(inhalt, ort).lesen();
    validiere_element_settings(nlohmann::json::parse(inhalt), ort);
    return beschreibung_aus_json(datei, root);
}

ModellInspektionsImport lade_inspektionsimport(const std::filesystem::path& datei)
{
    const std::string ort = datei.u8string();
    const auto inhalt = lesen(datei, ort);
    auto root = Parser(inhalt, ort).lesen();
    validiere_element_settings(nlohmann::json::parse(inhalt), ort);
    ModellInspektionsImport result;
    result.beschreibung = beschreibung_aus_json(datei, root);
    Fingerabdruck fassung;
    fassung.datei(inhalt);
    try {
        result.modell = lade_modell_aus_json(datei, root, std::move(fassung));
        result.modell->fassung = modell_fassung(datei);
    } catch (const std::exception& error) {
        result.import_fehler = error.what();
    }
    return result;
}

std::string modell_fassung(const std::filesystem::path& datei)
{
    const std::string ort = datei.u8string();
    const auto inhalt = lesen(datei, ort);
    auto root = nlohmann::json::parse(inhalt);
    if (!root.is_object()) fehler(2, ort, "Modell muss ein JSON-Objekt sein");
    root.erase("konfiguration");
    if (root.contains("gruppen") && root["gruppen"].is_object())
        for (auto& gruppe : root["gruppen"])
            if (gruppe.is_object() && gruppe.contains("elemente") && gruppe["elemente"].is_array())
                for (auto& element : gruppe["elemente"])
                    if (element.is_object() && element.contains("id") && element["id"].is_string())
                        element.erase("setting");
    Fingerabdruck fassung;
    fassung.datei(root.dump());
    std::set<std::string> quellen;
    const auto sammeln = [&](const auto& self, const nlohmann::json& wert) -> void {
        if (wert.is_object()) {
            for (auto it = wert.begin(); it != wert.end(); ++it) {
                if ((it.key() == "datei" || it.key() == "quelle_datei" ||
                     it.key() == "lookup_datei") && it.value().is_string())
                    quellen.insert(it.value().get<std::string>());
                else self(self, it.value());
            }
        } else if (wert.is_array()) {
            for (const auto& kind : wert) self(self, kind);
        }
    };
    sammeln(sammeln, root);
    for (const auto& quelle : quellen) {
        fassung.datei(quelle);
        const auto pfad = datei.parent_path() / std::filesystem::u8path(quelle);
        if (std::filesystem::exists(pfad))
            fassung.datei(lesen(pfad, ort));
        else
            fassung.datei("[fehlende Quelldatei]");
    }
    return fassung.text();
}

void konvertiere_modell_v2(const std::filesystem::path& quelle,
                          const std::filesystem::path& zielordner)
{
    const auto modell = lade_modell(quelle);
    auto beschreibung = lies_beschreibung(quelle);
    if (std::filesystem::exists(zielordner))
        throw std::runtime_error("CORE-MOD-0005: Zielordner existiert bereits");
    const auto inhalt = lesen(quelle, quelle.u8string());
    auto dokument = json_wert(Parser(inhalt, quelle.u8string()).lesen(), false);
    std::ostringstream obj;
    obj.imbue(std::locale::classic());
    obj << std::setprecision(std::numeric_limits<double>::max_digits10);
    std::size_t offset = 1;
    auto mesh_schreiben = [&](const std::string& id, const Mesh& mesh) {
        obj << "o " << id << '\n';
        for (const auto& p : mesh.punkte) obj << "v " << p[0] << ' ' << p[1] << ' ' << p[2] << '\n';
        for (const auto& t : mesh.dreiecke)
            obj << "f " << t[0]+offset << ' ' << t[1]+offset << ' ' << t[2]+offset << '\n';
        offset += mesh.punkte.size();
    };
    std::set<std::string> mesh_ids;
    for (const auto& q : beschreibung.quellmeshes) {
        mesh_schreiben(q.id, q.mesh);
        mesh_ids.insert(q.id);
    }
    std::size_t ableitungsnummer = 0;
    for (auto& gruppe : dokument["gruppen"]) for (auto& element : gruppe["elemente"]) {
        const auto id = element.at("id").get<std::string>();
        const auto e = std::find_if(beschreibung.elemente.begin(), beschreibung.elemente.end(),
            [&](const ElementBeschreibung& kandidat) { return kandidat.id == id; });
        auto referenz_ersetzen = [&](nlohmann::json& block, const std::optional<std::string>& mesh_id) {
            const auto source = std::find_if(beschreibung.quellmeshes.begin(), beschreibung.quellmeshes.end(),
                [&](const QuellMesh& q) { return mesh_id && q.id == *mesh_id; });
            if (source == beschreibung.quellmeshes.end())
                throw std::runtime_error("CORE-MOD-0006: Quellmesh nicht lesbar: " + id);
            block.erase("datei");
            block["mesh"] = *mesh_id;
            if (block.contains("ableitung")) {
                const auto encoded = block.at("ableitung").dump();
                const auto location = quelle.u8string() + ", Element " + id + ".ableitung";
                const auto mesh = geometrie_ableiten(source->mesh, Parser(encoded, location).lesen(), location);
                std::string derived_id;
                do {
                    derived_id = "abgeleitet_" + std::to_string(++ableitungsnummer);
                } while (!mesh_ids.insert(derived_id).second);
                mesh_schreiben(derived_id, mesh);
                block["mesh"] = derived_id;
                block.erase("ableitung");
            }
        };
        if (element.contains("Darstellung")) referenz_ersetzen(element["Darstellung"], e->darstellung_mesh);
        if (element.contains("Hydrostatik")) {
            auto& hydro = element["Hydrostatik"];
            if (hydro.contains("volumen")) referenz_ersetzen(hydro["volumen"], e->volumen_mesh);
            if (hydro.contains("masse") && hydro["masse"].value("art", "") != "punktmasse")
                referenz_ersetzen(hydro["masse"], e->masse_mesh);
        }
        const auto physik = std::find_if(modell.elemente.begin(), modell.elemente.end(),
            [&](const Element& kandidat) { return kandidat.id == id; });
        if (!element.contains("Hydrodynamik") && physik->hydrodynamik_rolle) {
            const auto rolle = *physik->hydrodynamik_rolle;
            element["Hydrodynamik"] = {{"rolle", rolle == HydrodynamikRolle::rumpf ? "rumpf" :
                rolle == HydrodynamikRolle::kiel ? "kiel" : "ruder"}};
        }
    }
    dokument["schema_version"] = "2";
    dokument["geometrie"] = {{"datei", "boot.obj"}, {"laengeneinheit", "m"}};
    const auto json = dokument.dump(2) + "\n";
    if (!std::filesystem::create_directory(zielordner))
        throw std::runtime_error("CORE-MOD-0005: Neuer Zielordner kann nicht erstellt werden");
    auto schreiben = [&](const char* name, const std::string& inhalt) {
        std::ofstream out(zielordner / name, std::ios::binary);
        out << inhalt;
        out.close();
        if (!out) throw std::runtime_error("CORE-MOD-0005: Konvertiertes Modell nicht schreibbar");
    };
    schreiben("boot.obj", obj.str());
    schreiben("bootsmodell.json", json);
    lade_modell(zielordner / "bootsmodell.json");
}

namespace {

bool gleiche_geometrie(const Mesh& left, const Mesh& right)
{
    auto points = [](const Mesh& mesh) {
        auto result = mesh.punkte;
        std::sort(result.begin(), result.end());
        return result;
    };
    using Triangle = std::array<Vektor3, 3>;
    auto triangles = [](const Mesh& mesh) {
        std::vector<Triangle> result;
        result.reserve(mesh.dreiecke.size());
        for (const auto& face : mesh.dreiecke) {
            const Triangle triangle{mesh.punkte.at(face[0]), mesh.punkte.at(face[1]),
                mesh.punkte.at(face[2])};
            result.push_back(std::min({triangle,
                Triangle{triangle[1], triangle[2], triangle[0]},
                Triangle{triangle[2], triangle[0], triangle[1]}}));
        }
        std::sort(result.begin(), result.end());
        return result;
    };
    return points(left) == points(right) && triangles(left) == triangles(right);
}

bool offene_kanten(const Mesh& mesh)
{
    std::map<std::pair<std::size_t, std::size_t>, unsigned> edges;
    for (const auto& face : mesh.dreiecke)
        for (std::size_t i = 0; i < 3; ++i) ++edges[std::minmax(face[i], face[(i+1)%3])];
    return std::any_of(edges.begin(), edges.end(), [](const auto& edge) {
        return edge.second == 1;
    });
}

bool ausreichend_gleich(const Mesh& candidate, const Mesh& reference, bool solid)
{
    const auto a = box(candidate), b = box(reference);
    for (std::size_t i = 0; i < 3; ++i)
        if (std::abs(a.unten[i]-b.unten[i]) > 1e-6 ||
            std::abs(a.oben[i]-b.oben[i]) > 1e-6) return false;
    Element x, y;
    masseneigenschaften(x, candidate, solid, 1., "Migrationsvergleich Kandidat");
    masseneigenschaften(y, reference, solid, 1., "Migrationsvergleich Bestand");
    const double relative = .001;
    if (std::abs(x.masse_kg-y.masse_kg) > relative*y.masse_kg) return false;
    for (std::size_t i = 0; i < 3; ++i)
        if (std::abs(x.schwerpunkt_m[i]-y.schwerpunkt_m[i]) > (solid ? .0001 : .001)) return false;
    const auto scale = *std::max_element(y.traegheit_kg_m2.begin(), y.traegheit_kg_m2.end(),
        [](double p, double q) { return std::abs(p) < std::abs(q); });
    for (std::size_t i = 0; i < 9; ++i)
        if (std::abs(x.traegheit_kg_m2[i]-y.traegheit_kg_m2[i]) >
            (solid ? .001 : .01)*std::max(std::abs(scale), 1e-12)) return false;
    return true;
}

std::optional<nlohmann::json> schnittebene(const Mesh& source, const Mesh& reference)
{
    for (const auto& face : reference.dreiecke) {
        const auto p = vektor(reference.punkte[face[0]]);
        const auto normal = kreuz(minus(vektor(reference.punkte[face[1]]), p),
            minus(vektor(reference.punkte[face[2]]), p));
        if (normal[2] <= 1e-12L) continue;
        const double sx = static_cast<double>(-normal[0]/normal[2]);
        const double sy = static_cast<double>(-normal[1]/normal[2]);
        const double z = static_cast<double>(p[2]-sx*p[0]-sy*p[1]);
        const auto distance = [&](const Vektor3& point) {
            return point[2]-z-sx*point[0]-sy*point[1];
        };
        if (std::any_of(reference.punkte.begin(), reference.punkte.end(),
                [&](const Vektor3& point) { return distance(point) > 1e-8; }) ||
            std::none_of(source.punkte.begin(), source.punkte.end(),
                [&](const Vektor3& point) { return distance(point) > 1e-8; })) continue;
        return nlohmann::json{{"art", "z_schnitt"}, {"z_m", z},
            {"steigung_x", sx}, {"steigung_y", sy}};
    }
    return std::nullopt;
}

}

void konvertiere_modell_v3(const std::filesystem::path& quelle,
                          const std::filesystem::path& zielordner,
                          bool ableitungen_zulassen)
{
    if (std::filesystem::exists(zielordner))
        throw std::runtime_error("CORE-MOD-0005: Zielordner existiert bereits");
    const auto modell = lade_modell(quelle);
    const auto beschreibung = lies_beschreibung(quelle);
    std::map<std::string, const QuellMesh*> meshes_by_id;
    for (const auto& mesh : beschreibung.quellmeshes)
        meshes_by_id.emplace(mesh.id, &mesh);

    std::map<std::string, std::string> source_id_by_element;
    std::map<std::pair<std::string, std::string>, nlohmann::json> derivations;
    std::vector<std::pair<std::string, Mesh>> canonical_meshes;
    for (const auto& element : beschreibung.elemente) {
        std::vector<std::optional<std::string>> source_ids{
            element.darstellung_mesh, element.masse_mesh, element.volumen_mesh};
        std::vector<const QuellMesh*> uses;
        for (const auto& source_id : source_ids) {
            if (!source_id) continue;
            const auto found = meshes_by_id.find(*source_id);
            if (found == meshes_by_id.end())
                fehler(6, quelle.u8string() + ", Element " + element.id,
                    "Quellmesh nicht lesbar: " + *source_id);
            uses.push_back(found->second);
        }
        if (uses.empty()) continue;
        const QuellMesh* canonical = uses.front();
        if (ableitungen_zulassen && element.masse_mesh) {
            canonical = meshes_by_id.at(*element.masse_mesh);
            if (offene_kanten(canonical->mesh) && element.volumen_mesh)
                canonical = meshes_by_id.at(*element.volumen_mesh);
        }
        const std::array<std::pair<const char*, std::optional<std::string>>, 3> roles{{
            {"Darstellung", element.darstellung_mesh}, {"masse", element.masse_mesh},
            {"volumen", element.volumen_mesh}}};
        for (const auto& [role, id] : roles) {
            if (!id) continue;
            const auto& reference = meshes_by_id.at(*id)->mesh;
            if (gleiche_geometrie(canonical->mesh, reference)) continue;
            const auto location = quelle.u8string() + ", Element " + element.id + "." + role;
            if (!ableitungen_zulassen)
                fehler(9, location, "Abweichende Geometriequellen verhindern die Schema-3-Migration: " +
                    canonical->id + " und " + *id);
            if (std::string(role) == "Darstellung" && !offene_kanten(reference)) {
                if (!ausreichend_gleich(canonical->mesh, reference, false))
                    fehler(9, location, "Darstellung weicht ueber die Migrationsgrenzen hinaus ab");
                continue;
            }
            const bool shell = offene_kanten(reference);
            auto definition = shell ?
                std::optional<nlohmann::json>({{"art", "ohne_deckflaeche"},
                    {"z_m", static_cast<double>(box(reference).oben[2])}}) :
                schnittebene(canonical->mesh, reference);
            if (!definition) fehler(9, location, "Keine reproduzierbare Schnittebene gefunden");
            const auto encoded = definition->dump();
            const auto derived = geometrie_ableiten(canonical->mesh,
                Parser(encoded, location).lesen(), location);
            if (shell ? !gleiche_geometrie(derived, reference) :
                !ausreichend_gleich(derived, reference, true))
                fehler(9, location, "Ableitung erhaelt Geometrie oder Physik nicht ausreichend");
            derivations.emplace(std::make_pair(element.id, role), std::move(*definition));
        }

        const auto mesh_id = "element_mesh_" + std::to_string(canonical_meshes.size() + 1);
        source_id_by_element.emplace(element.id, mesh_id);
        canonical_meshes.emplace_back(mesh_id, canonical->mesh);
    }

    const auto source_text = lesen(quelle, quelle.u8string());
    auto document = json_wert(Parser(source_text, quelle.u8string()).lesen(), false);
    std::ostringstream obj;
    obj.imbue(std::locale::classic());
    obj << std::setprecision(std::numeric_limits<double>::max_digits10);
    std::size_t offset = 1;
    for (const auto& [mesh_id, mesh] : canonical_meshes) {
        obj << "o " << mesh_id << '\n';
        for (const auto& point : mesh.punkte)
            obj << "v " << point[0] << ' ' << point[1] << ' ' << point[2] << '\n';
        for (const auto& face : mesh.dreiecke)
            obj << "f " << face[0] + offset << ' ' << face[1] + offset << ' '
                << face[2] + offset << '\n';
        offset += mesh.punkte.size();
    }

    for (auto& group : document["gruppen"]) for (auto& json_element : group["elemente"]) {
        const auto id = json_element.at("id").get<std::string>();
        const auto described = std::find_if(beschreibung.elemente.begin(), beschreibung.elemente.end(),
            [&](const ElementBeschreibung& candidate) { return candidate.id == id; });
        if (described == beschreibung.elemente.end())
            fehler(6, quelle.u8string() + ", Element " + id, "Elementbeschreibung fehlt");
        const auto canonical = source_id_by_element.find(id);
        auto replace_reference = [&](nlohmann::json& block,
                                     const std::optional<std::string>& source_id,
                                     const char* role) {
            if (!source_id || canonical == source_id_by_element.end())
                fehler(6, quelle.u8string() + ", Element " + id,
                    "Geometrieverwendung ohne lesbare gemeinsame Quelle");
            block.erase("datei");
            block["mesh"] = canonical->second;
            const auto derived = derivations.find({id, role});
            if (derived != derivations.end()) block["ableitung"] = derived->second;
        };
        if (json_element.contains("Darstellung"))
            replace_reference(json_element["Darstellung"], described->darstellung_mesh, "Darstellung");
        if (json_element.contains("Hydrostatik")) {
            auto& hydro = json_element["Hydrostatik"];
            if (hydro.contains("volumen"))
                replace_reference(hydro["volumen"], described->volumen_mesh, "volumen");
            if (hydro.contains("masse") &&
                hydro["masse"].value("art", "") != "punktmasse")
                replace_reference(hydro["masse"], described->masse_mesh, "masse");
        }
        const auto physical = std::find_if(modell.elemente.begin(), modell.elemente.end(),
            [&](const Element& candidate) { return candidate.id == id; });
        if (!json_element.contains("Hydrodynamik") && physical->hydrodynamik_rolle) {
            const auto role = *physical->hydrodynamik_rolle;
            json_element["Hydrodynamik"] = {{"rolle",
                role == HydrodynamikRolle::rumpf ? "rumpf" :
                role == HydrodynamikRolle::kiel ? "kiel" : "ruder"}};
        }
    }
    document["schema_version"] = "3";
    document["geometrie"] = {{"datei", "boot.obj"}, {"laengeneinheit", "m"}};
    const auto json = document.dump(2) + "\n";
    std::map<std::string, Mesh> prepared;
    for (const auto& [id, mesh] : canonical_meshes) prepared.emplace(id, mesh);
    lade_modell_aus_json(zielordner / "bootsmodell.json",
        Parser(json, zielordner.u8string()).lesen(), Fingerabdruck{}, &prepared);
    if (!std::filesystem::create_directory(zielordner))
        throw std::runtime_error("CORE-MOD-0005: Neuer Zielordner kann nicht erstellt werden");
    auto write_file = [&](const char* name, const std::string& contents) {
        std::ofstream output(zielordner / name, std::ios::binary);
        output << contents;
        output.close();
        if (!output) throw std::runtime_error("CORE-MOD-0005: Konvertiertes Modell nicht schreibbar");
    };
    write_file("boot.obj", obj.str());
    write_file("bootsmodell.json", json);
    lade_modell(zielordner / "bootsmodell.json");
}

}
