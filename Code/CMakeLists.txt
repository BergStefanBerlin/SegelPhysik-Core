cmake_minimum_required(VERSION 3.20)
project(SegelPhysikCore LANGUAGES CXX)

include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
include(FetchContent)
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
FetchContent_Declare(core_jsoncons
    URL https://codeload.github.com/danielaparker/jsoncons/tar.gz/refs/tags/v1.10.0
    URL_HASH SHA256=c829350a2eece662beb378143e2ab1c1cbb07d703687bf168626665a8a7d8bc1)
FetchContent_Declare(core_picosha2
    URL https://codeload.github.com/okdshin/PicoSHA2/tar.gz/refs/tags/v1.0.1
    URL_HASH SHA256=9983136544234e573fe07cc1a22fdf978ad7979043e7be2e9082f6d4991ff8a8)
set(JSONCONS_BUILD_TESTS OFF CACHE BOOL "Build dependency tests" FORCE)
FetchContent_MakeAvailable(core_jsoncons core_picosha2)
set(SEGELPHYSIK_MODELLE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../Modelle_alt"
    CACHE PATH "Legacy model fixtures archived under the model repository")
option(SEGELPHYSIK_TEST_INSTRUMENTATION "Prepare instrumentation for explicit test runs" OFF)
if(SEGELPHYSIK_TEST_INSTRUMENTATION AND NOT BUILD_TESTING)
    message(FATAL_ERROR "Test instrumentation requires BUILD_TESTING=ON")
endif()

set(fehlerkatalog "${CMAKE_CURRENT_SOURCE_DIR}/ressourcen/fehlerkatalog.json")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${fehlerkatalog}")
file(READ "${fehlerkatalog}" FEHLERKATALOG_JSON)
string(JSON katalogversion GET "${FEHLERKATALOG_JSON}" vertragsversion)
if(NOT katalogversion STREQUAL "1")
    message(FATAL_ERROR "Unsupported error catalog version")
endif()
string(JSON fehlerzahl LENGTH "${FEHLERKATALOG_JSON}" fehler)
if(fehlerzahl LESS 1)
    message(FATAL_ERROR "Error catalog must not be empty")
endif()
math(EXPR letzter_fehler "${fehlerzahl} - 1")
set(fehler_ids)
foreach(index RANGE ${letzter_fehler})
    foreach(feld id kurztext pass_kriterium fail_kriterium handlungsanweisung pruefstatus)
        string(JSON feldtyp TYPE "${FEHLERKATALOG_JSON}" fehler ${index} ${feld})
        string(JSON feldwert GET "${FEHLERKATALOG_JSON}" fehler ${index} ${feld})
        if(NOT feldtyp STREQUAL "STRING" OR feldwert STREQUAL "")
            message(FATAL_ERROR "Error catalog entry ${index}: invalid ${feld}")
        endif()
    endforeach()
    if(NOT feldwert STREQUAL "implementiert" AND NOT feldwert STREQUAL "geplant")
        message(FATAL_ERROR "Invalid error catalog implementation status")
    endif()
    string(JSON fehler_id GET "${FEHLERKATALOG_JSON}" fehler ${index} id)
    if(NOT fehler_id MATCHES "^CORE-[A-Z]+-[0-9][0-9][0-9][0-9]$" OR fehler_id IN_LIST fehler_ids)
        message(FATAL_ERROR "Invalid or duplicate error ID: ${fehler_id}")
    endif()
    list(APPEND fehler_ids "${fehler_id}")
endforeach()
string(FIND "${FEHLERKATALOG_JSON}" ")CORE_ERRORS\"" delimiter_position)
if(NOT delimiter_position EQUAL -1)
    message(FATAL_ERROR "Error catalog contains reserved source delimiter")
endif()
# MSVC limits individual string literals to 16 KiB; preserve bytes in adjacent lines.
string(REPLACE "\n" "\n)CORE_ERRORS\"\nR\"CORE_ERRORS("
    FEHLERKATALOG_JSON "${FEHLERKATALOG_JSON}")
configure_file(src/fehlerkatalog.hpp.in generated/fehlerkatalog.hpp @ONLY)

add_library(segelphysik_core STATIC src/core.cpp src/pose.cpp src/modell.cpp
    src/modellformat_03.cpp src/hydrostatik.cpp src/hydrotraegheit.cpp src/messung.cpp
    src/simulation.cpp src/katalog.cpp
    src/inspektion.cpp src/aufbereitung.cpp src/segelkonfigurator.cpp)
