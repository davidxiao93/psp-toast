#ifndef __MAIN_H__
#define __MAIN_H__

#include <pspctrl.h>
#include <pspkernel.h>
#include <psprtc.h>

int sceCtrl_driver_PeekBufferPositive(SceCtrlData *pad_data, int count);
int sceRtc_driver_GetCurrentClockLocalTime(pspTime *time);

/*
int sceUsb_driver_Start(const char* driverName, int size, void *args);
int sceUsb_driver_Stop(const char* driverName, int size, void *args);
int sceUsb_driver_Activate(u32 pid);
int sceUsb_driver_Deactivate(u32 pid);
int sceUsb_driver_GetState(void);
*/

#endif
