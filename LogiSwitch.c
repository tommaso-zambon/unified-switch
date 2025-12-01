#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>

static char k[512], m[512];
static int loaded = 0;

static void load(void) {
    if (loaded) return;
    GetPrivateProfileStringA("INTERFACES","keyboard_path","",k,512,".\\config.ini");
    GetPrivateProfileStringA("INTERFACES","mouse_path","",m,512,".\\config.ini");
    loaded = 1;
}

static void sw(const char* p, int n) {
    HANDLE h = CreateFileA(p,GENERIC_WRITE|GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
    if (h == INVALID_HANDLE_VALUE) return;
    BYTE w[20] = {0x11,0x00,0x09,0x02,0x00};
    BYTE c[20] = {0x11,0x00,0x09,0x1E,(BYTE)n};
    if (strstr(p,"b03") || strstr(p,"B03")) w[2] = c[2] = 0x0A;
    DWORD _;
    WriteFile(h,w,20,&_,NULL);
    Sleep(45);
    WriteFile(h,c,20,&_,NULL);
    CloseHandle(h);
}

int wmain(int argc, wchar_t** argv) {
    load();
    if (!k[0] || !m[0]) return 1;
    int p = (argc > 1) ? _wtoi(argv[1]) : 1;
    if (p < 1 || p > 3) p = 1;
    sw(k,p);
    Sleep(30);
    sw(m,p);
    mouse_event(MOUSEEVENTF_MOVE,1,0,0,0);
    mouse_event(MOUSEEVENTF_MOVE,-1,0,0,0);
    return 0;
}