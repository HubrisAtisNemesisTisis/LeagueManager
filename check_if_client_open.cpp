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
    HWND leagueClientHandle = NULL;
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
            settings.leagueClientHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);
            if (!settings.leagueClientHandle) {
                ERROR_LOG("Failed to open LeagueClient.exe process. Error Code: %lu", GetLastError());
                CloseHandle(hSnapshot);
                exit(EXIT_FAILURE);
            }
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
    waitUntilReadyAndLogin();
}
#endif

#define DB "RiotAccounts.db"
static sqlite3* db = NULL;

static void db_open() {
    if (sqlite3_open(DB, &db) != SQLITE_OK) {
        ERROR_LOG("Cannot open database: %s", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(EXIT_FAILURE);
    }
    const char *sqlCreateTableStr =
    "CREATE TABLE IF NOT EXISTS ACCOUNTS("
    "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
    "USERNAME TEXT UNIQUE NOT NULL,"
    "PASSWORD TEXT NOT NULL,"
    "COMMENT TEXT"
    ")";
    char* errMsg = NULL;
    if (sqlite3_exec(db, sqlCreateTableStr, NULL, NULL, &errMsg) != SQLITE_OK) {
        ERROR_LOG("SQL error: %s", errMsg);
        sqlite3_free(errMsg);
        sqlite3_close(db);
        exit(EXIT_FAILURE);
    }
}

static void db_close() {
    if (db) {
        sqlite3_close(db);
        db = NULL;
    }
}

static void add_account(const char* username, const char* password, const char* comment) {
    sqlite3_stmt* stmt = NULL;
    const char* sqlInsertStr = "INSERT INTO ACCOUNTS (USERNAME, PASSWORD, COMMENT) VALUES (?, ?, ?)";
    if (sqlite3_prepare_v2(db, sqlInsertStr, -1, &stmt, NULL) != SQLITE_OK) {
        ERROR_LOG("Failed to prepare statement: %s", sqlite3_errmsg(db));
        exit(EXIT_FAILURE);
    }
    if (sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC) != SQLITE_OK ||
        sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC) != SQLITE_OK ||
        sqlite3_bind_text(stmt, 3, comment, -1, SQLITE_STATIC) != SQLITE_OK) {
        ERROR_LOG("Failed to bind parameters: %s", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        exit(EXIT_FAILURE);
    }
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        ERROR_LOG("Failed to execute statement: %s", sqlite3_errmsg(db));
        exit(EXIT_FAILURE);
    }
}

static inline void flush_stdin(void){
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

int main() {

    db_open();
    printf("Choice : 1)Add account 2)Remove account 3)Choose accounts\n");
    int choice;
    printf("Select (1-3): ");
    if (scanf("%d", &choice) != 1) {        /* input was not a number */
        puts("Invalid input");
        return EXIT_FAILURE;
    }
    switch (choice) {
        case 1:    
            char username[256], password[256], comment[512];
            printf("Enter username: ");
            scanf("%255s", username);
            printf("Enter password: ");
            scanf("%255s", password);
            flush_stdin();
            printf("Enter comment (optional, press ENTER to skip): ");
            if (fgets(comment, sizeof comment, stdin)) {
                comment[strcspn(comment, "\r\n")] = '\0';
            }
            add_account(username, password, comment);
            INFO_LOG("Account added: %s", username);
            break;
        case 2:  /* remove account*/  break;
        case 3:  /* choose account*/  break;
        case 4: /* edit account */  break;
        default: puts("Choice must be 1, 2 or 3");  break;

    }
    
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
    db_close();
    free(settings.EXE_PATH);
    free(settings.EXE_ARGS);
    return 0;
}