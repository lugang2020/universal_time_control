#include <windows.h>
#include <stdio.h>
#include <mmsystem.h>
#include <shellapi.h>
//#include <AFXCOM_.H>


extern void logMessage(const char* fmt, ...);

#pragma comment(lib,"winmm.lib")
void play(wchar_t *path)
{

    logMessage("play thread:%s\n",path);
    char buf[128] = { 0 };
    MCI_OPEN_PARMS mciOpen;
    MCIERROR mciError;
    mciOpen.lpstrDeviceType = L"mpegvideo";
    mciOpen.lpstrElementName = path;
    mciError = mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_ELEMENT, (DWORD)&mciOpen);
    if (mciError)
    {
        mciGetErrorStringA(mciError, buf, 128);
        logMessage("%s", buf);
        return;
    }
    UINT DeviceID = mciOpen.wDeviceID;
    MCI_PLAY_PARMS mciPlay;
    mciError = mciSendCommand(DeviceID, MCI_PLAY, 0, (DWORD)&mciPlay);
    if (mciError)
    {
        logMessage("send MCI_PLAY command failed/n");
        return;
    }
    //WinExec("sndvol32.exe",SW_SHOWNORMAL);
    //������Դ��������Ʋ��������ñ��ʵ�֡�


    Sleep(2000);
    


}