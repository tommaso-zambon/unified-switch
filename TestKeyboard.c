#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

int main() {
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    
    printf("=== Testing MX Mechanical Keyboard (Bluetooth col03) ===\n\n");
    
    // Your keyboard path TO FILL FROM CONFIG.INI
    const char* path = "\\\\?\\hid#...";
    
    printf("Make sure keyboard is on channel 1!\n");
    printf("Testing switch to channel 2...\n\n");
    
    HANDLE h = CreateFileA(path, GENERIC_WRITE | GENERIC_READ,
                           FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    
    if(h == INVALID_HANDLE_VALUE) {
        printf("Cannot open device: Error %lu\n", GetLastError());
        printf("Press Enter...");
        getchar();
        return 1;
    }
    
    printf("Device opened!\n\n");
    
    // Different packet formats to try
    struct {
        const char* desc;
        BYTE data[20];
        int len;
    } tests[] = {
        {"Report 0x0A (works for mouse)", {0x11, 0x00, 0x0A, 0x1E, 0x01}, 20},
        {"Report 0x09 (older standard)", {0x11, 0x00, 0x09, 0x1E, 0x01}, 20},
        {"Report 0x10", {0x11, 0x00, 0x10, 0x1E, 0x01}, 20},
        {"Report 0x11", {0x11, 0x00, 0x11, 0x1E, 0x01}, 20},
        {"Different command byte 0x1D", {0x11, 0x00, 0x0A, 0x1D, 0x01}, 20},
        {"Different command byte 0x09", {0x11, 0x00, 0x0A, 0x09, 0x01}, 20},
        {"Short packet 0x10", {0x10, 0x00, 0x0A, 0x1E, 0x01, 0x00, 0x00}, 7},
        {"Alt wrapper", {0x10, 0xFF, 0x0A, 0x1E, 0x01, 0x00, 0x00}, 7},
    };
    
    for(int i = 0; i < 8; i++) {
        printf("Test %d: %s\n", i+1, tests[i].desc);
        printf("  Packet (%d bytes): ", tests[i].len);
        for(int j = 0; j < tests[i].len && j < 10; j++) {
            printf("%02X ", tests[i].data[j]);
        }
        printf("\n");
        
        DWORD written;
        BOOL ok = WriteFile(h, tests[i].data, tests[i].len, &written, NULL);
        
        printf("  WriteFile: %s (%lu bytes, Error %lu)\n", 
               ok && written == tests[i].len ? "SUCCESS" : "FAILED", 
               written, GetLastError());
        printf("  >>> DID KEYBOARD SWITCH? <<<\n");
        
        printf("\n");
        Sleep(1500);
    }
    
    CloseHandle(h);
    
    printf("Did any test switch the keyboard?\n");
    printf("Press Enter to exit...");
    getchar();
    return 0;
}