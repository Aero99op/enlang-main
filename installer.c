/*
 * Enlangg Sovereign Setup Launcher (setup.exe)
 * Launches the modern WPF GUI installer seamlessly without console flash.
 */

#include <windows.h>
#include <stdio.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    char currentDir[MAX_PATH];
    GetModuleFileNameA(NULL, currentDir, MAX_PATH);
    char* lastSlash = strrchr(currentDir, '\\');
    if (lastSlash) *lastSlash = '\0';

    char psScript[MAX_PATH + 32];
    snprintf(psScript, sizeof(psScript), "%s\\install.ps1", currentDir);

    char cmd[MAX_PATH * 2 + 128];
    snprintf(cmd, sizeof(cmd), "powershell.exe -ExecutionPolicy Bypass -File \"%s\"", psScript);

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));

    if (CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, currentDir, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return 0;
    }

    return 1;
}
