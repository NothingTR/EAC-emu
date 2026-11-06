#pragma once
#include <cstdint>

typedef int32_t  EOS_EResult;
typedef void*    EOS_HPlatform;
typedef void*    EOS_HConnect;
typedef void*    EOS_HAntiCheatClient;
typedef uint64_t EOS_NotificationId;
typedef uint64_t EOS_ProductUserId;
typedef void*    EOS_ClientData;

#define EOS_SUCCESS 0
#define EOS_INVALID_PARAMETERS 3
#define EOS_UNEXPECTED_ERROR 2

struct EOS_InitializeOptions { const char* ProductName; const char* ProductVersion; };
struct EOS_Platform_Options { const char* ProductId; const char* SandboxId; };
struct EOS_Connect_LoginOptions { const void* Credentials; };
struct EOS_AntiCheatClient_BeginSessionOptions { int Mode; };
struct EOS_AntiCheatClient_EndSessionOptions { int Reason; };
struct EOS_AntiCheatClient_ReceiveMessageFromServerOptions { uint32_t DataLengthBytes; const void* Data; };

struct EOS_Connect_LoginCallbackInfo {
    EOS_EResult ResultCode; EOS_ClientData ClientData; EOS_ProductUserId LocalUserId;
};
struct EOS_Connect_CreateUserCallbackInfo {
    EOS_EResult ResultCode; EOS_ClientData ClientData; EOS_ProductUserId LocalUserId;
};
struct EOS_Connect_AuthExpirationCallbackInfo {
    EOS_ClientData ClientData; EOS_ProductUserId LocalUserId;
};
struct EOS_AntiCheatClient_OnMessageToServerCallbackInfo {
    EOS_ClientData ClientData; uint32_t MessageLengthBytes; const void* MessageData;
};
struct EOS_AntiCheatClient_OnClientIntegrityViolatedCallbackInfo {
    int ViolationType; EOS_ClientData ClientData; const char* ViolationMessage;
};
struct EOS_AntiCheatClient_OnPeerActionRequiredCallbackInfo {
    EOS_ClientData ClientData; uint64_t PeerHandle; int PeerAction;
};

typedef void (*EOS_Connect_OnLoginCallback)(const EOS_Connect_LoginCallbackInfo*);
typedef void (*EOS_Connect_OnCreateUserCallback)(const EOS_Connect_CreateUserCallbackInfo*);
typedef void (*EOS_Connect_OnAuthExpirationCallback)(const EOS_Connect_AuthExpirationCallbackInfo*);
typedef void (*EOS_AntiCheatClient_OnMessageToServerCallback)(const EOS_AntiCheatClient_OnMessageToServerCallbackInfo*);
typedef void (*EOS_AntiCheatClient_OnClientIntegrityViolatedCallback)(const EOS_AntiCheatClient_OnClientIntegrityViolatedCallbackInfo*);