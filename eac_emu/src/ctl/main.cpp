#include <windows.h>
#include <shellapi.h>
#include <TlHelp32.h>
#include <conio.h>
#include <cstdio>
#include <string>
#include <vector>

#include "../include/eac_common.h"
#include "../include/driver_ctl.h"
#include "../include/log.h"

static std::wstring g_driver = L"eac_emu";
static std::wstring g_gameDir;
static std::wstring g_binDir;
static std::wstring g_dllName = L"EasyAntiCheat_EOS.dll";

static bool FileExistsW(const wchar_t* p) {
    return GetFileAttributesW(p) != INVALID_FILE_ATTRIBUTES;
}

static std::string narrow(const std::wstring& w) {
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string o((size_t)n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), o.data(), n, nullptr, nullptr);
    return o;
}

static std::wstring exeDir() {
    wchar_t b[MAX_PATH]{};
    GetModuleFileNameW(nullptr, b, MAX_PATH);
    std::wstring p = b;
    size_t s = p.find_last_of(L'\\');
    return s == std::wstring::npos ? L"." : p.substr(0, s);
}

static void findDirs() {
    const wchar_t* roots[] = {
        L"C:\\Program Files\\Epic Games\\Fortnite",
        L"D:\\Epic Games\\Fortnite",
        L"E:\\Epic Games\\Fortnite",
        L"C:\\Program Files\\Epic Games\\FortniteGame",
        L"D:\\Fortnite"
    };
    for (auto r : roots) {
        std::wstring bin = std::wstring(r) + L"\\FortniteGame\\Binaries\\Win64";
        if (FileExistsW(bin.c_str())) {
            g_binDir = bin;
            g_gameDir = bin + L"\\EasyAntiCheat";
            return;
        }
    }
}

