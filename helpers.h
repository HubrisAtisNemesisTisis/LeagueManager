/* Convert const char* to wchar_t 
-----------------------------------
used at: load_settings()
-----------------------------------
*/
static inline wchar_t* to_wide_string(const char* str, size_t len) {
    /* MultiByteToWideChar( CodePage to use performing conversion, 
                            Flag indicating type of conversion, 
                            String to convert, 
                            Size of string to convert in bytes or - 1 if null-terminated, 
                            Return pointer wide char buffer or NULL to get required size,
                            Size of wide char buffer in wide chars or 0 if getting required size
        );
    */
    int wlen = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, str, (int)len, NULL, 0);
    if (wlen <= 0) {
        ERROR_LOG("Failed to convert string to wide string.");
        exit(EXIT_FAILURE);
    }
    wchar_t* wstr = (wchar_t*)malloc((wlen + 1) * sizeof(wchar_t));
    if (!wstr) {
        ERROR_LOG("Memory allocation of wchar_t pointer wstr failed.");
        exit(EXIT_FAILURE);
    }
    MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, str, (int)len, wstr, wlen);
    wstr[wlen] = L'\0';
    return wstr;
}

/* Send Key and Keys for username and password input 
----------------------------------------------------
used at: loginToClient()
----------------------------------------------------
*/
static inline void sendVirtualKey(const wchar_t key) {
    INPUT input[2] = {};
    input[0].type = input[1].type = INPUT_KEYBOARD;
    input[0].ki.wVk = input[1].ki.wVk = key;
    input[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(2, input, sizeof(INPUT));
    Sleep(80);
}

static inline void sendKeys(const wchar_t* text) {
    INPUT input[2] = {};
    input[0].type = input[1].type = INPUT_KEYBOARD;
    input[0].ki.dwFlags = input[1].ki.dwFlags = KEYEVENTF_UNICODE;
    input[1].ki.dwFlags |= KEYEVENTF_KEYUP;
    for (; *text; ++text) {
        input[0].ki.wScan = input[1].ki.wScan = *text;
        SendInput(2, input, sizeof(INPUT));
        Sleep(80);
    }
}

/* Foreground Window Check and Wait 
----------------------------------------------------
used at: openLeagueClient()
----------------------------------------------------
*/

static inline bool isForegroundWindow(const wchar_t* const expectedTitle) {
    HWND hwnd = GetForegroundWindow();        // <-- Win32 API
    if (!hwnd) {
        WARN_LOG("No foreground window found.");
        return false;
    }
    wchar_t title[256] = {};
    GetWindowTextW(hwnd, title, 256);         // get real window caption
    bool ret = (wcsstr(title, expectedTitle) != NULL);
    if (ret) settings.riotClientHandle = hwnd;
    return ret;
}

// Generic Wait Untility
template <typename Func> static inline bool waitUntil(Func&& func, DWORD timeoutTimeLeft = 12000, DWORD period = 1000) {
    HANDLE hTimer = CreateWaitableTimerW(NULL, TRUE, L"GenericWaitTimer");
    if (!hTimer) {
        ERROR_LOG("CreateWaitableTimer failed (%lu)", GetLastError());
        exit(EXIT_FAILURE);
    }
    LARGE_INTEGER due;
    while (timeoutTimeLeft) {
        if (func()) {
            CloseHandle(hTimer);
            return true;
        }
        DWORD slice = (timeoutTimeLeft < period) ? timeoutTimeLeft : period;
        due.QuadPart = -(LONGLONG)(slice * 10000LL); // accepts relative time in nanoseconds
        if (!SetWaitableTimer(hTimer, &due, 0, NULL, NULL, FALSE)) {
            ERROR_LOG("SetWaitableTimer failed in function waitUntil (%lu)", GetLastError());
            CloseHandle(hTimer);
            exit(EXIT_FAILURE);
        }
        WaitForSingleObject(hTimer, INFINITE);
        timeoutTimeLeft -= slice;
    }
    CloseHandle(hTimer);
    return false;
}

static inline void waitUntilForegroundWindow(const wchar_t* const expectedTitle) {
    if(!waitUntil([expectedTitle]() -> bool { return isForegroundWindow(expectedTitle); })) {
        WARN_LOG("Timeout waiting for foreground window with title containing \"%ls\"", expectedTitle);
        exit(EXIT_FAILURE);
    }
}

static inline void loginToClient() {
    INFO_LOG("Logging into the client..");
    const wchar_t* usrname =  L"lovebitesonme";
    const wchar_t* passwd = L"uncertainty320";
    sendKeys(usrname);
    sendVirtualKey(VK_TAB);
    sendKeys(passwd);
    sendVirtualKey(VK_RETURN);
}

static inline void waitUntilReadyAndLogin() {

    if(!settings.riotClientHandle) {
        ERROR_LOG("No valid Riot Client window handle found.");
        exit(EXIT_FAILURE);
    }

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    IUIAutomation* uiAuto = NULL;
    HRESULT result = CoCreateInstance (
        __uuidof(CUIAutomation8), 
        NULL, 
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&uiAuto)
    );

    if(FAILED(result) || !uiAuto) {
        ERROR_LOG("Failed to create UIAutomation instance. Error: 0x%lX", result);
        CoUninitialize();
        exit(EXIT_FAILURE);
    }

    IUIAutomationElement* root = NULL;
    result = uiAuto->ElementFromHandle(settings.riotClientHandle, &root);
    if(FAILED(result) || !root) {
        ERROR_LOG("Failed to get root element from handle. Error: 0x%lX", result);
        uiAuto->Release();
        CoUninitialize();
        exit(EXIT_FAILURE);
    }

    IUIAutomationCondition* cond = NULL;
    VARIANT vt_autoIDname;
    VariantInit(&vt_autoIDname);
    vt_autoIDname.vt = VT_BSTR;
    vt_autoIDname.bstrVal = SysAllocString(L"username");
    result = uiAuto->CreatePropertyCondition(UIA_AutomationIdPropertyId, vt_autoIDname, &cond);
    if(FAILED(result) || !cond) {
        ERROR_LOG("Failed to create property condition. Error: 0x%lX", result);
        if (vt_autoIDname.bstrVal) SysFreeString(vt_autoIDname.bstrVal);
        vt_autoIDname.bstrVal = NULL;
        root->Release();
        root = NULL;
        uiAuto->Release();
        uiAuto = NULL;
        CoUninitialize();
        exit(EXIT_FAILURE);
    }
    IUIAutomationElement* usernameField = NULL;
    bool ok = waitUntil([&]() -> bool {
        result = root->FindFirst(TreeScope_Descendants, cond, &usernameField);
        return usernameField != NULL;    // stop loop when non-NULL
    });
    if(!ok) {
        WARN_LOG("Timeout waiting for username field to appear.");
    }
    if(FAILED(result) || !usernameField) {
        WARN_LOG("Failed to find username field. Error: 0x%lX", result);
        if (cond) cond->Release();
        cond = NULL;
        root->Release();
        root = NULL;
        uiAuto->Release();
        uiAuto = NULL;
        CoUninitialize();
        exit(EXIT_FAILURE);
    }
    if(vt_autoIDname.bstrVal) SysFreeString(vt_autoIDname.bstrVal);
    vt_autoIDname.bstrVal = NULL;
    WINBOOL off = TRUE, enabled = FALSE;
    result = usernameField->get_CurrentIsOffscreen(&off);
    if(FAILED(result) ) {
        WARN_LOG("Failed to get IsOffscreen property of username field. Error: 0x%lX", result);
    }
    result = usernameField->get_CurrentIsEnabled(&enabled);
    if(FAILED(result)) {
        WARN_LOG("Failed to get IsEnabled property of username field. Error: 0x%lX", result);
    }
    if(!off && enabled) {
        result = usernameField->SetFocus();
        if(FAILED(result)) {
            WARN_LOG("Failed to set focus to username field. Error: 0x%lX", result);
        } else {
            INFO_LOG("Riot Client username field focus is set.");
        }
        INFO_LOG("Riot Client is ready for login.");
        loginToClient();
    } else {
        ERROR_LOG("Username field is either offscreen or disabled.");
        if (cond) cond->Release();
        cond = NULL;
        if (usernameField) usernameField->Release();
        usernameField = NULL;
        if (root) root->Release();
        root = NULL;
        if (uiAuto) uiAuto->Release();
        uiAuto = NULL;
        CoUninitialize();
        exit(EXIT_FAILURE);
    }

    if (cond) cond->Release();
    cond = NULL;
    if (usernameField) usernameField->Release();
    usernameField = NULL;
    if (root) root->Release();
    root = NULL;
    if (uiAuto) uiAuto->Release();
    uiAuto = NULL;
    CoUninitialize();
}

static inline void bringWindowToForeground(HWND hwnd) {
    if (!hwnd) {
        ERROR_LOG("Invalid window handle provided to bringWindowToForeground.");
        exit(EXIT_FAILURE);
    }
    INFO_LOG("Bringing window (Handle: %p) to foreground.", hwnd);
    if (IsIconic(hwnd)) {
        ShowWindow(hwnd, SW_RESTORE);
    }
    SetForegroundWindow(hwnd);
}

static inline void bringRiotClientToForeground() {
    // find riot client handle 
    bringWindowToForeground(settings.riotClientHandle);
    waitUntilForegroundWindow(L"Riot Client");
    waitUntilReadyAndLogin();

}