#include <ntddk.h>
#include <wdm.h>
#include "eac_common.h"
#include "xtea.h"

#define DEV_NAME  EAC_DEVICE_KERNEL
#define SYM_NAME  EAC_SYMLINK

static PDEVICE_OBJECT g_dev = nullptr;
static PKEVENT g_evtReady = nullptr;
static HANDLE  g_hReady = nullptr;
static HANDLE  g_hStop = nullptr;

static void MakeKey(eac_u32* k, void* buf, eac_u32 pid) {
    k[0] = (eac_u32)(eac_u64)buf;
    k[1] = 0;
    k[2] = pid;
    k[3] = 0;
}

static void XteaBuf(PVOID buf, ULONG len, ULONG pid, BOOLEAN enc) {
    eac_u32 key[4];
    ULONG i;
    if (!buf || len < 8) return;
    MakeKey(key, buf, pid);
    for (i = 0; i + 8 <= len; i += 8) {
        if (enc) eac_xtea_encrypt((eac_u32*)((PUCHAR)buf + i), key);
        else     eac_xtea_decrypt((eac_u32*)((PUCHAR)buf + i), key);
    }
}

DRIVER_UNLOAD Unload;

static VOID Unload(PDRIVER_OBJECT) {
    UNICODE_STRING sym;
    RtlInitUnicodeString(&sym, SYM_NAME);
    IoDeleteSymbolicLink(&sym);
    if (g_dev) { IoDeleteDevice(g_dev); g_dev = nullptr; }
    if (g_hReady) { ZwClose(g_hReady); g_hReady = nullptr; g_evtReady = nullptr; }
    if (g_hStop)  { ZwClose(g_hStop);  g_hStop = nullptr; }
    DbgPrint("[eac_emu] unloaded\n");
}

static NTSTATUS DispatchCreate(PDEVICE_OBJECT, PIRP irp) {
    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = 0;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static NTSTATUS DispatchClose(PDEVICE_OBJECT, PIRP irp) {
    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = 0;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static NTSTATUS DispatchIoctl(PDEVICE_OBJECT, PIRP irp) {
    PIO_STACK_LOCATION sp = IoGetCurrentIrpStackLocation(irp);
    ULONG code = sp->Parameters.DeviceIoControl.IoControlCode;
    ULONG inLen = sp->Parameters.DeviceIoControl.InputBufferLength;
    ULONG outLen = sp->Parameters.DeviceIoControl.OutputBufferLength;
    PVOID inBuf = sp->Parameters.DeviceIoControl.Type3InputBuffer;
    PVOID outBuf = irp->UserBuffer;
    ULONG info = 0;
    NTSTATUS st = STATUS_SUCCESS;

    if (KeGetCurrentIrql() > PASSIVE_LEVEL) {
        st = STATUS_INVALID_DEVICE_STATE;
        goto done;
    }

    __try {
        switch (code) {
        case IOCTL_EAC_GET_VERSION: {
            if (!outBuf || outLen < sizeof(ULONG)) { st = STATUS_BUFFER_TOO_SMALL; break; }
            ProbeForWrite(outBuf, sizeof(ULONG), 1);
            *(ULONG*)outBuf = EAC_DRIVER_VERSION;
            info = sizeof(ULONG);
            DbgPrint("[eac_emu] version -> 0x%x\n", EAC_DRIVER_VERSION);
            break;
        }
        case IOCTL_EAC_SETUP: {
            EAC_SETUP* s;
            if (!inBuf || inLen < sizeof(EAC_SETUP)) { st = STATUS_BUFFER_TOO_SMALL; break; }
            ProbeForRead(inBuf, sizeof(EAC_SETUP), 1);
            s = (EAC_SETUP*)inBuf;
            DbgPrint("[eac_emu] setup game=%u pid=%u proc=%s size=%u\n",
                     s->gameId, (ULONG)(ULONG_PTR)PsGetCurrentProcessId(),
                     s->processName, s->fileDataSize);

            if (s->fileDataSize && inLen >= sizeof(EAC_SETUP) + s->fileDataSize) {
                PVOID data = (PUCHAR)inBuf + sizeof(EAC_SETUP);
                ProbeForRead(data, s->fileDataSize, 1);
                XteaBuf(data, s->fileDataSize,
                        (ULONG)(ULONG_PTR)PsGetCurrentProcessId(), FALSE);
                DbgPrint("[eac_emu] module decrypted %u bytes\n", s->fileDataSize);
            }

            if (outBuf && outLen >= sizeof(ULONG)) {
                ProbeForWrite(outBuf, sizeof(ULONG), 1);
                *(ULONG*)outBuf = 0;
                info = sizeof(ULONG);
            }
            if (g_evtReady) KeSetEvent(g_evtReady, IO_NO_INCREMENT, FALSE);
            break;
        }
        case IOCTL_EAC_CHECK_NTDLL: {
            if (outBuf && outLen >= sizeof(ULONG)) {
                ProbeForWrite(outBuf, sizeof(ULONG), 1);
                *(ULONG*)outBuf = 0;
                info = sizeof(ULONG);
            }
            break;
        }
        default:
            DbgPrint("[eac_emu] unknown ioctl 0x%x\n", code);
            st = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        st = GetExceptionCode();
    }

done:
    irp->IoStatus.Status = st;
    irp->IoStatus.Information = info;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return st;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT drv, PUNICODE_STRING) {
    UNICODE_STRING devName, symName, evtName;
    PDEVICE_OBJECT dev = nullptr;
    NTSTATUS st;

    drv->DriverUnload = Unload;
    drv->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;
    drv->MajorFunction[IRP_MJ_CLOSE] = DispatchClose;
    drv->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIoctl;

    RtlInitUnicodeString(&devName, DEV_NAME);
    st = IoCreateDevice(drv, 0, &devName, FILE_DEVICE_UNKNOWN,
                        FILE_DEVICE_SECURE_OPEN, FALSE, &dev);
    if (!NT_SUCCESS(st)) return st;
    g_dev = dev;

    RtlInitUnicodeString(&symName, SYM_NAME);
    st = IoCreateSymbolicLink(&symName, &devName);
    if (!NT_SUCCESS(st)) {
        IoDeleteDevice(dev);
        g_dev = nullptr;
        return st;
    }

    RtlInitUnicodeString(&evtName, L"\\BaseNamedObjects\\EasyAntiCheat_EOS_Ready");
    g_evtReady = IoCreateSynchronizationEvent(&evtName, &g_hReady);
    if (!g_evtReady) DbgPrint("[eac_emu] ready event yok\n");

    RtlInitUnicodeString(&evtName, L"\\BaseNamedObjects\\EasyAntiCheat_EOS_Stop");
    if (!IoCreateSynchronizationEvent(&evtName, &g_hStop))
        DbgPrint("[eac_emu] stop event yok\n");

    DbgPrint("[eac_emu] loaded, version 0x%x\n", EAC_DRIVER_VERSION);
    return STATUS_SUCCESS;
}