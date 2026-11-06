#include "segelphysik/modell.hpp"

#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    const bool ableitungen = argc == 4 && std::string(argv[1]) == "--schema3-ableitungen";
    const bool schema3 = ableitungen || (argc == 4 && std::string(argv[1]) == "--schema3");
    if (argc != 3 && !schema3) {
        std::cerr << "Aufruf: segelphysik_konvertieren [--schema3|--schema3-ableitungen] <bootsmodell.json> <neuer Zielordner>\n";
        return 2;
    }
    try {
        const auto source = std::filesystem::u8path(argv[schema3 ? 2 : 1]);
        const auto target = std::filesystem::u8path(argv[schema3 ? 3 : 2]);
        if (schema3) segelphysik::konvertiere_modell_v3(source, target, ableitungen);
        else segelphysik::konvertiere_modell_v2(source, target);
        std::cout << "Schema " << (schema3 ? 3 : 2)
                  << ": bootsmodell.json und boot.obj erstellt; Original unveraendert.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
