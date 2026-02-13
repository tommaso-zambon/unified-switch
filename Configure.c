#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <setupapi.h>
#include <hidsdi.h>
#include <stdio.h>
#pragma comment(lib,"hid.lib")
#pragma comment(lib,"setupapi.lib")

#define LOGITECH_VID 0x046D

// Supported device PIDs
static const WORD kbds[] = {0xB366, 0xB365, 0xB35C, 0xB367, 0xB36B, 0xB371};
static const WORD mice[] = {0xB034, 0xB033, 0xB035, 0xB068, 0xB07C, 0xB07D};
static const WORD bolt_recv = 0xC548;
static const WORD unifying_recv = 0xC52B;

typedef enum {
    DEV_UNKNOWN = 0,
    DEV_BLUETOOTH,
    DEV_BOLT,
    DEV_UNIFYING
} DevType;

static DevType get_type(const char* p, WORD pid) {
    if(pid == bolt_recv) return DEV_BOLT;
    if(pid == unifying_recv) return DEV_UNIFYING;
    if(strstr(p, "00001812-0000-1000-8000-00805f9b34fb")) return DEV_BLUETOOTH;
    return DEV_UNKNOWN;
}

static int rank(const char* p, DevType type) {
    if(type == DEV_BOLT || type == DEV_UNIFYING) {
        int base = (type == DEV_BOLT) ? 200 : 100;
        if(strstr(p, "&mi_02")) {
            if(strstr(p, "&col01")) return base + 10;
            if(strstr(p, "&col02")) return base + 9;
        }
        if(strstr(p, "&mi_01")) {
            if(strstr(p, "&col01")) return base + 3;
            if(strstr(p, "&col02")) return base + 2;
            if(strstr(p, "&col03")) return base + 1;
        }
        return base;
    }
    
    int base = 300;
    if(strstr(p, "&col03")) return base + 3;
    if(strstr(p, "&col01")) return base + 2;
    if(strstr(p, "&col02")) return base + 1;
    return base;
}

static int GetHotkeyChoice() {
    int result = MessageBoxA(NULL, 
        "Choose your hotkey:\n\n"
        "YES - Win+1/2/3\n"
        "NO - Ctrl+Alt+1/2/3 (recommended)\n"
        "CANCEL - Ctrl+Shift+1/2/3",
        "UnifiedSwitch Setup - Hotkeys",
        MB_YESNOCANCEL | MB_ICONASTERISK | MB_DEFBUTTON2);
    
    if(result == IDYES) return 1;
    if(result == IDNO) return 2;
    return 3;
}

static BOOL GetStartupChoice() {
    int result = MessageBoxA(NULL,
        "Start automatically with Windows?",
        "UnifiedSwitch Setup - Startup",
        MB_YESNO | MB_ICONASTERISK | MB_DEFBUTTON1);
    return (result == IDYES);
}

