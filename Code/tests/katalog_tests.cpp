#include "segelphysik/katalog.hpp"
#include "segelphysik/hydrostatik.hpp"

#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>

using namespace segelphysik;
namespace fs = std::filesystem;

namespace {
void pruefe(bool ok, const std::string& text)
{
    if (!ok) throw std::runtime_error(text);
}

template <class F>
std::string fehler_von(F&& aufruf)
{
    try {
        aufruf();
    } catch (const std::exception& e) {
        return e.what();
    }
    return {};
}

std::string lesen(const fs::path& datei)
{
    std::ifstream in(datei, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

void schreiben(const fs::path& datei, const std::string& text)
{
    std::ofstream out(datei, std::ios::binary);
    out << text;
}

// Only documented keys may appear (A-C-00042); answers contain no paths.
void nur_felder(const std::string& json, const std::set<std::string>& erlaubt, const fs::path& basis)
{
    for (std::size_t pos = json.find('"'); pos != std::string::npos; pos = json.find('"', pos + 1)) {
        const auto ende = json.find('"', pos + 1);
        if (ende + 1 < json.size() && json[ende + 1] == ':') {
            const auto feld = json.substr(pos + 1, ende - pos - 1);
            pruefe(erlaubt.count(feld) == 1, "Undokumentiertes Feld: " + feld);
        }
        pos = ende;
    }
    for (const auto& form : {basis.u8string(), basis.generic_u8string(), std::string(":\\\\"), std::string(":/")})
        pruefe(json.find(form) == std::string::npos, "Pfad in Antwort: " + form);
}

std::pair<std::size_t, std::size_t> obj_zahlen(const fs::path& datei)
{
    std::istringstream in(lesen(datei));
    std::string zeile;
    std::size_t punkte = 0, dreiecke = 0;
    while (std::getline(in, zeile)) {
        std::istringstream t(zeile);
        std::string art, token;
        t >> art;
        if (art == "v") ++punkte;
        if (art == "f") {
            std::size_t n = 0;
            while (t >> token) ++n;
            dreiecke += n - 2;
        }
    }
    return {punkte, dreiecke};
}

const std::set<std::string> katalogfelder{"katalogversion", "modelle", "modell_id", "name", "beschreibung",
    "modell_fassung", "status", "fehleranzahl"};

void beispielordner(const fs::path& ordner)
{
    const auto katalog = modellkatalog(ordner);
    pruefe(katalog.eintraege().size() == 11, "Beispielordner nicht vollstaendig");
    pruefe(katalog.eintraege().front().modell_id == "420", "Reihenfolge nicht nach Ordnername");
    for (std::size_t i = 1; i < katalog.eintraege().size(); ++i)
        pruefe(katalog.eintraege()[i - 1].modell_id < katalog.eintraege()[i].modell_id, "Reihenfolge instabil");
    const auto nochmal = modellkatalog(ordner);
    for (std::size_t i = 0; i < katalog.eintraege().size(); ++i) {
        const auto& e = katalog.eintraege()[i];
        pruefe(e.modell_id == nochmal.eintraege()[i].modell_id &&
            e.modell_fassung == nochmal.eintraege()[i].modell_fassung, "Katalog nicht reproduzierbar");
        if (e.status == "verwendbar") {
            pruefe(e.fehleranzahl == 0, "Verwendbares Modell mit Diagnosen");
            pruefe(katalogmodell(katalog, e.modell_id, e.modell_fassung).fassung == e.modell_fassung,
                "Katalogfassung weicht vom Ergebnisvertrag ab: " + e.modell_id);
        }
    }
    pruefe(katalog.eintrag("quader") && katalog.eintrag("quader")->status == "verwendbar", "Quader fehlt");
    nur_felder(katalog_json(katalog), katalogfelder, ordner);
}

void details(const fs::path& quelle, const fs::path& tmp)
{
    const auto ordner = tmp / "details";
    fs::create_directories(ordner);
    fs::copy(quelle / "quader", ordner / "q", fs::copy_options::recursive);
    auto katalog = modellkatalog(ordner);
    const auto fassung = katalog.eintrag("quader")->modell_fassung;
    const auto struktur = modellstruktur(katalog, "quader", fassung);
    pruefe(struktur.gruppen.size() == 1 && struktur.gruppen[0].id == "rumpf" &&
        struktur.gruppen[0].elemente.size() == 1 && struktur.gruppen[0].elemente[0].id == "Rumpf",
        "Struktur falsch");
    pruefe(struktur.gesamtmasse_kg && *struktur.gesamtmasse_kg > 0. && struktur.huellquader_m, "Masse/Huelle fehlt");
    nur_felder(modellstruktur_json(struktur), {"katalogversion", "modell_id", "modell_fassung", "gruppen", "id",
        "name", "elemente", "darstellung", "hydrostatik_volumen", "gesamtmasse_kg", "huellquader_m", "min", "max"},
        ordner);
    const auto geometrie = darstellungsgeometrie(katalog, "quader", fassung);
    const auto erwartet = obj_zahlen(ordner / "q" / "meshes" / "gui" / "rumpf.obj");
    pruefe(geometrie.teile.size() == 1 && geometrie.teile[0].herkunft == "darstellung" &&
        geometrie.teile[0].punkte_m.size() == erwartet.first &&
        geometrie.teile[0].dreiecke.size() == erwartet.second, "Darstellungsgeometrie weicht ab");
    nur_felder(darstellungsgeometrie_json(geometrie), {"katalogversion", "modell_id", "modell_fassung", "einheit",
        "frame", "teile", "element_id", "gruppe", "herkunft", "farbe_rgb", "oberflaeche", "punkte_m", "dreiecke", "diagnosen",
        "fehler_id", "text", "fundstelle"}, ordner);
    pruefe(darstellungsgeometrie(katalog, "quader", fassung, std::string("Rumpf")).teile.size() == 1,
        "Elementabfrage fehlt");
    pruefe(fehler_von([&] { darstellungsgeometrie(katalog, "quader", fassung, std::string("X")); })
        .rfind("CORE-KAT-0006", 0) == 0, "Unbekanntes Element ohne Fehler-ID");
    pruefe(fehler_von([&] { modellstruktur(katalog, "fehlt", fassung); }).rfind("CORE-KAT-0003", 0) == 0,
        "Unbekannte Modell-ID ohne Fehler-ID");
    pruefe(modelldiagnose(katalog, "quader", fassung).empty(), "Diagnose fuer gueltiges Modell");

    // Run via catalogue object and model ID.
    const auto modell = katalogmodell(katalog, "quader", fassung);
    pruefe(gleichgewicht(modell, Zustand{}).status == "konvergiert", "Lauf ueber Modell-ID fehlgeschlagen");

    // Changed model file: the old version is rejected, a new catalogue works.
    schreiben(ordner / "q" / "modell.json", lesen(ordner / "q" / "modell.json") + "\n");
    pruefe(fehler_von([&] { katalogmodell(katalog, "quader", fassung); }).rfind("CORE-KAT-0004", 0) == 0,
        "Veraltete Fassung nicht erkannt");
    pruefe(fehler_von([&] { darstellungsgeometrie(katalog, "quader", fassung); }).rfind("CORE-KAT-0004", 0) == 0,
        "Detailanfrage mit veralteter Fassung");
    katalog = modellkatalog(ordner);
    const auto neu = katalog.eintrag("quader")->modell_fassung;
    pruefe(neu != fassung && katalogmodell(katalog, "quader", neu).fassung == neu, "Neue Fassung fehlt");

    // Without display mesh the volume mesh is delivered as substitute.
    auto json = lesen(ordner / "q" / "modell.json");
    const auto anfang = json.find("\"Darstellung\"");
    json.erase(anfang, json.find("},", anfang) + 2 - anfang);
    schreiben(ordner / "q" / "modell.json", json);
    katalog = modellkatalog(ordner);
    const auto ersatz = darstellungsgeometrie(katalog, "quader", katalog.eintrag("quader")->modell_fassung);
    pruefe(ersatz.teile.size() == 1 && ersatz.teile[0].herkunft == "volumen_ersatz", "Volumenersatz fehlt");
}

void farben(const fs::path& quelle, const fs::path& tmp)
{
    const auto ordner = tmp / "farben";
    fs::create_directories(ordner);
    fs::copy(quelle / "quader", ordner / "q", fs::copy_options::recursive);
    const auto datei = ordner / "q" / "modell.json";
    // Vorlage ohne Darstellungsstil: farbe/oberflaeche der Beispieldatei entfernen.
    const auto original = std::regex_replace(lesen(datei),
        std::regex(R"re(,\s*"(farbe|oberflaeche)": "[^"]*")re"), "");
    schreiben(datei, original);
    const std::string feld = "\"datei\": \"meshes/gui/rumpf.obj\"";
    pruefe(original.find(feld) != std::string::npos, "Testvorlage ohne Darstellungsdatei");

    auto katalog = modellkatalog(ordner);
    pruefe(!darstellungsgeometrie(katalog, "quader", katalog.eintrag("quader")->modell_fassung)
        .teile[0].farbe_rgb, "Farbe ohne Angabe geliefert");

    auto json = original;
    json.replace(json.find(feld), feld.size(), feld + ", \"farbe\": \"#FF8000\"");
    schreiben(datei, json);
    katalog = modellkatalog(ordner);
    const auto fassung = katalog.eintrag("quader")->modell_fassung;
    const auto teil = darstellungsgeometrie(katalog, "quader", fassung).teile[0];
    pruefe(teil.farbe_rgb && std::abs((*teil.farbe_rgb)[0] - 1.) < 1e-12 &&
        std::abs((*teil.farbe_rgb)[1] - 128. / 255.) < 1e-12 && (*teil.farbe_rgb)[2] == 0.,
        "Farbe nicht als RGB 0..1 geliefert");
    pruefe(darstellungsgeometrie_json(darstellungsgeometrie(katalog, "quader", fassung))
        .find("\"farbe_rgb\":[1") != std::string::npos, "farbe_rgb fehlt im JSON");
    pruefe(katalogmodell(katalog, "quader", fassung).masse_kg > 0., "Modell mit Farbe nicht ladbar");

    json = original;
    json.replace(json.find(feld), feld.size(), feld + ", \"farbe\": \"orange\"");
    schreiben(datei, json);
    katalog = modellkatalog(ordner);
    const auto falsch = katalog.eintrag("quader")->modell_fassung;
    pruefe(katalog.eintrag("quader")->status == "fehlerhaft", "Ungueltige Farbe nicht erkannt");
    pruefe(fehler_von([&] { katalogmodell(katalog, "quader", falsch); }).rfind("CORE-MOD-0002", 0) == 0,
        "Ungueltige Farbe ohne CORE-MOD-0002");
    pruefe(!darstellungsgeometrie(katalog, "quader", falsch).teile[0].farbe_rgb,
        "Ungueltige Farbe in Darstellung geliefert");

    json = original;
    json.replace(json.find(feld), feld.size(), feld + ", \"oberflaeche\": \"metall\"");
    schreiben(datei, json);
    katalog = modellkatalog(ordner);
    const auto metall = katalog.eintrag("quader")->modell_fassung;
    const auto metallteil = darstellungsgeometrie(katalog, "quader", metall).teile[0];
    pruefe(metallteil.oberflaeche && *metallteil.oberflaeche == "metall", "Oberflaeche nicht geliefert");
    pruefe(darstellungsgeometrie_json(darstellungsgeometrie(katalog, "quader", metall))
        .find("\"oberflaeche\":\"metall\"") != std::string::npos, "oberflaeche fehlt im JSON");

    for (const std::string art : {"chrom", "lack", "glas", "leuchtend", "schillernd", "facettiert",
             "drahtgitter", "perlmutt"}) {
        json = original;
        json.replace(json.find(feld), feld.size(), feld + ", \"oberflaeche\": \"" + art + "\"");
        schreiben(datei, json);
        katalog = modellkatalog(ordner);
        const auto teil_art = darstellungsgeometrie(
            katalog, "quader", katalog.eintrag("quader")->modell_fassung).teile[0];
        pruefe(teil_art.oberflaeche && *teil_art.oberflaeche == art, "Effektoberflaeche abgelehnt: " + art);
    }

    json = original;
    json.replace(json.find(feld), feld.size(), feld + ", \"oberflaeche\": \"regenbogen\"");
    schreiben(datei, json);
    katalog = modellkatalog(ordner);
    pruefe(katalog.eintrag("quader")->status == "fehlerhaft", "Ungueltige Oberflaeche nicht erkannt");
    pruefe(!darstellungsgeometrie(katalog, "quader", katalog.eintrag("quader")->modell_fassung)
        .teile[0].oberflaeche, "Ungueltige Oberflaeche geliefert");
}

void fehlerfaelle(const fs::path& quelle, const fs::path& tmp)
{
    const auto ordner = tmp / "fehler";
    fs::create_directories(ordner);
    fs::copy(quelle / "quader", ordner / "a", fs::copy_options::recursive);
    fs::copy(quelle / "quader", ordner / "b", fs::copy_options::recursive);
    fs::copy(quelle / "quader", ordner / "c", fs::copy_options::recursive);
    fs::create_directories(ordner / "d_ohne_modell");
    fs::create_directories(ordner / "e_kaputt");
    schreiben(ordner / "e_kaputt" / "modell.json", "{");
    auto json = lesen(ordner / "c" / "modell.json");
    json.replace(json.find("\"id\": \"quader\""), 14, "\"id\": \"quader_c\"");
    schreiben(ordner / "c" / "modell.json", json);
    fs::remove(ordner / "c" / "meshes" / "masse" / "rumpf.obj");

    const auto katalog = modellkatalog(ordner);
    pruefe(katalog.eintraege().size() == 4, "Unterordner ohne modell.json mitgezaehlt");
    for (std::size_t i : {0u, 1u}) {
        const auto& e = katalog.eintraege()[i];
        pruefe(e.status == "fehlerhaft" && e.fehleranzahl > 0, "ID-Duplikat nicht fehlerhaft");
        const auto d = modelldiagnose(katalog, "quader_c", katalog.eintraege()[2].modell_fassung);
        pruefe(!d.empty() && d[0].fehler_id == "CORE-MOD-0005", "Fehlende Massendatei nicht diagnostiziert");
    }
    const auto& c = katalog.eintraege()[2];
    pruefe(c.modell_id == "quader_c" && c.status == "fehlerhaft", "Fehlerhaftes Modell nicht markiert");
    const auto geometrie = darstellungsgeometrie(katalog, "quader_c", c.modell_fassung);
    pruefe(geometrie.teile.size() == 1, "Geometrie fehlerhafter Modelle nicht lesbar");
    pruefe(fehler_von([&] { katalogmodell(katalog, "quader_c", c.modell_fassung); }).rfind("CORE-MOD-0005", 0) == 0,
        "Lauf eines fehlerhaften Modells nicht abgelehnt");
    const auto& kaputt = katalog.eintraege()[3];
    pruefe(kaputt.modell_id == "e_kaputt" && kaputt.status == "fehlerhaft", "Kaputtes JSON nicht fehlerhaft");
    const auto diagnose = modelldiagnose_json("quader_c", c.modell_fassung,
        modelldiagnose(katalog, "quader_c", c.modell_fassung));
    nur_felder(diagnose, {"katalogversion", "modell_id", "modell_fassung", "diagnosen", "fehler_id", "text",
        "fundstelle"}, ordner);
    nur_felder(katalog_json(katalog), katalogfelder, ordner);
    const auto meldung = fehler_von([&] { modellkatalog(tmp / "gibt-es-nicht"); });
    pruefe(meldung.rfind("CORE-KAT-0001", 0) == 0, "Nicht lesbarer Ordner ohne Fehler-ID");
    pruefe(katalogfehler_json(meldung).find("\"fehler_id\":\"CORE-KAT-0001\"") != std::string::npos,
        "Fehlerantwort ohne Fehler-ID");
}
}

int main(int argc, char* argv[])
{
    if (argc != 2) return 2;
    const auto tmp = fs::temp_directory_path() /
        ("segel-katalog-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    int status = 0;
    try {
        beispielordner(argv[1]);
        details(argv[1], tmp);
        farben(argv[1], tmp);
        fehlerfaelle(argv[1], tmp);
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        status = 1;
    }
    std::error_code ignoriert;
    fs::remove_all(tmp, ignoriert);
    return status;
}
