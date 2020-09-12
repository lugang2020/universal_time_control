#include <windows.h>
#include <stdio.h>
#include <mmsystem.h>
#include <shellapi.h>
#include <AFXCOM_.H>
#pragma comment(lib,"winmm.lib")
void play(char *path)
{

    //char str[128] = { 0 };
    //int i = 0;
    char buf[128] = { 0 };
    MCI_OPEN_PARMS mciOpen;
    MCIERROR mciError;
    mciOpen.lpstrDeviceType = "mpegvideo";
    mciOpen.lpstrElementName = path;
    mciError = mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_ELEMENT, (DWORD)&mciOpen);
    if (mciError)
    {
        mciGetErrorString(mciError, buf, 128);
        printf("%s/n", buf);
        return;
    }
    UINT DeviceID = mciOpen.wDeviceID;
    MCI_PLAY_PARMS mciPlay;
    mciError = mciSendCommand(DeviceID, MCI_PLAY, 0, (DWORD)&mciPlay);
    if (mciError)
    {
        printf("send MCI_PLAY command failed/n");
        return;
    }
    //WinExec("sndvol32.exe",SW_SHOWNORMAL);
    //这个可以打开音量控制不过可以用编程实现。


    Sleep(2000);
    


}