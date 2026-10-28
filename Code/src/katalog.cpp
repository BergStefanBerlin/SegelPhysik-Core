#include "segelphysik/katalog.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <limits>
#include <locale>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <system_error>

namespace segelphysik {
namespace {

const char* const modelldateiname = "modell.json";

[[noreturn]] void kat_fehler(int nummer, const std::string& text)
{
    throw std::runtime_error("CORE-KAT-000" + std::to_string(nummer) + ": " + text);
}

std::string fehler_id(const std::string& diagnose, const char* ersatz)
{
    if (diagnose.rfind("CORE-", 0) != 0) return ersatz;
    return diagnose.substr(0, diagnose.find_first_of(" [:"));
}

void ersetzen(std::string& text, const std::string& alt, const std::string& neu)
{
    if (alt.empty()) return;
    for (auto pos = text.find(alt); pos != std::string::npos; pos = text.find(alt, pos + neu.size()))
        text.replace(pos, alt.size(), neu);
}

// Answers must not contain paths: strip the model folder prefix in every spelling.
std::string ohne_pfad(std::string text, const std::filesystem::path& ordner)
{
    for (const auto& form : {ordner.u8string(), ordner.generic_u8string()}) {
        ersetzen(text, form + "\\", "");
        ersetzen(text, form + "/", "");
        ersetzen(text, form, ".");
    }
    return text;
}

KatalogDiagnose diagnose_aus(const std::string& meldung, const std::filesystem::path& ordner,
                             const char* ersatz)
{
    const auto id = fehler_id(meldung, ersatz);
    std::string rest = meldung.rfind("CORE-", 0) == 0 ? meldung.substr(id.size()) : meldung;
    std::string fundstelle;
    if (!rest.empty() && rest.front() == ' ' && rest.size() > 1 && rest[1] == '[') {
        const auto ende = rest.find("]: ");
        if (ende != std::string::npos) {
            fundstelle = rest.substr(2, ende - 2);
            rest = rest.substr(ende + 3);
        }
    } else if (rest.rfind(": ", 0) == 0) rest = rest.substr(2);
    return {id, ohne_pfad(rest, ordner), ohne_pfad(fundstelle, ordner)};
}

std::string json_text(const std::string& s)
{
    std::ostringstream out;
    out << '"';
    for (unsigned char c : s) {
        switch (c) {
        case '"': out << "\\\""; break;
        case '\\': out << "\\\\"; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default:
            if (c < 32) out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                            << static_cast<int>(c) << std::dec;
            else out << static_cast<char>(c);
        }
    }
    out << '"';
    return out.str();
}

std::ostringstream json_strom()
{
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(17);
    return out;
}

void zahlen(std::ostream& out, const Vektor3& v)
{
    out << '[' << v[0] << ',' << v[1] << ',' << v[2] << ']';
}

double obj_zahl(const std::string& token, const std::string& ort)
{
    double wert = 0.;
    const char* start = token.data();
    if (!token.empty() && token.front() == '+') ++start;
    const auto ergebnis = std::from_chars(start, token.data() + token.size(), wert);
    if (ergebnis.ec != std::errc{} || ergebnis.ptr != token.data() + token.size() || !std::isfinite(wert))
        kat_fehler(5, "Ungueltige OBJ-Zahl in " + ort);
    return wert;
}

// Display meshes are read leniently like the GUI did in G-0.1: polygons are
// fan-triangulated, unsupported records are ignored.
void obj_darstellung(const std::filesystem::path& datei, const std::string& ort, Darstellungsteil& teil)
{
    std::ifstream strom(datei, std::ios::binary);
    if (!strom) kat_fehler(5, "Darstellungsnetz nicht lesbar: " + ort);
    std::string zeile;
    std::size_t nummer = 0;
    while (std::getline(strom, zeile)) {
        ++nummer;
        const std::string stelle = ort + ":" + std::to_string(nummer);
        std::istringstream tokens(zeile);
        tokens.imbue(std::locale::classic());
        std::string art, token;
        if (!(tokens >> art)) continue;
        if (art == "v") {
            Vektor3 p{};
            for (std::size_t i = 0; i < 3; ++i) {
                if (!(tokens >> token)) kat_fehler(5, "OBJ-Punkt unvollstaendig in " + stelle);
                p[i] = obj_zahl(token, stelle);
            }
            teil.punkte_m.push_back(p);
        } else if (art == "f") {
            std::vector<std::size_t> indizes;
            while (tokens >> token) {
                const auto feld = token.substr(0, token.find('/'));
                long long index = 0;
                const auto ergebnis = std::from_chars(feld.data(), feld.data() + feld.size(), index);
                const auto anzahl = static_cast<long long>(teil.punkte_m.size());
                if (ergebnis.ec != std::errc{} || ergebnis.ptr != feld.data() + feld.size() || index == 0)
                    kat_fehler(5, "Ungueltiger OBJ-Index in " + stelle);
                const long long null_basiert = index > 0 ? index - 1 : anzahl + index;
                if (null_basiert < 0 || null_basiert >= anzahl)
                    kat_fehler(5, "OBJ-Index ausserhalb des Netzes in " + stelle);
                indizes.push_back(static_cast<std::size_t>(null_basiert));
            }
            if (indizes.size() < 3) kat_fehler(5, "Entartete OBJ-Flaeche in " + stelle);
            for (std::size_t i = 1; i + 1 < indizes.size(); ++i)
                teil.dreiecke.push_back({indizes[0], indizes[i], indizes[i + 1]});
        }
    }
    if (teil.punkte_m.empty() || teil.dreiecke.empty())
        kat_fehler(5, "Darstellungsnetz ohne Dreiecke: " + ort);
}

std::string relativer_name(const std::filesystem::path& datei, const std::filesystem::path& ordner)
{
    std::error_code fehler;
    const auto rel = std::filesystem::relative(datei, ordner, fehler);
    return fehler || rel.empty() ? datei.filename().u8string() : rel.generic_u8string();
}

} // namespace

const KatalogEintrag* Modellkatalog::eintrag(const std::string& modell_id) const
{
    const auto fund = std::find_if(eintraege_.begin(), eintraege_.end(),
        [&](const KatalogEintrag& e) { return e.modell_id == modell_id; });
    return fund == eintraege_.end() ? nullptr : &*fund;
}

Modellkatalog modellkatalog(const std::filesystem::path& ordner)
{
    std::error_code fehler;
    if (ordner.empty() || !std::filesystem::is_directory(ordner, fehler))
        kat_fehler(1, "Modellordner nicht lesbar");
    std::vector<std::pair<std::string, std::filesystem::path>> kandidaten;
    if (std::filesystem::is_regular_file(ordner / modelldateiname, fehler))
        kandidaten.emplace_back(std::string(), ordner / modelldateiname);
    std::filesystem::directory_iterator it(ordner, fehler), ende;
    if (fehler) kat_fehler(1, "Modellordner nicht lesbar");
    for (; it != ende; it.increment(fehler)) {
        if (fehler) kat_fehler(1, "Modellordner nicht vollstaendig lesbar");
        std::error_code typfehler;
        if (!it->is_directory(typfehler)) continue;
        const auto datei = it->path() / modelldateiname;
        if (std::filesystem::is_regular_file(datei, typfehler))
            kandidaten.emplace_back(it->path().filename().u8string(), datei);
    }
    // Stable order by folder name, case-insensitive like the GUI G-0.1 listing.
    auto klein = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c) { return static_cast<char>(c < 128 ? std::tolower(c) : c); });
        return s;
    };
    std::stable_sort(kandidaten.begin(), kandidaten.end(), [&](const auto& a, const auto& b) {
        const auto ka = klein(a.first), kb = klein(b.first);
        return ka != kb ? ka < kb : a.first < b.first;
    });

    Modellkatalog katalog;
    for (const auto& [ordnername, datei] : kandidaten) {
        KatalogEintrag eintrag;
        std::vector<KatalogDiagnose> diagnosen;
        const auto modellordner = datei.parent_path();
        try {
            const auto beschreibung = lies_beschreibung(datei);
            eintrag.modell_id = beschreibung.id;
            eintrag.name = beschreibung.name;
            eintrag.beschreibung = beschreibung.beschreibung;
        } catch (const std::exception& e) {
            eintrag.modell_id = ordnername.empty() ? ordner.filename().u8string() : ordnername;
            eintrag.name = eintrag.modell_id;
            diagnosen.push_back(diagnose_aus(e.what(), modellordner, "CORE-MOD-0005"));
        }
        try {
            eintrag.modell_fassung = modell_fassung(datei);
            if (diagnosen.empty()) lade_modell(datei);
        } catch (const std::exception& e) {
            diagnosen.push_back(diagnose_aus(e.what(), modellordner, "CORE-MOD-0005"));
        }
        katalog.eintraege_.push_back(std::move(eintrag));
        katalog.dateien_.push_back(datei);
        katalog.diagnosen_.push_back(std::move(diagnosen));
    }
    std::map<std::string, std::size_t> anzahl;
    for (const auto& e : katalog.eintraege_) ++anzahl[e.modell_id];
    for (std::size_t i = 0; i < katalog.eintraege_.size(); ++i) {
        auto& e = katalog.eintraege_[i];
        if (anzahl[e.modell_id] > 1)
            katalog.diagnosen_[i].push_back({"CORE-KAT-0002",
                "Modell-ID mehrfach im Ordner vergeben: " + e.modell_id,
                relativer_name(katalog.dateien_[i], ordner)});
        e.fehleranzahl = katalog.diagnosen_[i].size();
        e.status = e.fehleranzahl ? "fehlerhaft" : "verwendbar";
    }
    return katalog;
}

