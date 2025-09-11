#pragma once
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