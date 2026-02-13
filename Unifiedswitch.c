#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>

// Global settings
static int multiMonitor = 0;
static int hotkeyMode = 2; // Default: Ctrl+Alt+1/2/3
static int autoStart = 0;
static char monitorTool[MAX_PATH] = {0};
static char workingDir[MAX_PATH] = {0};

// Load configuration from config.ini
static void LoadConfig() {
    GetCurrentDirectoryA(MAX_PATH, workingDir);
    
    char iniPath[MAX_PATH];
    snprintf(iniPath, MAX_PATH, "%s\\config.ini", workingDir);
    
    multiMonitor = GetPrivateProfileIntA("SETTINGS", "multiMonitor", 0, iniPath);
    hotkeyMode = GetPrivateProfileIntA("SETTINGS", "hotkeyMode", 2, iniPath);
    autoStart = GetPrivateProfileIntA("SETTINGS", "autoStart", 0, iniPath);
    GetPrivateProfileStringA("PATHS", "clickmon", "", monitorTool, MAX_PATH, iniPath);
    
    // If monitor tool path is relative, make it absolute
    if(monitorTool[0] && monitorTool[1] != ':') {
        char temp[MAX_PATH];
        snprintf(temp, MAX_PATH, "%s\\%s", workingDir, monitorTool);
        strcpy(monitorTool, temp);
    }
}

// Switch monitor input
static void SwitchMonitor(int channel) {
    if(multiMonitor || !monitorTool[0]) return;
    
    char iniPath[MAX_PATH];
    snprintf(iniPath, MAX_PATH, "%s\\config.ini", workingDir);
    
    char key[32];
    snprintf(key, 32, "device%d", channel);
    
    char input[16] = {0};
    GetPrivateProfileStringA("SOURCES", key, "", input, 16, iniPath);
    
    if(input[0]) {
        char cmd[512];
        snprintf(cmd, 512, "\"%s\" /SetValue Primary 60 %s", monitorTool, input);
        
        STARTUPINFOA si = {sizeof(si)};
        PROCESS_INFORMATION pi = {0};
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        
        CreateProcessA(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
        if(pi.hProcess) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }
}

// Switch devices (keyboard/mouse)
static void SwitchDevices(int channel) {
    char cmd[MAX_PATH];
    snprintf(cmd, MAX_PATH, "%s\\LogiSwitch.exe %d", workingDir, channel);
    
    STARTUPINFOA si = {sizeof(si)};
    PROCESS_INFORMATION pi = {0};
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    
    if(CreateProcessA(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        // Wait for LogiSwitch to complete
        WaitForSingleObject(pi.hProcess, 2000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        
        // Wake mouse cursor
        Sleep(40);
        mouse_event(MOUSEEVENTF_MOVE, 1, 0, 0, 0);
        mouse_event(MOUSEEVENTF_MOVE, -1, 0, 0, 0);
    }
}

// Handle hotkey press
static void HandleHotkey(int channel) {
    // Switch monitor first (if enabled)
    SwitchMonitor(channel);
    
    // Small delay between monitor and devices
    Sleep(50);
    
    // Switch devices
    SwitchDevices(channel);
}

// Global hook handle
static HHOOK keyboardHook = NULL;
static HWND mainWindow = NULL;

// Custom messages for our hotkeys
#define WM_HOTKEY_CUSTOM (WM_USER + 1)

// Low-level keyboard hook procedure
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if(nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* kbd = (KBDLLHOOKSTRUCT*)lParam;
        
        // Only process key down events
        if(wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            BOOL winKey = (GetAsyncKeyState(VK_LWIN) & 0x8000) || (GetAsyncKeyState(VK_RWIN) & 0x8000);
            BOOL ctrlKey = (GetAsyncKeyState(VK_CONTROL) & 0x8000);
            BOOL altKey = (GetAsyncKeyState(VK_MENU) & 0x8000);
            BOOL shiftKey = (GetAsyncKeyState(VK_SHIFT) & 0x8000);
            
            // Check which mode is active
            BOOL match = FALSE;
            if(hotkeyMode == 1 && winKey && !ctrlKey && !altKey) {
                match = TRUE; // Win+1/2/3
            } else if(hotkeyMode == 2 && ctrlKey && altKey && !winKey) {
                match = TRUE; // Ctrl+Alt+1/2/3
            } else if(hotkeyMode == 3 && ctrlKey && shiftKey && !winKey && !altKey) {
                match = TRUE; // Ctrl+Shift+1/2/3
            }
            
            if(match) {
                if(kbd->vkCode == '1') {
                    PostMessage(mainWindow, WM_HOTKEY_CUSTOM, 1, 0);
                    return 1;
                } else if(kbd->vkCode == '2') {
                    PostMessage(mainWindow, WM_HOTKEY_CUSTOM, 2, 0);
                    return 1;
                } else if(kbd->vkCode == '3') {
                    PostMessage(mainWindow, WM_HOTKEY_CUSTOM, 3, 0);
                    return 1;
                }
            }
        }
    }
    
    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

// Window procedure for message handling
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_HOTKEY_CUSTOM:
            HandleHotkey((int)wParam);
            return 0;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// Check if already running
static BOOL IsAlreadyRunning() {
    HANDLE mutex = CreateMutexA(NULL, FALSE, "UnifiedSwitch_SingleInstance");
    if(GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(mutex);
        return TRUE;
    }
    return FALSE;
}

// Add to startup (only if not already present)
static void AddToStartup() {
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    
    HKEY hKey;
    if(RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
                     0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        
        char existingPath[MAX_PATH] = {0};
        DWORD size = MAX_PATH;
        DWORD type = REG_SZ;
        
        LONG result = RegQueryValueExA(hKey, "UnifiedSwitch", NULL, &type, (BYTE*)existingPath, &size);
        
        // Check if already exists
        if(result != ERROR_SUCCESS || strcmp(existingPath, exePath) != 0) {
            RegSetValueExA(hKey, "UnifiedSwitch", 0, REG_SZ, (BYTE*)exePath, strlen(exePath) + 1);
        }
        RegCloseKey(hKey);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Prevent multiple instances
    if(IsAlreadyRunning()) {
        MessageBoxA(NULL, "UnifiedSwitch is already running!", "UnifiedSwitch", MB_OK | MB_ICONINFORMATION);
        return 1;
    }
    
    // Load configuration
    LoadConfig();
    
    // Create invisible window for message loop
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "UnifiedSwitchClass";
    
    if(!RegisterClassA(&wc)) {
        MessageBoxA(NULL, "Failed to register window class", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    mainWindow = CreateWindowA("UnifiedSwitchClass", "UnifiedSwitch", 0, 
                               0, 0, 0, 0, NULL, NULL, hInstance, NULL);
    
    if(!mainWindow) {
        MessageBoxA(NULL, "Failed to create window", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    // Install low-level keyboard hook to intercept Win+1/2/3
    keyboardHook = SetWindowsHookExA(WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0);
    if(!keyboardHook) {
        MessageBoxA(NULL, "Failed to install keyboard hook.\nMake sure to run as Administrator!", 
                    "Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    // Auto-start on Windows startup (if user chose this option)
    if(autoStart) {
        AddToStartup();
    }
    
    // Message loop
    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Cleanup
    if(keyboardHook) {
        UnhookWindowsHookEx(keyboardHook);
    }
    
    return 0;
}