const std::filesystem::path& katalog_datei(const Modellkatalog& katalog,
    const std::string& modell_id, const std::string& erwartete_fassung)
{
    std::size_t treffer = katalog.eintraege_.size();
    for (std::size_t i = 0; i < katalog.eintraege_.size(); ++i)
        if (katalog.eintraege_[i].modell_id == modell_id) {
            if (treffer != katalog.eintraege_.size())
                kat_fehler(2, "Modell-ID mehrfach im Ordner vergeben: " + modell_id);
            treffer = i;
        }
    if (treffer == katalog.eintraege_.size()) kat_fehler(3, "Unbekannte Modell-ID: " + modell_id);
    const auto& datei = katalog.dateien_[treffer];
    std::string aktuell;
    try {
        aktuell = modell_fassung(datei);
    } catch (const std::exception&) {
        kat_fehler(4, "Modell " + modell_id + " ist nicht mehr lesbar; Katalog neu abrufen");
    }
    if (aktuell != erwartete_fassung)
        kat_fehler(4, "Modell " + modell_id + " wurde geaendert (erwartet " + erwartete_fassung +
            ", aktuell " + aktuell + "); Katalog neu abrufen");
    return datei;
}

std::vector<KatalogDiagnose> modelldiagnose(const Modellkatalog& katalog,
    const std::string& modell_id, const std::string& erwartete_fassung)
{
    const auto& datei = katalog_datei(katalog, modell_id, erwartete_fassung);
    for (std::size_t i = 0; i < katalog.dateien_.size(); ++i)
        if (&katalog.dateien_[i] == &datei) return katalog.diagnosen_[i];
    return {};
}

