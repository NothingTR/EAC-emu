#include "../include/driver_ctl.h"
#include "../include/eac_common.h"
#include "../include/log.h"

static bool ctl(const std::wstring& name, DWORD code) {
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    if (!scm) return false;
    SC_HANDLE s = OpenServiceW(scm, name.c_str(), SERVICE_ALL_ACCESS);
    if (!s) { CloseServiceHandle(scm); return false; }
    SERVICE_STATUS st{};
    BOOL ok = ControlService(s, code, &st);
    CloseServiceHandle(s);
    CloseServiceHandle(scm);
    return ok != FALSE;
}

bool drvInstall(const std::wstring& sysPath, const std::wstring& name) {
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    if (!scm) { Log::get().e("SCM acilamadi (yonetici?)"); return false; }
    SC_HANDLE s = CreateServiceW(scm, name.c_str(), name.c_str(),
        SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START,
        SERVICE_ERROR_NORMAL, sysPath.c_str(), nullptr, nullptr, nullptr,
        nullptr, nullptr);
    if (!s) {
        DWORD e = GetLastError();
        if (e == ERROR_SERVICE_EXISTS) {
            Log::get().w("servis zaten var, guncelleniyor");
            s = OpenServiceW(scm, name.c_str(), SERVICE_ALL_ACCESS);
            if (s) ChangeServiceConfigW(s, SERVICE_NO_CHANGE, SERVICE_DEMAND_START,
                SERVICE_NO_CHANGE, sysPath.c_str(), nullptr, nullptr, nullptr,
                nullptr, nullptr, nullptr);
        }
    }
    bool ok = s != nullptr;
    if (s) CloseServiceHandle(s);
    CloseServiceHandle(scm);
    return ok;
}

bool drvStart(const std::wstring& name) {
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    if (!scm) return false;
    SC_HANDLE s = OpenServiceW(scm, name.c_str(), SERVICE_ALL_ACCESS);
    bool ok = false;
    if (s) {
        ok = StartServiceW(s, 0, nullptr) != FALSE;
        if (!ok && GetLastError() == ERROR_SERVICE_ALREADY_RUNNING) ok = true;
        CloseServiceHandle(s);
    }
    CloseServiceHandle(scm);
    return ok;
}

bool drvStop(const std::wstring& name)  { return ctl(name, SERVICE_CONTROL_STOP); }
bool drvDelete(const std::wstring& name) {
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    if (!scm) return false;
    SC_HANDLE s = OpenServiceW(scm, name.c_str(), DELETE);
    bool ok = false;
    if (s) { ok = DeleteService(s) != FALSE; CloseServiceHandle(s); }
    CloseServiceHandle(scm);
    return ok;
}

bool drvRunning(const std::wstring& name) {
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) return false;
    SC_HANDLE s = OpenServiceW(scm, name.c_str(), SERVICE_QUERY_STATUS);
    bool r = false;
    if (s) {
        SERVICE_STATUS st{};
        if (QueryServiceStatus(s, &st)) r = st.dwCurrentState == SERVICE_RUNNING;
        CloseServiceHandle(s);
    }
    CloseServiceHandle(scm);
    return r;
}

HANDLE devOpen() {
    return CreateFileW(EAC_DEVICE_USER, GENERIC_READ | GENERIC_WRITE,
                       0, nullptr, OPEN_EXISTING, 0, nullptr);
}

bool devGetVersion(HANDLE h, unsigned long* ver) {
    DWORD out = 0;
    DWORD ret = 0;
    BOOL ok = DeviceIoControl(h, IOCTL_EAC_GET_VERSION, nullptr, 0,
                              &out, sizeof(out), &ret, nullptr);
    if (ok && ver) *ver = out;
    return ok != FALSE;
}

bool devSetup(HANDLE h, unsigned long* status) {
    EAC_SETUP s{};
    s.gameId = 0;
    strcpy_s(s.processName, "FortniteClient-Win64-Shipping.exe");
    s.threadId = GetCurrentThreadId();
    DWORD out = 0, ret = 0;
    BOOL ok = DeviceIoControl(h, IOCTL_EAC_SETUP, &s, sizeof(s),
                              &out, sizeof(out), &ret, nullptr);
    if (status) *status = out;
    return ok != FALSE;
}

bool devCheckNtdll(HANDLE h, unsigned long* status) {
    DWORD out = 0, ret = 0;
    BOOL ok = DeviceIoControl(h, IOCTL_EAC_CHECK_NTDLL, nullptr, 0,
                              &out, sizeof(out), &ret, nullptr);
    if (status) *status = out;
    return ok != FALSE;
}