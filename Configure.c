#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <setupapi.h>
#include <hidsdi.h>
#include <stdio.h>
#pragma comment(lib,"hid.lib")
#pragma comment(lib,"setupapi.lib")

#define REPORT_LEN 20

static const WORD kbds[] = {0xB366,0xB365,0xB35C,0xB367};
static const WORD mice[] = {0xB034,0xB033,0xB035,0xB068};

static int rank(const char* p){
    const char* c = strstr(p,"&col0");
    return c && c[5]=='2' ? 2 : c && c[5]=='3' ? 1 : 0;
}

static BOOL test_packet(HANDLE h){
    BYTE pkt[REPORT_LEN] = {0x11, 0x00, 0x09, 0x1E, 0x01};
    DWORD w;
    return WriteFile(h, pkt, REPORT_LEN, &w, NULL) && w == REPORT_LEN;
}

int wmain(){
    GUID g; HidD_GetHidGuid(&g);
    HDEVINFO set = SetupDiGetClassDevsA(&g,NULL,NULL,DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);

    char bestk[512]={},bestm[512]={};
    int rk=-1,rm=-1;

    SP_DEVICE_INTERFACE_DATA ifc = {sizeof(ifc)};
    for(DWORD i=0;SetupDiEnumDeviceInterfaces(set,NULL,&g,i++,&ifc);){
        DWORD sz; SetupDiGetDeviceInterfaceDetailA(set,&ifc,NULL,0,&sz,NULL);
        PSP_DEVICE_INTERFACE_DETAIL_DATA_A d = malloc(sz); d->cbSize=sizeof(*d);
        if(SetupDiGetDeviceInterfaceDetailA(set,&ifc,d,sz,NULL,NULL)){
            HANDLE h = CreateFileA(d->DevicePath,GENERIC_READ|GENERIC_WRITE,
                                   FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
            if(h!=INVALID_HANDLE_VALUE){
                HIDD_ATTRIBUTES a={sizeof(a)};
                if(HidD_GetAttributes(h,&a) && a.VendorID==0x046D){
                    int r = rank(d->DevicePath);
                    if(r>0 && test_packet(h)){
                        for(int j=0;j<4;j++){
                            if(a.ProductID==kbds[j] && r>rk){ strcpy(bestk,d->DevicePath); rk=r; }
                            if(a.ProductID==mice[j] && r>rm){ strcpy(bestm,d->DevicePath); rm=r; }
                        }
                    }
                }
                CloseHandle(h);
            }
        }
        free(d);
    }
    SetupDiDestroyDeviceInfoList(set);

    if(bestk[0] && bestm[0]){
        FILE* f = fopen("config.ini","w");
        if(f){
            fprintf(f,"[PATHS]\nclickmon=dependencies\\ControlMyMonitor.exe\n\n");
            fprintf(f,"[INTERFACES]\nkeyboard_path=%s\nmouse_path=%s\n\n",bestk,bestm);
            fprintf(f,"[SOURCES]\ndevice1=\ndevice2=\ndevice3=\n\n");
            fprintf(f,"[SETTINGS]\nmultiMonitor=0\n");
            fclose(f);
        }
    }
    return 0;
}