Modell katalogmodell(const Modellkatalog& katalog, const std::string& modell_id,
                     const std::string& erwartete_fassung)
{
    return lade_modell(katalog_datei(katalog, modell_id, erwartete_fassung));
}

Darstellungsgeometrie darstellungsgeometrie(const Modellkatalog& katalog,
    const std::string& modell_id, const std::string& erwartete_fassung,
    const std::optional<std::string>& element_id)
{
    const auto& datei = katalog_datei(katalog, modell_id, erwartete_fassung);
    const auto ordner = datei.parent_path();
    Darstellungsgeometrie result{modell_id, erwartete_fassung, {}, {}};
    ModellBeschreibung beschreibung;
    try {
        beschreibung = lies_beschreibung(datei);
    } catch (const std::exception& e) {
        if (element_id) kat_fehler(6, "Unbekannte Element-ID: " + *element_id);
        result.diagnosen.push_back(diagnose_aus(e.what(), ordner, "CORE-MOD-0005"));
        return result;
    }
    bool gefunden = false;
    for (const auto& e : beschreibung.elemente) {
        if (element_id && e.id != *element_id) continue;
        gefunden = true;
        const auto& quelle = e.darstellung ? e.darstellung : e.volumen;
        if (!quelle) continue;
        Darstellungsteil teil;
        teil.element_id = e.id;
        teil.gruppe = e.gruppe;
        teil.herkunft = e.darstellung ? "darstellung" : "volumen_ersatz";
        teil.farbe_rgb = e.farbe_rgb;
        teil.oberflaeche = e.oberflaeche;
        const auto name = relativer_name(*quelle, ordner);
        try {
            obj_darstellung(*quelle, name, teil);
            result.teile.push_back(std::move(teil));
        } catch (const std::exception& fehler) {
            result.diagnosen.push_back(diagnose_aus(fehler.what(), ordner, "CORE-KAT-0005"));
            result.diagnosen.back().fundstelle = "Element " + e.id + ", " + name;
        }
    }
    if (element_id && !gefunden) kat_fehler(6, "Unbekannte Element-ID: " + *element_id);
    return result;
}