static BOOL GetMultiMonitorChoice() {
    int result = MessageBoxA(NULL,
        "Do you use multiple monitors?\n\n"
        "YES - Keep current monitor setup\n"
        "NO - Auto-switch monitor input",
        "UnifiedSwitch Setup - Monitors",
        MB_YESNO | MB_ICONASTERISK | MB_DEFBUTTON1);
    return (result == IDYES) ? 1 : 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    int result = MessageBoxA(NULL,
        "UnifiedSwitch Setup\n\n"
        "This will detect your devices and configure hotkeys.",
        "Welcome",
        MB_OKCANCEL | MB_ICONINFORMATION);
    
    if(result != IDOK) {
        return 0;  // User pressed Cancel or X
    }
    
    GUID g;
    HidD_GetHidGuid(&g);
    HDEVINFO set = SetupDiGetClassDevsA(&g, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    char bestk[512] = {}, bestm[512] = {};
    int rk = -1, rm = -1;
    DevType kt = DEV_UNKNOWN, mt = DEV_UNKNOWN;

    SP_DEVICE_INTERFACE_DATA ifc = {sizeof(ifc)};
    for(DWORD i = 0; SetupDiEnumDeviceInterfaces(set, NULL, &g, i++, &ifc);) {
        DWORD sz;
        SetupDiGetDeviceInterfaceDetailA(set, &ifc, NULL, 0, &sz, NULL);
        PSP_DEVICE_INTERFACE_DETAIL_DATA_A d = (PSP_DEVICE_INTERFACE_DETAIL_DATA_A)malloc(sz);
        d->cbSize = sizeof(*d);
        
        if(SetupDiGetDeviceInterfaceDetailA(set, &ifc, d, sz, NULL, NULL)) {
            HANDLE h = CreateFileA(d->DevicePath, GENERIC_READ | GENERIC_WRITE,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
            if(h != INVALID_HANDLE_VALUE) {
                HIDD_ATTRIBUTES a = {sizeof(a)};
                if(HidD_GetAttributes(h, &a) && a.VendorID == LOGITECH_VID) {
                    DevType dtype = get_type(d->DevicePath, a.ProductID);
                    int r = rank(d->DevicePath, dtype);
                    
                    if(r > 0) {
                        for(int j = 0; j < sizeof(kbds)/sizeof(kbds[0]); j++) {
                            if(a.ProductID == kbds[j] && r > rk) {
                                strcpy(bestk, d->DevicePath);
                                rk = r;
                                kt = dtype;
                            }
                        }
                        for(int j = 0; j < sizeof(mice)/sizeof(mice[0]); j++) {
                            if(a.ProductID == mice[j] && r > rm) {
                                strcpy(bestm, d->DevicePath);
                                rm = r;
                                mt = dtype;
                            }
                        }
                        if(a.ProductID == bolt_recv || a.ProductID == unifying_recv) {
                            if(r > rk) {
                                strcpy(bestk, d->DevicePath);
                                rk = r;
                                kt = dtype;
                            }
                            if(r > rm) {
                                strcpy(bestm, d->DevicePath);
                                rm = r;
                                mt = dtype;
                            }
                        }
                    }
                }
                CloseHandle(h);
            }
        }
        free(d);
    }
    SetupDiDestroyDeviceInfoList(set);

    if(!bestk[0] && !bestm[0]) {
        MessageBoxA(NULL, 
            "No Logitech devices found.\n\n"
            "Make sure your devices are:\n"
            "• Powered on and paired\n"
            "• Visible in Device Manager",
            "Setup Error",
            MB_OK | MB_ICONERROR);
        return 1;
    }

    int hotkeyMode = GetHotkeyChoice();
    BOOL autoStart = GetStartupChoice();
    BOOL multiMonitor = GetMultiMonitorChoice();
    
    FILE* f = fopen("config.ini", "w");
    if(f) {
        fprintf(f,"[PATHS]\nclickmon=dependencies\\ControlMyMonitor.exe\n\n");
        fprintf(f,"[INTERFACES]\n");
        if(bestk[0]) fprintf(f,"keyboard_path=%s\n", bestk);
        if(bestm[0]) fprintf(f,"mouse_path=%s\n", bestm);
        fprintf(f,"\n[SOURCES]\ndevice1=\ndevice2=\ndevice3=\n\n");
        fprintf(f,"[SETTINGS]\nmultiMonitor=%d\nhotkeyMode=%d\nautoStart=%d\n", 
                multiMonitor, hotkeyMode, autoStart ? 1 : 0);
        fclose(f);
        
        const char* ktype = (kt == DEV_BLUETOOTH) ? "Bluetooth" : (kt == DEV_BOLT) ? "Bolt" : "Unifying";
        const char* mtype = (mt == DEV_BLUETOOTH) ? "Bluetooth" : (mt == DEV_BOLT) ? "Bolt" : "Unifying";
        const char* hotkeys[] = {"", "Win+1/2/3", "Ctrl+Alt+1/2/3", "Ctrl+Shift+1/2/3"};
        
        char msg[512];
        snprintf(msg, 512,
            "Setup complete!\n\n"
            "%s%s\n"
            "%s%s\n\n"
            "Hotkeys: %s\n"
            "Auto-start: %s\n\n"
            "Run UnifiedSwitch.exe to start!",
            bestk[0] ? "Keyboard: " : "", bestk[0] ? ktype : "",
            bestm[0] ? "Mouse: " : "", bestm[0] ? mtype : "",
            hotkeys[hotkeyMode],
            autoStart ? "Yes" : "No");
        
        MessageBoxA(NULL, msg, "Success", MB_OK | MB_ICONINFORMATION);
        return 0;
    }
    
    MessageBoxA(NULL, "Failed to save configuration.", "Error", MB_OK | MB_ICONERROR);
    return 1;
}