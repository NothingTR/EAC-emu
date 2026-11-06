#pragma once

#ifdef _KERNEL_MODE
#include <ntddk.h>
typedef ULONG eac_u32;
typedef ULONG64 eac_u64;
#else
#include <windows.h>
#include <stdint.h>
typedef uint32_t eac_u32;
typedef uint64_t eac_u64;
#endif

#define EAC_DEVICE_USER   L"\\\\.\\EasyAntiCheat"
#define EAC_DEVICE_KERNEL L"\\Device\\EasyAntiCheat"
#define EAC_SYMLINK       L"\\DosDevices\\EasyAntiCheat"

#define EAC_DRIVER_VERSION 0x35

/* CTL_CODE(0x22, 0x800, METHOD_NEITHER, FILE_READ_DATA) */
#define IOCTL_EAC_GET_VERSION 0x226003
/* CTL_CODE(0x22, 0x801, METHOD_NEITHER, FILE_READ_DATA|FILE_WRITE_DATA) */
#define IOCTL_EAC_SETUP       0x22E007
/* CTL_CODE(0x22, 0x807, METHOD_NEITHER, FILE_READ_DATA|FILE_WRITE_DATA) */
#define IOCTL_EAC_CHECK_NTDLL 0x22E01F

#pragma pack(push, 1)
typedef struct _EAC_SETUP {
    unsigned char pad0[0x8];
    eac_u32 syncEvent;
    eac_u32 umCallback;
    unsigned char pad1[0xC];
    eac_u32 threadId;
    eac_u32 gameId;
    char  processName[256];
    eac_u32 fileDataSize;
    /* char fileData[]; */
} EAC_SETUP;
#pragma pack(pop)

#define EAC_SETUP_FIXED sizeof(EAC_SETUP)