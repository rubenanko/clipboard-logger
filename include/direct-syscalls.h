#ifndef DIRECT_SYSCALLS
#define DIRECT_SYSCALLS

#include <windows.h>
#include <ntdef.h>
#include <winternl.h>

typedef LONG NTSTATUS;
typedef DWORD ACCESS_MASK;
typedef ACCESS_MASK* PACCESS_MASK;

typedef NTSTATUS (NTAPI *pNtCreateThreadEx)(
    PHANDLE ThreadHandle,
    ACCESS_MASK DesiredAccess,
    PVOID ObjectAttributes,
    HANDLE ProcessHandle,
    PVOID StartRoutine,
    PVOID Argument,
    ULONG CreateFlags,
    SIZE_T ZeroBits,
    SIZE_T StackSize,
    SIZE_T MaximumStackSize,
    PVOID AttributeList
);

 typedef struct _CLIENT_ID {
   HANDLE UniqueProcess;
   HANDLE UniqueThread;
 } CLIENT_ID, *PCLIENT_ID;


NTSTATUS dCreateThreadEx(
    PHANDLE ThreadHandle, //out
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    HANDLE ProcessHandle,
    PVOID StartRoutine,
    PVOID Argument,
    ULONG CreateFlags,
    SIZE_T ZeroBits,
    SIZE_T StackSize,
    SIZE_T MaximumStackSize,
    PVOID AttributeList
);

NTSTATUS dAllocateVirtualMemory(
    HANDLE    ProcessHandle, 
    PVOID     *BaseAddress, //out
    ULONG_PTR ZeroBits,
    PSIZE_T   RegionSize, //out
    ULONG     AllocationType,
    ULONG     Protect
);

NTSTATUS dProtectVirtualMemory(
    HANDLE ProcessHandle,
    PVOID *BaseAddress, //in, out
    PSIZE_T RegionSize, //in, out
    ULONG NewProtection,
    PULONG OldProtection //out
);

NTSTATUS dReadVirtualMemory(
    HANDLE ProcessHandle,
    PVOID BaseAddress,
    PVOID Buffer, //out
    SIZE_T NumberOfBytesToRead,
    PSIZE_T NumberOfBytesRead //out
    );

NTSTATUS dWriteVirtualMemory(
    HANDLE ProcessHandle,
    PVOID BaseAddress,
    PVOID Buffer,
    SIZE_T NumberOfBytesToWrite,
    PSIZE_T NumberOfBytesWritten //out
);

NTSTATUS dOpenProcess(
    PHANDLE            ProcessHandle, //out
    ACCESS_MASK        DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PCLIENT_ID         ClientId
);

NTSTATUS dWriteFile(
    HANDLE           FileHandle,
    HANDLE           Event,
    PIO_APC_ROUTINE  ApcRoutine,
    PVOID            ApcContext,
    PIO_STATUS_BLOCK IoStatusBlock, //out
    PVOID            Buffer,
    ULONG            Length,
    PLARGE_INTEGER   ByteOffset,
    PULONG           Key
);

NTSTATUS dCreateFile(
    PHANDLE            FileHandle, //out
    ACCESS_MASK        DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK   IoStatusBlock, //out
    PLARGE_INTEGER     AllocationSize,
    ULONG              FileAttributes,
    ULONG              ShareAccess,
    ULONG              CreateDisposition,
    ULONG              CreateOptions,
    PVOID              EaBuffer,
    ULONG              EaLength
);

#endif