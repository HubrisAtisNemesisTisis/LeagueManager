/* Global Includes and Variables*/
#include "globals.hpp"
/* Include Simd Library header */
#include <simdjson.h>

/* Helpers */
#include "charStrToWideStr.hpp" 

void loadSettings() {
    using namespace simdjson;
    dom::parser parser;
    dom::element settingsJson;
    auto error = parser.load("settings.json").get(settingsJson);
    if (error) {
        ERROR_LOG("Failed to load settings.json: %s", error_message(error));
        exit(EXIT_FAILURE);
    }
    std::string_view str_view;
    error = settingsJson["EXE_PATH"].get_string().get(str_view);
    if (error) {
        ERROR_LOG("Failed to get EXE_PATH from settings.json: %s", error_message(error));
        exit(EXIT_FAILURE);
    }
    DEBUG_LOG("Printing EXE_PATH string_view: %.*s", (int)str_view.size(), str_view.data());
    settings.EXE_PATH = to_wide_string(str_view.data(), str_view.size());
    INFO_LOG("Loaded EXE_PATH: %ls", settings.EXE_PATH);
    error = settingsJson["EXE_ARGS"].get_string().get(str_view);
    if (error) {
        ERROR_LOG("Failed to get EXE_ARGS from settings.json: %s", error_message(error));
        exit(EXIT_FAILURE);
    }
    DEBUG_LOG("Printing EXE_ARGS string_view: %.*s", (int)str_view.size(), str_view.data());
    settings.EXE_ARGS = to_wide_string(str_view.data(), str_view.size());
    INFO_LOG("Loaded EXE_ARGS: %ls", settings.EXE_ARGS);
    INFO_LOG("Settings loaded successfully.");
}