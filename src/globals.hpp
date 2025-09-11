#pragma once
// Include Windows-specific headers
#ifdef _WIN32
#include <windows.h>
#include <uiautomation.h>
#include <tlhelp32.h>
#endif

/* Include Standard Library */
#include <cstdlib>
#include <cstdio>
#include <ctime>

/* Include EventLogger_HPP */
#define LOG_LEVEL LOG_LEVEL_DEBUG
#include "eventlogger.hpp"


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
};

extern Settings settings; 


/* Global Functions */
void loadSettings();