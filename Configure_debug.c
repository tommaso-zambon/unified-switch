#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <setupapi.h>
#include <hidsdi.h>
#include <stdio.h>
#pragma comment(lib,"hid.lib")
#pragma comment(lib,"setupapi.lib")

#define LOGITECH_VID 0x046D

// Known Logitech device PIDs
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

static const char* type_name(DevType t) {
    switch(t) {
        case DEV_BLUETOOTH: return "Bluetooth";
        case DEV_BOLT: return "Logi Bolt";
        case DEV_UNIFYING: return "Unifying";
        default: return "Unknown";
    }
}

static const char* device_name(WORD pid) {
    // Keyboards
    if(pid == 0xB366) return "MX Mechanical";
    if(pid == 0xB365) return "MX Keys";
    if(pid == 0xB35C) return "MX Keys Mini";
    
    // Mice
    if(pid == 0xB034) return "MX Master 3S";
    if(pid == 0xB033) return "MX Master 3";
    if(pid == 0xB07C) return "MX Master 2S";
    
    // Receivers
    if(pid == bolt_recv) return "Logi Bolt Receiver";
    if(pid == unifying_recv) return "Unifying Receiver";
    
    return "Unknown Device";
}

int main() {
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    
    printf("================================================\n");
    printf("  UnifiedSwitch - Device Scanner (DEBUG)\n");
    printf("================================================\n\n");
    
    GUID g;
    HidD_GetHidGuid(&g);
    HDEVINFO set = SetupDiGetClassDevsA(&g, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    
    int bluetooth_count = 0;
    int bolt_count = 0;
    int total_count = 0;

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
                    
                    if(dtype != DEV_UNKNOWN) {
                        total_count++;
                        if(dtype == DEV_BLUETOOTH) bluetooth_count++;
                        if(dtype == DEV_BOLT) bolt_count++;
                        
                        printf("------------------------------------------------\n");
                        printf("Device #%d\n", total_count);
                        printf("Name: %s\n", device_name(a.ProductID));
                        printf("PID: 0x%04X\n", a.ProductID);
                        printf("Type: %s\n", type_name(dtype));
                        
                        // Check if it's a known device type
                        BOOL is_keyboard = FALSE, is_mouse = FALSE, is_receiver = FALSE;
                        for(int j = 0; j < sizeof(kbds)/sizeof(kbds[0]); j++) {
                            if(a.ProductID == kbds[j]) is_keyboard = TRUE;
                        }
                        for(int j = 0; j < sizeof(mice)/sizeof(mice[0]); j++) {
                            if(a.ProductID == mice[j]) is_mouse = TRUE;
                        }
                        if(a.ProductID == bolt_recv || a.ProductID == unifying_recv) {
                            is_receiver = TRUE;
                        }
                        
                        if(is_keyboard) printf("Category: KEYBOARD\n");
                        else if(is_mouse) printf("Category: MOUSE\n");
                        else if(is_receiver) printf("Category: RECEIVER\n");
                        else printf("Category: UNKNOWN\n");
                        
                        // Show collection info for Bluetooth
                        if(dtype == DEV_BLUETOOTH) {
                            if(strstr(d->DevicePath, "&col03")) printf("Collection: col03 (Primary - USE THIS) *** BEST\n");
                            else if(strstr(d->DevicePath, "&col02")) printf("Collection: col02 (Secondary) [WRONG - need col03!]\n");
                            else if(strstr(d->DevicePath, "&col01")) printf("Collection: col01 (Control)\n");
                        }
                        
                        // Show interface info for receivers
                        if(dtype == DEV_BOLT || dtype == DEV_UNIFYING) {
                            if(strstr(d->DevicePath, "&mi_02&col01")) printf("Interface: mi_02 col01 (HID++ Control - USE THIS)\n");
                            else if(strstr(d->DevicePath, "&mi_02&col02")) printf("Interface: mi_02 col02 (HID++ Data)\n");
                            else if(strstr(d->DevicePath, "&mi_01")) printf("Interface: mi_01 (Media/Consumer)\n");
                        }
                        
                        printf("Path: %s\n", d->DevicePath);
                    }
                }
                CloseHandle(h);
            }
        }
        free(d);
    }
    SetupDiDestroyDeviceInfoList(set);
    
    printf("================================================\n");
    printf("Summary:\n");
    printf("  Total Logitech devices: %d\n", total_count);
    printf("  Bluetooth devices: %d\n", bluetooth_count);
    printf("  Bolt/Unifying receivers: %d\n", bolt_count);
    
    if(total_count == 0) {
        printf("\n[!] No devices found!\n");
        printf("    Make sure devices are powered on and paired.\n");
    } else {
        printf("\n[OK] Devices detected successfully!\n");
    }
    
    printf("================================================\n");
    printf("\nPress Enter to exit...");
    getchar();
    
    return 0;
}