target_include_directories(segelphysik_core PUBLIC include)
target_include_directories(segelphysik_core PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_include_directories(segelphysik_core SYSTEM PRIVATE
    "${core_jsoncons_SOURCE_DIR}/include" "${core_picosha2_SOURCE_DIR}")
target_link_libraries(segelphysik_core PUBLIC nlohmann_json::nlohmann_json)
target_compile_features(segelphysik_core PUBLIC cxx_std_17)
set_target_properties(segelphysik_core PROPERTIES CXX_EXTENSIONS OFF)
if(SEGELPHYSIK_TEST_INSTRUMENTATION)
    target_compile_definitions(segelphysik_core PUBLIC SEGELPHYSIK_TEST_INSTRUMENTATION=1)
endif()

add_executable(segelphysik_cli src/cli.cpp)
target_link_libraries(segelphysik_cli PRIVATE segelphysik_core)
add_executable(segelphysik_konvertieren src/konvertieren.cpp)
target_link_libraries(segelphysik_konvertieren PRIVATE segelphysik_core)

set(core_targets segelphysik_core segelphysik_cli segelphysik_konvertieren)
if(BUILD_TESTING)
    set(SEGELPHYSIK_MODELFORMAT03_TEST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../Modelle"
        CACHE PATH "Active model format 0.3 test data root")
    add_executable(aufbereitung_tests tests/aufbereitung_tests.cpp)
    target_link_libraries(aufbereitung_tests PRIVATE segelphysik_core)
    list(APPEND core_targets aufbereitung_tests)
    add_test(NAME modellaufbereitung COMMAND aufbereitung_tests
        "${SEGELPHYSIK_MODELLE_DIR}/420/modells_information.json"
        "${SEGELPHYSIK_MODELLE_DIR}/folkeboot/modells_information.json")
    add_executable(aufbereitung_benchmark tests/aufbereitung_benchmark.cpp)
    target_link_libraries(aufbereitung_benchmark PRIVATE segelphysik_core)
    list(APPEND core_targets aufbereitung_benchmark)
    set(SEGELPHYSIK_PERFORMANCE_BASELINE_LIBRARY "" CACHE FILEPATH
        "Optional frozen C-0.9 library for the identical performance runner")
    if(SEGELPHYSIK_PERFORMANCE_BASELINE_LIBRARY)
        add_executable(aufbereitung_referenz tests/aufbereitung_benchmark.cpp)
        target_include_directories(aufbereitung_referenz PRIVATE include)
        target_compile_features(aufbereitung_referenz PRIVATE cxx_std_17)
        target_compile_definitions(aufbereitung_referenz PRIVATE CORE_PERFORMANCE_BASELINE=1)
        target_link_libraries(aufbereitung_referenz PRIVATE
            "${SEGELPHYSIK_PERFORMANCE_BASELINE_LIBRARY}" nlohmann_json::nlohmann_json)
        list(APPEND core_targets aufbereitung_referenz)
    endif()
    add_executable(core_tests tests/core_tests.cpp)
    target_link_libraries(core_tests PRIVATE segelphysik_core)
    if(SEGELPHYSIK_TEST_INSTRUMENTATION)
        target_compile_definitions(core_tests PRIVATE SEGELPHYSIK_EXPECT_INSTRUMENTATION=1)
    endif()
    list(APPEND core_targets core_tests)
    add_executable(pose_tests tests/pose_tests.cpp)
    target_link_libraries(pose_tests PRIVATE segelphysik_core)
    list(APPEND core_targets pose_tests)
    add_test(NAME pose_transformations COMMAND pose_tests)
    add_executable(physik_tests tests/physik_tests.cpp)
    target_link_libraries(physik_tests PRIVATE segelphysik_core)
    list(APPEND core_targets physik_tests)
    add_test(NAME hydrostatik_referenzen COMMAND physik_tests)
    add_executable(hydrodynamik_tests tests/hydrodynamik_tests.cpp)
    target_link_libraries(hydrodynamik_tests PRIVATE segelphysik_core)
    list(APPEND core_targets hydrodynamik_tests)
    add_test(NAME hydrodynamik_referenzen COMMAND hydrodynamik_tests)
    add_test(NAME hydrodynamik_referenzmodelle COMMAND hydrodynamik_tests
        "${SEGELPHYSIK_MODELLE_DIR}/optimist/modells_information.json"
        "${SEGELPHYSIK_MODELLE_DIR}/420/modells_information.json"
        "${SEGELPHYSIK_MODELLE_DIR}/folkeboot/modells_information.json"
        "${SEGELPHYSIK_MODELLE_DIR}/bleistift/modells_information.json"
        "${SEGELPHYSIK_MODELLE_DIR}/kegel/modells_information.json"
        "${SEGELPHYSIK_MODELLE_DIR}/kugel/modells_information.json"
        "${SEGELPHYSIK_MODELLE_DIR}/platte/modells_information.json"
        "${SEGELPHYSIK_MODELLE_DIR}/pyramide/modells_information.json"
        "${SEGELPHYSIK_MODELLE_DIR}/quader/modells_information.json"
        "${SEGELPHYSIK_MODELLE_DIR}/rolle/modells_information.json"
        "${SEGELPHYSIK_MODELLE_DIR}/unregelmaessig/modells_information.json")
    add_executable(lateraldruckpunkt_tests tests/lateraldruckpunkt_tests.cpp)
    target_link_libraries(lateraldruckpunkt_tests PRIVATE segelphysik_core)
    list(APPEND core_targets lateraldruckpunkt_tests)
    add_test(NAME lateraldruckpunkt COMMAND lateraldruckpunkt_tests)
    add_executable(traegheit_tests tests/traegheit_tests.cpp)
    target_link_libraries(traegheit_tests PRIVATE segelphysik_core)
    list(APPEND core_targets traegheit_tests)
    add_test(NAME hydrodynamik_traegheit COMMAND traegheit_tests
        "${SEGELPHYSIK_MODELLE_DIR}/quader/modells_information.json"
        "${SEGELPHYSIK_MODELLE_DIR}/kugel/modells_information.json"
        "${SEGELPHYSIK_MODELLE_DIR}/optimist/modells_information.json"
        "${SEGELPHYSIK_MODELLE_DIR}/420/modells_information.json"
        "${SEGELPHYSIK_MODELLE_DIR}/folkeboot/modells_information.json")
    add_executable(simulation_tests tests/simulation_tests.cpp)
    target_link_libraries(simulation_tests PRIVATE segelphysik_core)
    list(APPEND core_targets simulation_tests)
    add_executable(stationaere_kurvenfahrt_tests tests/stationaere_kurvenfahrt_tests.cpp)
    target_link_libraries(stationaere_kurvenfahrt_tests PRIVATE segelphysik_core)
    list(APPEND core_targets stationaere_kurvenfahrt_tests)
    add_executable(testantrieb_tests tests/testantrieb_tests.cpp)
    target_link_libraries(testantrieb_tests PRIVATE segelphysik_core)
    list(APPEND core_targets testantrieb_tests)
    add_test(NAME temporaerer_testantrieb COMMAND testantrieb_tests
        "${SEGELPHYSIK_MODELLE_DIR}/quader/modells_information.json"
        "${SEGELPHYSIK_MODELLE_DIR}/420/modells_information.json")
    add_test(NAME simulation_gesamtlauf COMMAND simulation_tests
        "${SEGELPHYSIK_MODELLE_DIR}/quader/modells_information.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/testdata/stationaere_referenz/modells_information.json"
        "${SEGELPHYSIK_MODELLE_DIR}/420/modells_information.json")
    add_test(NAME stationaere_kurvenfahrt COMMAND stationaere_kurvenfahrt_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/testdata/stationaere_kurvenreferenz/modells_information.json"
        "${SEGELPHYSIK_MODELLE_DIR}/420/modells_information.json")
    add_executable(core_benchmark tests/benchmark.cpp)
    target_link_libraries(core_benchmark PRIVATE segelphysik_core)
    list(APPEND core_targets core_benchmark)
    add_executable(modell_tests tests/modell_tests.cpp)
    target_link_libraries(modell_tests PRIVATE segelphysik_core)
    list(APPEND core_targets modell_tests)
    add_test(NAME modell_import COMMAND modell_tests
        "${SEGELPHYSIK_MODELLE_DIR}/quader/modells_information.json")
    add_test(NAME core_build_configuration COMMAND core_tests)
    add_executable(bilanz_tests tests/bilanz_tests.cpp)
    target_link_libraries(bilanz_tests PRIVATE segelphysik_core)
    list(APPEND core_targets bilanz_tests)
    add_test(NAME kraft_momentenbilanz COMMAND bilanz_tests
        "${SEGELPHYSIK_MODELLE_DIR}/quader/modells_information.json")
    add_executable(katalog_tests tests/katalog_tests.cpp)
    target_link_libraries(katalog_tests PRIVATE segelphysik_core)
    list(APPEND core_targets katalog_tests)
    add_executable(modellformat_03_tests tests/modellformat_03_tests.cpp)
    target_link_libraries(modellformat_03_tests PRIVATE segelphysik_core)
    add_test(NAME modellformat_03_fingerprints COMMAND modellformat_03_tests
        "${SEGELPHYSIK_MODELFORMAT03_TEST_ROOT}")
    add_test(NAME modellformat_03_schemabestand COMMAND modellformat_03_tests
        --source-schemas "${SEGELPHYSIK_MODELFORMAT03_TEST_ROOT}")
    add_test(NAME modellformat_03_fehlermatrix COMMAND modellformat_03_tests
        --schema-error-matrix "${SEGELPHYSIK_MODELFORMAT03_TEST_ROOT}")
    add_test(NAME modellformat_03_source_integration COMMAND modellformat_03_tests
        --source-integration "${SEGELPHYSIK_MODELFORMAT03_TEST_ROOT}")
    list(APPEND core_targets modellformat_03_tests)
    add_test(NAME modellkatalog_schnittstelle COMMAND katalog_tests
        "${SEGELPHYSIK_MODELLE_DIR}")
    add_executable(inspektion_tests tests/inspektion_tests.cpp)
    target_link_libraries(inspektion_tests PRIVATE segelphysik_core)
    list(APPEND core_targets inspektion_tests)
    add_test(NAME modellinspektion_c08 COMMAND inspektion_tests
        "${SEGELPHYSIK_MODELLE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
    add_executable(modell_v2_tests tests/modell_v2_tests.cpp)
    target_link_libraries(modell_v2_tests PRIVATE segelphysik_core)
    list(APPEND core_targets modell_v2_tests)
    add_test(NAME modell_geometrie_v2 COMMAND modell_v2_tests
        "${SEGELPHYSIK_MODELLE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
    add_executable(segelkonfigurator_tests tests/segelkonfigurator_tests.cpp)
    target_link_libraries(segelkonfigurator_tests PRIVATE segelphysik_core)
    list(APPEND core_targets segelkonfigurator_tests)
    add_test(NAME segelkonfiguration_contract COMMAND segelkonfigurator_tests
        "${SEGELPHYSIK_MODELLE_DIR}/420/modells_information.json")
    set_tests_properties(modell_import kraft_momentenbilanz modellkatalog_schnittstelle
        modellinspektion_c08 modell_geometrie_v2 segelkonfiguration_contract
        PROPERTIES LABELS "legacy-compatibility")
    set_tests_properties(modellformat_03_fingerprints modellformat_03_schemabestand
        modellformat_03_fehlermatrix modellformat_03_source_integration
        PROPERTIES LABELS "model-format-0.3")
    add_test(NAME segel_mittschiffs_basis COMMAND segelkonfigurator_tests
        --zero-base-contract "${SEGELPHYSIK_MODELLE_DIR}")
    add_executable(montage_tests tests/montage_tests.cpp)
    target_link_libraries(montage_tests PRIVATE segelphysik_core)
    list(APPEND core_targets montage_tests)
    add_test(NAME montage_alle_objekte COMMAND montage_tests
        "${SEGELPHYSIK_MODELLE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
    foreach(scenario help build_info error_catalog no_arguments invalid_argument extra_argument)
        add_test(NAME cli_${scenario}
            COMMAND "${CMAKE_COMMAND}"
                "-DCLI=$<TARGET_FILE:segelphysik_cli>"
                "-DSCENARIO=${scenario}"
                "-DINSTRUMENTATION=${SEGELPHYSIK_TEST_INSTRUMENTATION}"
                "-DCATALOG=${fehlerkatalog}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/cli_test.cmake")
    endforeach()
endif()

foreach(target IN LISTS core_targets)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX /permissive- /fp:precise /utf-8)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endforeach()
