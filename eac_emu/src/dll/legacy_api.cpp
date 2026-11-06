#include <windows.h>
#include "../include/eac_types.h"
#include "../include/log.h"

struct DummyIface { void* vtbl[32]; };
static DummyIface g_client{};
static DummyIface g_auth{};

extern "C" __declspec(dllexport) void* CreateGameClient(const char*) {
    Log::get().i("CreateGameClient");
    return &g_client;
}

extern "C" __declspec(dllexport) void* CreateClientAuth(const char*) {
    Log::get().i("CreateClientAuth");
    return &g_auth;
}