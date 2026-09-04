import os
import subprocess

def main():
    print("Building Standalone Enlangg Windows Installer...")
    
    with open("enlangg.exe", "rb") as f:
        enlangg_bytes = f.read()
    with open("enlng.exe", "rb") as f:
        enlng_bytes = f.read()
        
    print(f"enlangg.exe size: {len(enlangg_bytes)} bytes")
    print(f"enlng.exe size: {len(enlng_bytes)} bytes")

    c_code = []
    c_code.append("""#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Embedded binaries
""")

    # Format bytes in chunks
    c_code.append(f"static const unsigned int enlangg_len = {len(enlangg_bytes)};\n")
    c_code.append("static const unsigned char enlangg_bin[] = {\n")
    for i in range(0, len(enlangg_bytes), 32):
        chunk = enlangg_bytes[i:i+32]
        c_code.append("    " + ", ".join(str(b) for b in chunk) + ",\n")
    c_code.append("};\n\n")

    c_code.append(f"static const unsigned int enlng_len = {len(enlng_bytes)};\n")
    c_code.append("static const unsigned char enlng_bin[] = {\n")
    for i in range(0, len(enlng_bytes), 32):
        chunk = enlng_bytes[i:i+32]
        c_code.append("    " + ", ".join(str(b) for b in chunk) + ",\n")
    c_code.append("};\n\n")

    c_code.append(r"""
static int write_file(const char *path, const unsigned char *data, unsigned int len) {
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    size_t written = fwrite(data, 1, len, f);
    fclose(f);
    return written == len;
}

static void add_to_path(const char *dir) {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        char current_path[32768] = {0};
        DWORD size = sizeof(current_path) - 1;
        DWORD type = REG_EXPAND_SZ;
        
        LONG res = RegQueryValueExA(hKey, "Path", NULL, &type, (LPBYTE)current_path, &size);
        if (res == ERROR_SUCCESS) {
            // Check if already in PATH
            if (strstr(current_path, dir) == NULL) {
                char new_path[32768] = {0};
                if (strlen(current_path) > 0 && current_path[strlen(current_path) - 1] != ';') {
                    snprintf(new_path, sizeof(new_path), "%s;%s", current_path, dir);
                } else {
                    snprintf(new_path, sizeof(new_path), "%s%s", current_path, dir);
                }
                RegSetValueExA(hKey, "Path", 0, type, (const BYTE *)new_path, (DWORD)strlen(new_path) + 1);
            }
        } else {
            RegSetValueExA(hKey, "Path", 0, REG_EXPAND_SZ, (const BYTE *)dir, (DWORD)strlen(dir) + 1);
        }
        RegCloseKey(hKey);

        // Notify Windows of environment change (non-blocking)
        SendNotifyMessageA(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)"Environment");
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char *cmd_line = GetCommandLineA();
    int is_silent = (strstr(cmd_line, "/quiet") != NULL || strstr(cmd_line, "-s") != NULL || strstr(cmd_line, "/silent") != NULL);

    const char *userprofile = getenv("USERPROFILE");
    if (!userprofile) userprofile = "C:\\";

    char install_dir[MAX_PATH];
    snprintf(install_dir, sizeof(install_dir), "%s\\.enlangg\\bin", userprofile);

    // Create directories
    char base_dir[MAX_PATH];
    snprintf(base_dir, sizeof(base_dir), "%s\\.enlangg", userprofile);
    CreateDirectoryA(base_dir, NULL);
    CreateDirectoryA(install_dir, NULL);

    char enlangg_path[MAX_PATH];
    char enlng_path[MAX_PATH];
    snprintf(enlangg_path, sizeof(enlangg_path), "%s\\enlangg.exe", install_dir);
    snprintf(enlng_path, sizeof(enlng_path), "%s\\enlng.exe", install_dir);

    // Extract files
    if (!write_file(enlangg_path, enlangg_bin, enlangg_len) ||
        !write_file(enlng_path, enlng_bin, enlng_len)) {
        if (!is_silent) {
            MessageBoxA(NULL,
                "Failed to write executables. Please verify directory permissions.",
                "Enlangg Setup Error",
                MB_ICONERROR | MB_OK);
        }
        return 1;
    }

    // Configure PATH
    add_to_path(install_dir);

    if (is_silent) {
        return 0;
    }

    char msg[1024];
    snprintf(msg, sizeof(msg),
        "Enlangg Sovereign Toolchain has been successfully installed!\n\n"
        "Installation Directory:\n  %s\n\n"
        "PATH environment variable has been configured automatically.\n\n"
        "Commands installed:\n"
        "  - enlangg.exe (General Toolchain)\n"
        "  - enlng.exe   (Compiler & Runtime)\n\n"
        "Would you like to open Command Prompt now to try it out?",
        install_dir);

    int res = MessageBoxA(NULL, msg, "Enlangg Setup — Installation Complete", MB_YESNO | MB_ICONINFORMATION);
    if (res == IDYES) {
        char cmd[MAX_PATH + 100];
        snprintf(cmd, sizeof(cmd), "cmd.exe /k \"title Enlangg Sovereign Terminal && cd /d %s && enlng --help\"", userprofile);
        WinExec(cmd, SW_SHOW);
    }

    return 0;
}
""")

    src_file = "scratch_setup.c"
    with open(src_file, "w", encoding="utf-8") as f:
        f.write("".join(c_code))

    manifest_xml = """<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">
  <assemblyIdentity version="5.0.0.0" processorArchitecture="*" name="Enlangg.Setup" type="win32"/>
  <description>Enlangg Sovereign Toolchain Installer</description>
  <trustInfo xmlns="urn:schemas-microsoft-com:asm.v3">
    <security>
      <requestedPrivileges>
        <requestedExecutionLevel level="asInvoker" uiAccess="false"/>
      </requestedPrivileges>
    </security>
  </trustInfo>
</assembly>"""

    with open("scratch_app.manifest", "w", encoding="utf-8") as f:
        f.write(manifest_xml)

    with open("scratch_res.rc", "w", encoding="utf-8") as f:
        f.write('1 24 "scratch_app.manifest"\n')

    subprocess.run(["windres", "scratch_res.rc", "-O", "coff", "-o", "scratch_res.res"], check=True)
        
    print(f"Compiling {src_file} with MinGW gcc and manifest...")
    cmd = ["gcc", "-O2", "-mwindows", "-static", src_file, "scratch_res.res", "-o", "website/enlangg-setup.exe"]
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print("Compile error:", res.stderr)
        return False
        
    # Also copy as setup.exe for convenience
    import shutil
    shutil.copy("website/enlangg-setup.exe", "website/setup.exe")
    shutil.copy("website/enlangg-setup.exe", "setup.exe")
    
    # Clean up scratch files
    for clean in [src_file, "scratch_app.manifest", "scratch_res.rc", "scratch_res.res"]:
        if os.path.exists(clean):
            os.remove(clean)

    exe_size = os.path.getsize("website/enlangg-setup.exe")
    print(f"Success! Generated website/enlangg-setup.exe and setup.exe ({exe_size} bytes)")
    return True

if __name__ == "__main__":
    main()