static void listDir(const std::wstring& dir) {
    WIN32_FIND_DATAW fd{};
    HANDLE h = FindFirstFileW((dir + L"\\*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) { std::printf("  (klasor okunamadi)\n"); return; }
    do {
        if (wcscmp(fd.cFileName, L".") && wcscmp(fd.cFileName, L".."))
            std::printf("  - %s\n", narrow(fd.cFileName).c_str());
    } while (FindNextFileW(h, &fd));
    FindClose(h);
}

static bool resolveDll(std::wstring& dst) {
    if (g_gameDir.empty()) findDirs();

    if (!g_gameDir.empty()) {
        std::wstring direct = g_gameDir + L"\\" + g_dllName;
        if (FileExistsW(direct.c_str())) { dst = direct; return true; }

        std::wstring binDll = g_binDir + L"\\" + g_dllName;
        if (FileExistsW(binDll.c_str())) { dst = binDll; return true; }

        dst = direct;
        return true;
    }
    return false;
}

static void showDirs() {
    std::printf("\n  bin : %s\n", g_binDir.empty() ? "bulunamadi" : narrow(g_binDir).c_str());
    std::printf("  eac : %s\n", g_gameDir.empty() ? "bulunamadi" : narrow(g_gameDir).c_str());
    if (!g_gameDir.empty() && FileExistsW(g_gameDir.c_str())) {
        std::printf("  icerik:\n");
        listDir(g_gameDir);
    }
}

static void installDll() {
    std::wstring dst;
    if (!resolveDll(dst)) {
        std::printf("  oyun bulunamadi, Win64 yolunu yapistir: ");
        wchar_t buf[MAX_PATH]{};
        if (!fgetws(buf, MAX_PATH, stdin)) return;
        std::wstring s = buf;
        while (!s.empty() && (s.back() == L'\n' || s.back() == L'\r')) s.pop_back();
        g_binDir = s;
        g_gameDir = s + L"\\EasyAntiCheat";
        if (!resolveDll(dst)) return;
    }

    std::wstring src = exeDir() + L"\\" + g_dllName;
    if (!FileExistsW(src.c_str())) { std::printf("  kaynak dll yok: %s\n", narrow(src).c_str()); return; }

    std::wstring dir = dst.substr(0, dst.find_last_of(L'\\'));
    CreateDirectoryW(dir.c_str(), nullptr);

    std::wstring bak = dst + L".eacbak";
    if (FileExistsW(dst.c_str()) && !FileExistsW(bak.c_str())) {
        if (!CopyFileW(dst.c_str(), bak.c_str(), FALSE)) {
            std::printf("  yedekleme basarisiz (%lu)\n", GetLastError());
            return;
        }
        std::printf("  yedek: %s\n", narrow(bak).c_str());
    }

    if (CopyFileW(src.c_str(), dst.c_str(), FALSE))
        std::printf("  kuruldu: %s\n", narrow(dst).c_str());
    else
        std::printf("  kopyalama basarisiz (%lu) - oyun kapali mi?\n", GetLastError());
}

static void restoreDll() {
    std::wstring dst;
    if (!resolveDll(dst)) return;
    std::wstring bak = dst + L".eacbak";
    if (!FileExistsW(bak.c_str())) { std::printf("  yedek yok\n"); return; }
    if (CopyFileW(bak.c_str(), dst.c_str(), FALSE)) std::printf("  geri alindi\n");
    else std::printf("  geri alma basarisiz (%lu)\n", GetLastError());
}

static void deviceTest() {
    HANDLE h = devOpen();
    if (h == INVALID_HANDLE_VALUE) { std::printf("  cihaz yok (%lu)\n", GetLastError()); return; }
    unsigned long v = 0, st = 0;
    if (devGetVersion(h, &v)) std::printf("  0x226003 -> surum 0x%lX\n", v);
    else std::printf("  0x226003 -> hata %lu\n", GetLastError());
    if (devSetup(h, &st)) std::printf("  0x22E007 -> durum %lu\n", st);
    else std::printf("  0x22E007 -> hata %lu\n", GetLastError());
    if (devCheckNtdll(h, &st)) std::printf("  0x22E01F -> durum %lu\n", st);
    else std::printf("  0x22E01F -> hata %lu\n", GetLastError());
    CloseHandle(h);
}

static void launchGame() {
    if (g_binDir.empty()) findDirs();
    if (g_binDir.empty()) { std::printf("  oyun yolu yok, 4 ile klasoru belirle\n"); return; }

    std::wstring exe = g_binDir + L"\\FortniteClient-Win64-Shipping.exe";
    if (!FileExistsW(exe.c_str())) { std::printf("  %s yok\n", narrow(exe).c_str()); return; }

    std::wstring args;
    args += L"-epicapp=Fortnite -epicenv=Prod -EpicPortal -noeac -nosplash ";
    args += L"-epiclocale=tr -epicovt=tr";

    std::printf("  baslatiliyor: %s\n", narrow(exe).c_str());
    HINSTANCE r = ShellExecuteW(nullptr, L"open", exe.c_str(), args.c_str(),
                                g_binDir.c_str(), SW_SHOWNORMAL);
    if ((INT_PTR)r <= 32) std::printf("  baslatma hatasi %lld\n", (long long)(INT_PTR)r);
}

int main() {
    Log::get().setFile(narrow(exeDir() + L"\\eac_emu.log"));
    SetConsoleTitleW(L"EasyAntiCheat Emulator");
    findDirs();

    while (true) {
        std::printf("\n=================================================\n");
        std::printf(" EasyAntiCheat Emulator (EOS)\n");
        std::printf(" surucu: %s\n", drvRunning(g_driver) ? "RUNNING" : "STOP");
        std::printf(" oyun  : %s\n", g_binDir.empty() ? "bulunamadi" : "bulundu");
        std::printf("=================================================\n");
        std::printf(" 1) Surucuyu kur ve baslat\n");
        std::printf(" 2) Surucuyu durdur ve kaldir\n");
        std::printf(" 3) Durum + klasor icerigi\n");
        std::printf(" 4) DLL kur (yedekler)\n");
        std::printf(" 5) DLL geri al\n");
        std::printf(" 6) Cihaz testi (IOCTL)\n");
        std::printf(" 7) Oyunu -noeac ile baslat\n");
        std::printf(" 0) Cikis\n");
        std::printf(" secim> ");
        int c = _getch();
        std::printf("%c\n", c);

        switch (c) {
            case '1': {
                std::wstring sys = exeDir() + L"\\eac_emu.sys";
                if (!FileExistsW(sys.c_str())) { std::printf("  eac_emu.sys yok\n"); break; }
                if (!drvInstall(sys, g_driver)) { std::printf("  kurulum hatasi %lu\n", GetLastError()); break; }
                if (drvStart(g_driver)) std::printf("  surucu calisiyor\n");
                else std::printf("  baslatma hatasi %lu\n", GetLastError());
                break;
            }
            case '2':
                drvStop(g_driver);
                Sleep(500);
                if (drvDelete(g_driver)) std::printf("  kaldirildi\n");
                else std::printf("  kaldirma hatasi %lu\n", GetLastError());
                break;
            case '3': {
                HANDLE h = devOpen();
                std::printf("  surucu: %s\n", drvRunning(g_driver) ? "RUNNING" : "STOP");
                std::printf("  cihaz : %s\n", h != INVALID_HANDLE_VALUE ? "acilabiliyor" : "YOK");
                if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
                showDirs();
                break;
            }
            case '4': installDll(); break;
            case '5': restoreDll(); break;
            case '6': deviceTest(); break;
            case '7': launchGame(); break;
            case '0': return 0;
        }
    }
}