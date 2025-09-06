// Include Windows-specific headers
#ifdef _WIN32
#include <windows.h>
#include <uiautomation.h>
#include <tlhelp32.h>
#endif
// Include Standard C++ headers
#ifdef __cplusplus
#include <cwchar>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <vector>
using std::vector;
#endif
// LIbrary headers
#include <simdjson.h>
#include <sqlite3.h>
// Include and Configure eventlogger
#define LOG_LEVEL LOG_LEVEL_DEBUG
#include "eventlogger.hpp"
time_t __logger_time = 0;


enum ClientAction {
    OPEN_CLIENT = 0,
    LOGIN_TO_CLIENT,
    NONE
};

struct Settings {
    wchar_t* EXE_PATH = NULL;
    wchar_t* EXE_ARGS = NULL;
    DWORD leagueClient_PID = 0;
    HWND riotClientHandle = NULL;
} settings;

#ifdef _WIN32
#include "helpers.h"

void load_settings() {
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

ClientAction checkRequiredClientAction() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        ERROR_LOG("Failed to create process snapshot. Error Code: %lu", GetLastError());
        exit(EXIT_FAILURE);
    }
    PROCESSENTRY32W processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32W);
    if (!Process32FirstW(hSnapshot, &processEntry)) {
        ERROR_LOG("Failed to retrieve the first process. Error Code: %lu", GetLastError());
        CloseHandle(hSnapshot);
        exit(EXIT_FAILURE);   
    }
    do {
        if (wcscmp(processEntry.szExeFile, L"LeagueClient.exe") == 0) {
            settings.leagueClient_PID = processEntry.th32ProcessID;
            INFO_LOG("LeagueClient.exe is running with PID: %lu", settings.leagueClient_PID);
            CloseHandle(hSnapshot);
            return NONE;
        }else if (wcscmp(processEntry.szExeFile, L"Riot Client.exe") == 0) {
            INFO_LOG("Riot Client.exe is running\nNeed to log in");
            CloseHandle(hSnapshot);
            return LOGIN_TO_CLIENT;
        }
    } while (Process32NextW(hSnapshot, &processEntry));
    INFO_LOG("LeagueClient.exe is not running.\nAttempting to open it...");
    CloseHandle(hSnapshot);
    return OPEN_CLIENT;
}

void openLeagueClientAndLogin() {
    STARTUPINFOW startupInfo = {};
    startupInfo.cb = sizeof(STARTUPINFOW);
    PROCESS_INFORMATION processInfo = {};
    if (!CreateProcessW(settings.EXE_PATH, settings.EXE_ARGS, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo)) {
        ERROR_LOG("Failed to launch Riot Client. Error Code: %lu", GetLastError());
        exit(EXIT_FAILURE);
    }
    INFO_LOG("Riot Client launched (PID %lu). Waiting...", processInfo.dwProcessId);
    waitUntilForegroundWindow(L"Riot Client");
    Sleep(2000); // Additional wait to ensure the window is fully ready
    waitUntilReadyAndLogin();
}
#endif


int main() {
    printf("Choice : 1)Add account 2)Remove account 3)Choose accounts\n");
    load_settings();
    ClientAction action = checkRequiredClientAction();

    if(action == OPEN_CLIENT) {
        openLeagueClientAndLogin();
        action = LOGIN_TO_CLIENT;   
    } else if(action == LOGIN_TO_CLIENT) {
        bringRiotClientToForeground();
    } else {
        
        INFO_LOG("No action required. Client is already running.");
    }
    
    
    
    
    
    
    
    // if(action == OPEN_CLIENT) {
    //     openLeagueClient();
    // } else if(action == LOGIN_TO_CLIENT) {
    //     Sleep(20000); // Wait for 20 seconds to allow the client to be ready for input
    //     HWND hwnd = GetForegroundWindow();
    //     if (!hwnd) return false;
    //     wchar_t title[256] = {};
    //     GetWindowTextW(hwnd, title, 256);
    //     printf("Current active window title: %ls\n", title);
    // } else {
    //     INFO_LOG("No action required. Client is already running.");
    // }
    free(settings.EXE_PATH);
    free(settings.EXE_ARGS);
    return 0;
}