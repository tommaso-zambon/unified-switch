#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>

/*
 * UnifiedSwitch - Bluetooth Channel Switcher
 * 
 * ADDING NEW DEVICES:
 * 1. Find your device PID in Configure_debug.exe output
 * 2. Test which report ID works using TestKeyboard.exe or TestMouseCol02.exe
 * 3. Add entry to bt_devices[] array below
 * 4. Submit PR with: Device name, PID, working report ID
 * 
 * Example: {0xB366, 0x09, "MX Mechanical"}
 *          └─PID   └─ID  └─Name (for reference)
 */

static char k[512], m[512];
static BOOL loaded = 0;

typedef enum {
    PROTO_BLUETOOTH = 0,
    PROTO_RECEIVER
} Protocol;

static void load(void) {
    if(loaded) return;
    GetPrivateProfileStringA("INTERFACES", "keyboard_path", "", k, 512, ".\\config.ini");
    GetPrivateProfileStringA("INTERFACES", "mouse_path", "", m, 512, ".\\config.ini");
    loaded = 1;
}

static Protocol detect_protocol(const char* p) {
    if(strstr(p, "00001812-0000-1000-8000-00805f9b34fb")) 
        return PROTO_BLUETOOTH;
    return PROTO_RECEIVER;
}

// Bluetooth device database - maps PID to known working report ID
// ADD NEW DEVICES HERE as they are tested
typedef struct {
    WORD pid;
    BYTE report_id;
    const char* name;  // For reference only
} BT_Device;

static const BT_Device bt_devices[] = {
    // Keyboards
    {0xB366, 0x09, "MX Mechanical"},        // Tested: works with 0x09
    {0xB365, 0x09, "MX Keys"},              // Assumed: same family as MX Mechanical
    {0xB35C, 0x09, "MX Keys Mini"},         // Assumed: same family
    
    // Mice
    {0xB034, 0x0A, "MX Master 3S"},         // Tested: works with 0x0A
    {0xB033, 0x0A, "MX Master 3"},          // Assumed: same family as 3S
    {0xB07C, 0x0A, "MX Master 2S"},         // Assumed: older MX series
};

// Get the correct report ID for a Bluetooth device
// Returns 0 if device not in database (caller should try defaults)
static BYTE get_bluetooth_report_id(const char* path) {
    // Extract PID from path (format: "pid&bXXX" or "pid_bXXX")
    const char* pid_str = strstr(path, "pid&");
    if(!pid_str) pid_str = strstr(path, "pid_");
    if(!pid_str) return 0;
    
    // Parse hex PID (skip "pid&" or "pid_")
    WORD pid = 0;
    sscanf(pid_str + 4, "%hx", &pid);
    
    // Look up in device database
    for(int i = 0; i < sizeof(bt_devices)/sizeof(bt_devices[0]); i++) {
        if(bt_devices[i].pid == pid) {
            return bt_devices[i].report_id;
        }
    }
    
    return 0;  // Not found
}

static BOOL switch_bluetooth(const char* path, int channel) {
    HANDLE h = CreateFileA(path, GENERIC_WRITE | GENERIC_READ, 
                           FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if(h == INVALID_HANDLE_VALUE) return FALSE;
    
    BYTE report_id = get_bluetooth_report_id(path);
    
    if(report_id != 0) {
        // Device found in database - use known working report ID
        BYTE cmd[20] = {0x11, 0x00, report_id, 0x1E, (BYTE)channel};
        DWORD written;
        BOOL success = WriteFile(h, cmd, 20, &written, NULL) && written == 20;
        CloseHandle(h);
        return success;
    } else {
        // Unknown device - try common report IDs
        const BYTE fallback_ids[] = {0x09, 0x0A};
        DWORD written;
        BOOL success = FALSE;
        
        for(int i = 0; i < 2 && !success; i++) {
            BYTE cmd[20] = {0x11, 0x00, fallback_ids[i], 0x1E, (BYTE)channel};
            success = WriteFile(h, cmd, 20, &written, NULL) && written == 20;
            if(!success) Sleep(10);
        }
        
        CloseHandle(h);
        return success;
    }
}

static BOOL switch_bolt(const char* path, int channel) {
    // TODO: Implement HID++ 2.0 protocol
    // For now, devices must use Bluetooth
    return FALSE;
}

int main(int argc, char** argv) {
    load();
    if(!k[0] && !m[0]) return 1;
    
    int channel = (argc > 1) ? atoi(argv[1]) : 1;
    if(channel < 1 || channel > 3) channel = 1;
    int hidChannel = channel - 1;
    
    Protocol kproto = detect_protocol(k);
    Protocol mproto = detect_protocol(m);
    
    BOOL kb_ok = TRUE, mouse_ok = TRUE;
    
    // Switch keyboard
    if(k[0]) {
        if(kproto == PROTO_BLUETOOTH) {
            kb_ok = switch_bluetooth(k, hidChannel);
        } else {
            kb_ok = switch_bolt(k, hidChannel);
        }
    }
    
    // Delay between devices
    if(k[0] && m[0]) Sleep(80);
    
    // Switch mouse
    if(m[0]) {
        if(mproto == PROTO_BLUETOOTH) {
            mouse_ok = switch_bluetooth(m, hidChannel);
        } else {
            mouse_ok = switch_bolt(m, hidChannel);
        }
    }
    
    // Wake mouse cursor
    if(m[0] && mouse_ok) {
        Sleep(40);
        mouse_event(MOUSEEVENTF_MOVE, 1, 0, 0, 0);
        mouse_event(MOUSEEVENTF_MOVE, -1, 0, 0, 0);
    }
    
    return (kb_ok && mouse_ok) ? 0 : 1;
}