Modellstruktur modellstruktur(const Modellkatalog& katalog, const std::string& modell_id,
                              const std::string& erwartete_fassung)
{
    const auto& datei = katalog_datei(katalog, modell_id, erwartete_fassung);
    Modellstruktur result;
    result.modell_id = modell_id;
    result.modell_fassung = erwartete_fassung;
    try {
        const auto beschreibung = lies_beschreibung(datei);
        for (const auto& gruppe : beschreibung.gruppen) result.gruppen.push_back({gruppe, gruppe, {}});
        for (const auto& e : beschreibung.elemente) {
            auto gruppe = std::find_if(result.gruppen.begin(), result.gruppen.end(),
                [&](const StrukturGruppe& g) { return g.id == e.gruppe; });
            if (gruppe != result.gruppen.end())
                gruppe->elemente.push_back({e.id, e.id, e.darstellung.has_value(),
                    e.volumen.has_value(), e.bewegung});
        }
    } catch (const std::exception&) {
        // Unreadable description: structure stays empty, see modelldiagnose.
    }
    const auto* eintrag = katalog.eintrag(modell_id);
    if (eintrag && eintrag->status == "verwendbar") {
        try { result.gesamtmasse_kg = lade_modell(datei).masse_kg; }
        catch (const std::exception&) {}
    }
    const auto geometrie = darstellungsgeometrie(katalog, modell_id, erwartete_fassung);
    for (const auto& teil : geometrie.teile)
        for (const auto& p : teil.punkte_m) {
            if (!result.huellquader_m) result.huellquader_m = std::array<Vektor3, 2>{p, p};
            for (std::size_t i = 0; i < 3; ++i) {
                (*result.huellquader_m)[0][i] = std::min((*result.huellquader_m)[0][i], p[i]);
                (*result.huellquader_m)[1][i] = std::max((*result.huellquader_m)[1][i], p[i]);
            }
        }
    return result;
}

std::string katalog_json(const Modellkatalog& katalog)
{
    auto out = json_strom();
    out << "{\"katalogversion\":\"1\",\"modelle\":[";
    for (std::size_t i = 0; i < katalog.eintraege().size(); ++i) {
        const auto& e = katalog.eintraege()[i];
        out << (i ? "," : "") << "{\"modell_id\":" << json_text(e.modell_id)
            << ",\"name\":" << json_text(e.name) << ",\"beschreibung\":"
            << (e.beschreibung ? json_text(*e.beschreibung) : std::string("null"))
            << ",\"modell_fassung\":" << json_text(e.modell_fassung)
            << ",\"status\":" << json_text(e.status) << ",\"fehleranzahl\":" << e.fehleranzahl << '}';
    }
    out << "]}";
    return out.str();
}

