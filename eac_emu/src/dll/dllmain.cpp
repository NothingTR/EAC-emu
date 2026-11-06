#include <windows.h>
#include <string>
#include "../include/log.h"
#include "../include/spoof.h"

HMODULE g_self = nullptr;

static std::string narrow(const std::wstring& w) {
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(),
                                nullptr, 0, nullptr, nullptr);
    std::string o((size_t)n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(),
                        o.data(), n, nullptr, nullptr);
    return o;
}

static std::string logPath() {
    wchar_t b[MAX_PATH]{};
    GetModuleFileNameW(nullptr, b, MAX_PATH);
    std::wstring p = b;
    size_t s = p.find_last_of(L'\\');
    p = (s == std::wstring::npos) ? L"." : p.substr(0, s);
    p += L"\\eac_emu_dll.log";
    return narrow(p);
}

BOOL APIENTRY DllMain(HMODULE h, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_self = h;
        DisableThreadLibraryCalls(h);
        Log::get().setFile(logPath());
        Log::get().i("eac_emu dll yuklendi");
        spoofStart();
    } else if (reason == DLL_PROCESS_DETACH) {
        spoofStop();
        Log::get().i("eac_emu dll kaldirildi");
    }
    return TRUE;
}