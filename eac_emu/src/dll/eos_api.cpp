#include <windows.h>
#include <random>
#include "../include/eac_types.h"
#include "../include/eac_common.h"
#include "../include/log.h"
#include "../include/spoof.h"

static HANDLE g_dev = INVALID_HANDLE_VALUE;
static EOS_AntiCheatClient_OnMessageToServerCallback g_msgToServer = nullptr;
static void* g_msgToServerData = nullptr;
static volatile LONG g_sessionActive = 0;

void spoofStart() {
    g_dev = CreateFileW(EAC_DEVICE_USER, GENERIC_READ | GENERIC_WRITE,
                        0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (g_dev == INVALID_HANDLE_VALUE) {
        Log::get().w("surucu cihazi yok, yalniz usermode spoof");
        return;
    }
    unsigned long v = 0;
    DWORD ret = 0;
    if (DeviceIoControl(g_dev, IOCTL_EAC_GET_VERSION, nullptr, 0,
                        &v, sizeof(v), &ret, nullptr))
        Log::get().i("surucu surumu 0x" + std::to_string(v));
}

void spoofStop() {
    if (g_dev != INVALID_HANDLE_VALUE) { CloseHandle(g_dev); g_dev = INVALID_HANDLE_VALUE; }
}

extern "C" {

__declspec(dllexport) EOS_EResult EOS_Initialize(const EOS_InitializeOptions*) {
    Log::get().i("EOS_Initialize");
    return EOS_SUCCESS;
}

__declspec(dllexport) EOS_EResult EOS_Shutdown() {
    Log::get().i("EOS_Shutdown");
    return EOS_SUCCESS;
}

__declspec(dllexport) EOS_HPlatform EOS_Platform_Create(const EOS_Platform_Options*) {
    Log::get().i("EOS_Platform_Create");
    return (EOS_HPlatform)1;
}

__declspec(dllexport) void EOS_Platform_Tick(EOS_HPlatform) {}

__declspec(dllexport) void EOS_Platform_Release(EOS_HPlatform) {}

__declspec(dllexport) EOS_HConnect EOS_Platform_GetConnectInterface(EOS_HPlatform) {
    return (EOS_HConnect)1;
}

__declspec(dllexport) EOS_HAntiCheatClient EOS_Platform_GetAntiCheatClientInterface(EOS_HPlatform) {
    Log::get().i("AntiCheatClient arayuzu istendi");
    return (EOS_HAntiCheatClient)1;
}

__declspec(dllexport) void EOS_Connect_Login(EOS_HConnect, const EOS_Connect_LoginOptions*,
                                             void* cd, EOS_Connect_OnLoginCallback cb) {
    if (cb) { EOS_Connect_LoginCallbackInfo i{}; i.ResultCode = EOS_SUCCESS; i.ClientData = cd; cb(&i); }
}

__declspec(dllexport) void EOS_Connect_CreateUser(EOS_HConnect, const EOS_Connect_LoginOptions*,
                                                  void* cd, EOS_Connect_OnCreateUserCallback cb) {
    if (cb) { EOS_Connect_CreateUserCallbackInfo i{}; i.ResultCode = EOS_SUCCESS; i.ClientData = cd; cb(&i); }
}

__declspec(dllexport) EOS_NotificationId EOS_Connect_AddNotifyAuthExpiration(
    EOS_HConnect, const void*, void* cd, EOS_Connect_OnAuthExpirationCallback cb) {
    if (cb) { EOS_Connect_AuthExpirationCallbackInfo i{}; i.ClientData = cd; cb(&i); }
    return 1;
}

__declspec(dllexport) EOS_EResult EOS_Connect_CopyIdToken(EOS_HConnect, const void*, void**) {
    return EOS_INVALID_PARAMETERS;
}

__declspec(dllexport) void EOS_Connect_IdToken_Release(void*) {}

__declspec(dllexport) EOS_EResult EOS_ProductUserId_ToString(EOS_ProductUserId id, char* out, int32_t* len) {
    if (!out || !len) return EOS_INVALID_PARAMETERS;
    sprintf_s(out, (size_t)*len, "%llu", (unsigned long long)id);
    return EOS_SUCCESS;
}

__declspec(dllexport) const char* EOS_EResult_ToString(EOS_EResult) { return "EOS_Success"; }

__declspec(dllexport) EOS_EResult EOS_Logging_SetCallback(void*) { return EOS_SUCCESS; }
__declspec(dllexport) EOS_EResult EOS_Logging_SetLogLevel(int, int) { return EOS_SUCCESS; }

__declspec(dllexport) EOS_EResult EOS_AntiCheatClient_BeginSession(
    EOS_HAntiCheatClient, const EOS_AntiCheatClient_BeginSessionOptions*) {
    InterlockedExchange(&g_sessionActive, 1);
    Log::get().i("BeginSession");
    if (g_dev != INVALID_HANDLE_VALUE) {
        EAC_SETUP s{};
        s.threadId = GetCurrentThreadId();
        s.gameId = 0;
        strcpy_s(s.processName, "FortniteClient-Win64-Shipping.exe");
        DWORD ret = 0, out = 0;
        DeviceIoControl(g_dev, IOCTL_EAC_SETUP, &s, sizeof(s), &out, sizeof(out), &ret, nullptr);
    }
    return EOS_SUCCESS;
}

__declspec(dllexport) EOS_EResult EOS_AntiCheatClient_EndSession(
    EOS_HAntiCheatClient, const EOS_AntiCheatClient_EndSessionOptions*) {
    InterlockedExchange(&g_sessionActive, 0);
    Log::get().i("EndSession");
    return EOS_SUCCESS;
}

__declspec(dllexport) EOS_EResult EOS_AntiCheatClient_ReceiveMessageFromServer(
    EOS_HAntiCheatClient, const EOS_AntiCheatClient_ReceiveMessageFromServerOptions* o) {
    Log::get().d("sunucudan mesaj " + std::to_string(o ? o->DataLengthBytes : 0) + "B");
    return EOS_SUCCESS;
}

__declspec(dllexport) EOS_NotificationId EOS_AntiCheatClient_AddNotifyMessageToServer(
    EOS_HAntiCheatClient, const void*, void* cd, EOS_AntiCheatClient_OnMessageToServerCallback cb) {
    g_msgToServer = cb;
    g_msgToServerData = cd;
    return 1;
}

__declspec(dllexport) EOS_NotificationId EOS_AntiCheatClient_AddNotifyClientIntegrityViolated(
    EOS_HAntiCheatClient, const void*, void* cd, EOS_AntiCheatClient_OnClientIntegrityViolatedCallback cb) {
    if (cb) { EOS_AntiCheatClient_OnClientIntegrityViolatedCallbackInfo i{}; i.ClientData = cd; cb(&i); }
    return 2;
}

__declspec(dllexport) EOS_NotificationId EOS_AntiCheatClient_AddNotifyPeerActionRequired(
    EOS_HAntiCheatClient, const void*, void*, void*) { return 3; }

__declspec(dllexport) EOS_NotificationId EOS_AntiCheatClient_AddNotifyConnectionEstablished(
    EOS_HAntiCheatClient, const void*, void*, void*) { return 4; }

__declspec(dllexport) EOS_NotificationId EOS_AntiCheatClient_AddNotifyConnectionRequested(
    EOS_HAntiCheatClient, const void*, void*, void*) { return 5; }

__declspec(dllexport) EOS_EResult EOS_AntiCheatClient_RegisterPeer(EOS_HAntiCheatClient, const void*) { return EOS_SUCCESS; }
__declspec(dllexport) EOS_EResult EOS_AntiCheatClient_UnregisterPeer(EOS_HAntiCheatClient, const void*) { return EOS_SUCCESS; }

__declspec(dllexport) EOS_EResult EOS_AntiCheatClient_GetProtectMessageOutputLength(
    EOS_HAntiCheatClient, const void*, uint32_t* outLen) {
    if (outLen) *outLen = 0;
    return EOS_SUCCESS;
}

__declspec(dllexport) EOS_EResult EOS_AntiCheatClient_ProtectMessage(
    EOS_HAntiCheatClient, const void*, void*, uint32_t* outLen) {
    if (outLen) *outLen = 0;
    return EOS_SUCCESS;
}

__declspec(dllexport) EOS_EResult EOS_AntiCheatClient_UnprotectMessage(
    EOS_HAntiCheatClient, const void*, void*, uint32_t* outLen) {
    if (outLen) *outLen = 0;
    return EOS_SUCCESS;
}

__declspec(dllexport) EOS_EResult EOS_AntiCheatClient_PollStatus(
    EOS_HAntiCheatClient, const void*, int* outStatus) {
    if (outStatus) *outStatus = 1;
    return EOS_SUCCESS;
}

} // extern "C"

void spoofPump() {
    if (g_msgToServer && g_sessionActive) {
        static DWORD last = 0;
        DWORD now = GetTickCount();
        if (now - last > 1000) {
            last = now;
            EOS_AntiCheatClient_OnMessageToServerCallbackInfo i{};
            i.ClientData = g_msgToServerData;
            g_msgToServer(&i);
        }
    }
}