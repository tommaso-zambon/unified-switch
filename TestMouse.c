#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

int main() {
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    
    printf("=== Testing Bluetooth col02 Mouse ===\n\n");
    
    // Your mouse path TO FILL FROM CONFIG.INI
    const char* path = "\\\\?\\hid#...";
    
    printf("Make sure you're on channel 1!\n");
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
        {"Standard (0x11, 0x00, 0x09)", {0x11, 0x00, 0x09, 0x1E, 0x01}, 20},
        {"Alt Report (0x10)", {0x10, 0x00, 0x09, 0x1E, 0x01, 0x00, 0x00}, 7},
        {"Direct (no wrapper)", {0x09, 0x1E, 0x01, 0x00, 0x00, 0x00, 0x00}, 7},
        {"Long format", {0x11, 0x00, 0x0A, 0x1E, 0x01}, 20},
        {"Report 0x0A", {0x11, 0x00, 0x0A, 0x1E, 0x01}, 20},
        {"Report 0x10", {0x11, 0x00, 0x10, 0x1E, 0x01}, 20},
        {"Report 0x11", {0x11, 0x00, 0x11, 0x1E, 0x01}, 20},
    };
    
    for(int i = 0; i < 7; i++) {
        printf("Test %d: %s\n", i+1, tests[i].desc);
        printf("  Packet (%d bytes): ", tests[i].len);
        for(int j = 0; j < tests[i].len && j < 10; j++) {
            printf("%02X ", tests[i].data[j]);
        }
        printf("\n");
        
        DWORD written;
        if(WriteFile(h, tests[i].data, tests[i].len, &written, NULL) && written == tests[i].len) {
            printf("  Result: SUCCESS - DID MOUSE SWITCH?\n");
        } else {
            printf("  Result: FAILED (Error %lu)\n", GetLastError());
        }
        
        printf("\n");
        Sleep(1000);
    }
    
    CloseHandle(h);
    
    printf("Did any test switch the mouse?\n");
    printf("Press Enter to exit...");
    getchar();
    return 0;
}