std::string modellstruktur_json(const Modellstruktur& s)
{
    auto out = json_strom();
    out << "{\"katalogversion\":\"1\",\"modell_id\":" << json_text(s.modell_id)
        << ",\"modell_fassung\":" << json_text(s.modell_fassung) << ",\"gruppen\":[";
    for (std::size_t g = 0; g < s.gruppen.size(); ++g) {
        const auto& gruppe = s.gruppen[g];
        out << (g ? "," : "") << "{\"id\":" << json_text(gruppe.id) << ",\"name\":" << json_text(gruppe.name)
            << ",\"elemente\":[";
        for (std::size_t i = 0; i < gruppe.elemente.size(); ++i) {
            const auto& e = gruppe.elemente[i];
            out << (i ? "," : "") << "{\"id\":" << json_text(e.id) << ",\"name\":" << json_text(e.name)
                << ",\"darstellung\":" << (e.darstellung ? "true" : "false")
                << ",\"hydrostatik_volumen\":" << (e.hydrostatik_volumen ? "true" : "false")
                << ",\"bewegung\":";
            if (!e.bewegung) out << "null";
            else {
                const auto& b = *e.bewegung;
                out << "{\"art\":" << json_text(b.art == Bewegungsart::drehen ? "drehen" : "heben")
                    << ",\"steuerung\":" << json_text(b.ruderwinkel ? "ruderwinkel" : "stellung_prozent");
                if (b.art == Bewegungsart::drehen) {
                    out << ",\"achse_modell\":";
                    zahlen(out, b.achse_modell);
                    out << ",\"drehpunkt_modell_m\":";
                    zahlen(out, b.drehpunkt_modell_m);
                } else {
                    out << ",\"richtung_modell\":";
                    zahlen(out, b.richtung_modell);
                    out << ",\"hubweg_m\":" << b.hubweg_m;
                }
                out << ",\"mitbewegte_elemente\":[";
                for (std::size_t j = 0; j < b.mitbewegte_elemente.size(); ++j)
                    out << (j ? "," : "") << json_text(b.mitbewegte_elemente[j]);
                out << "]}";
            }
            out << '}';
        }
        out << "]}";
    }
    out << "],\"gesamtmasse_kg\":";
    if (s.gesamtmasse_kg) out << *s.gesamtmasse_kg; else out << "null";
    out << ",\"huellquader_m\":";
    if (s.huellquader_m) {
        out << "{\"min\":";
        zahlen(out, (*s.huellquader_m)[0]);
        out << ",\"max\":";
        zahlen(out, (*s.huellquader_m)[1]);
        out << '}';
    } else out << "null";
    out << '}';
    return out.str();
}

namespace {
void diagnosen_json(std::ostream& out, const std::vector<KatalogDiagnose>& diagnosen)
{
    out << '[';
    for (std::size_t i = 0; i < diagnosen.size(); ++i)
        out << (i ? "," : "") << "{\"fehler_id\":" << json_text(diagnosen[i].fehler_id)
            << ",\"text\":" << json_text(diagnosen[i].text)
            << ",\"fundstelle\":" << json_text(diagnosen[i].fundstelle) << '}';
    out << ']';
}
}

std::string darstellungsgeometrie_json(const Darstellungsgeometrie& g)
{
    auto out = json_strom();
    out << "{\"katalogversion\":\"1\",\"modell_id\":" << json_text(g.modell_id)
        << ",\"modell_fassung\":" << json_text(g.modell_fassung) << ",\"einheit\":\"m\",\"frame\":\"modell\""
        << ",\"teile\":[";
    for (std::size_t t = 0; t < g.teile.size(); ++t) {
        const auto& teil = g.teile[t];
        out << (t ? "," : "") << "{\"element_id\":" << json_text(teil.element_id)
            << ",\"gruppe\":" << json_text(teil.gruppe) << ",\"herkunft\":" << json_text(teil.herkunft)
            << ",\"farbe_rgb\":";
        if (teil.farbe_rgb) zahlen(out, *teil.farbe_rgb); else out << "null";
        out << ",\"oberflaeche\":" << (teil.oberflaeche ? json_text(*teil.oberflaeche) : std::string("null"));
        out << ",\"punkte_m\":[";
        for (std::size_t i = 0; i < teil.punkte_m.size(); ++i) {
            if (i) out << ',';
            zahlen(out, teil.punkte_m[i]);
        }
        out << "],\"dreiecke\":[";
        for (std::size_t i = 0; i < teil.dreiecke.size(); ++i)
            out << (i ? "," : "") << '[' << teil.dreiecke[i][0] << ',' << teil.dreiecke[i][1] << ','
                << teil.dreiecke[i][2] << ']';
        out << "]}";
    }
    out << "],\"diagnosen\":";
    diagnosen_json(out, g.diagnosen);
    out << '}';
    return out.str();
}

std::string modelldiagnose_json(const std::string& modell_id, const std::string& modell_fassung,
                                const std::vector<KatalogDiagnose>& diagnosen)
{
    auto out = json_strom();
    out << "{\"katalogversion\":\"1\",\"modell_id\":" << json_text(modell_id)
        << ",\"modell_fassung\":" << json_text(modell_fassung) << ",\"diagnosen\":";
    diagnosen_json(out, diagnosen);
    out << '}';
    return out.str();
}

std::string katalogfehler_json(const std::string& diagnose)
{
    auto out = json_strom();
    out << "{\"katalogversion\":\"1\",\"fehler\":{\"fehler_id\":"
        << json_text(fehler_id(diagnose, "CORE-KAT-0001")) << ",\"text\":" << json_text(diagnose) << "}}";
    return out.str();
}

}
