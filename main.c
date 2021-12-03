#include <pspsdk.h>
#include <pspkernel.h>
#include <pspdisplay_kernel.h>
#include <pspdisplay.h>
#include <pspimpose_driver.h>
#include <pspinit.h>
#include <pspctrl.h>
#include <pspmoduleinfo.h>
//#include <pspsysmem.h>

#include <stdio.h>
#include <string.h>
#include "screenshot.h"
#include "main.h"
#include "draw.h"
#include "sysconhk.h"
#include "apihook.h"

typedef struct _Config
{
		int brightness;
} Config;

static Config config;
static char configPath[128];
static int lastSavedBrightness = -1;


PSP_MODULE_INFO("PSP-TOAST", PSP_MODULE_KERNEL, 1, 0);
PSP_MAIN_THREAD_ATTR(0);
PSP_MAIN_THREAD_STACK_SIZE_KB(0);

#define BRIGHTNESS_CONFIG_FILE "brightness.bin"

#define MAX(X,Y) ((X)>(Y)?(X):(Y))
#define MIN(X,Y) ((X)<(Y)?(X):(Y))
#define SWITCH(X) X=(X+1)&1
#define SWAP(X,Y,TEMP) TEMP=X,X=Y,Y=TEMP

#define MIN_LEVEL 20 // Minimum brightness level
#define STEP_LEVEL 20 // Steps to in increasing brightness

u32 SELECTION_FILL = RGB(0, 0, 0);
u32 SELECTION_BORDER = RGB(0, 0, 0);

CANVAS displayCanvas;
int drawing_mode = 0; // 0 - main thread, 1 - display hook
int wait = 0;



const int showTime = 200;

int wlanTicker = showTime + 1;
int volumeTicker = showTime + 1;
int brightnessTicker = showTime + 1;
int holdTicker = showTime + 1;

int wlanStatus = 0; // 0 for off, 1 for on
int holdStatus = 0;


void showWlanStatus() {
	wlanTicker = 0;
}

void showVolume() {
	volumeTicker = 0;
}

void showBrightness() {
	brightnessTicker = 0;
}

void showHoldStatus() {
	holdTicker = 0;
}


int getBrightness() {
	int ret;
	u32 k1;
    k1 = pspSdkSetK1(0);
	sceDisplayGetBrightness(&ret, 0);
    pspSdkSetK1(k1);
    return ret;
}

void setBrightness(int value){
	u32 k1;
    k1 = pspSdkSetK1(0);
	sceDisplaySetBrightness(value, 0);
    pspSdkSetK1(k1);
	config.brightness = value;
}

int getVolume() {
	return sceImposeGetParam(PSP_IMPOSE_MAIN_VOLUME);
}

const char* getWlanStatus() {
	if (wlanStatus == 1) {
		return "ON";
	}
	return "OFF";
}

const char* getHoldStatus() {
	if (holdStatus == 1) {
		return "ON";
	}
	return "OFF";
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//  MENU NAVIGATION
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

SceCtrlData pad;
int pad_old = 0;
int display_width = 480;
int display_height = 272;
int screenshot_timer = -1;

void initInputs() {
	sceCtrl_driver_PeekBufferPositive(&pad, 1);

	if (pad.Buttons & PSP_CTRL_WLAN_UP) {
		wlanStatus = 1;
	}

	if (pad.Buttons & PSP_CTRL_HOLD) {
		holdStatus = 1;
	}
}

static SceCtrlData sysconRawCtrl = {0};
static u32 sysconPrevButtons, sysconNewButtons;
static int screenButtonPressed = 0;

void syscon_ctrl(sceSysconPacket *packet)
{
	sysconNewButtons = 0; // Clear new buttons
	
	switch(packet->rx_response)
	{
	case 0x08: // Used for analog stick

	case 0x07: // Used for digital buttons
		sysconNewButtons = syscon_get_dword(packet->rx_data);
		sysconNewButtons = ~sysconNewButtons;

		sysconPrevButtons = sysconRawCtrl.Buttons;
		sysconRawCtrl.Buttons = sysconNewButtons;


		if (sysconNewButtons & SYSCON_CTRL_NOTE) {
			// Note button has been pressed
			if (screenshot_timer == -1) {
				// only start taking a screenshot if not currently taking a screenshot
				screenshot_timer = 10;
			}
			// Remove system handling of the note button
			sysconNewButtons &= ~SYSCON_CTRL_NOTE;
			// scePowerTick(PSP_POWER_TICK_DISPLAY);
		} else {
			if (sysconNewButtons & SYSCON_CTRL_VOL_UP || sysconNewButtons & SYSCON_CTRL_VOL_DN) {
				// volume button has been pressed
				showVolume();
			}

			if (sysconNewButtons & SYSCON_CTRL_WLAN && wlanStatus == 0) {
				// wlan has been switched on
				wlanStatus = 1;
				showWlanStatus();
			} else if (!(sysconNewButtons & SYSCON_CTRL_WLAN) && wlanStatus == 1) {
				// wlan has been switched off
				wlanStatus = 0;
				showWlanStatus();
			}

			if (sysconNewButtons & SYSCON_CTRL_HOLD && holdStatus == 0) {
				// hold has been switched on
				holdStatus = 1;
				showHoldStatus();
			} else if (!(sysconNewButtons & SYSCON_CTRL_HOLD) && holdStatus == 1) {
				// hold has been switched off
				holdStatus = 0;
				showHoldStatus();
			}


			if (!(sysconNewButtons & SYSCON_CTRL_LCD) && screenButtonPressed == 1) {
				// screen button has been released
				screenButtonPressed = 0;
			}

			if (sysconNewButtons & SYSCON_CTRL_LCD && screenButtonPressed == 0) {
				// screen button has been pressed

				int brightness = getBrightness();

				if (brightness + STEP_LEVEL > 100) {
					setBrightness(MIN_LEVEL);
				} else {
					setBrightness(brightness + STEP_LEVEL);
				}
				
				showBrightness();

				screenButtonPressed = 1;
			}

			if (sysconNewButtons & SYSCON_CTRL_LCD) {
				// Remove system handling of this button
				sysconNewButtons &= ~SYSCON_CTRL_LCD;

				// sleep???
				// scePowerTick(PSP_POWER_TICK_DISPLAY);
			}
		}

	}
	
	// Put the data to syscon..
	syscon_put_dword(packet->rx_data,~sysconNewButtons);
	syscon_make_checksum(&packet->rx_sts);
 
}



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//  TOAST DRAWING
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void drawToast(CANVAS *canvas) {
	int x = 480/2, y;
	int sel_x = 180, sel_w = 480 - sel_x * 2;

	if (wlanTicker < showTime) {
		y = 200;
		char wlanText[255];
		sprintf(wlanText, "WLAN: %s", getWlanStatus());
		fillRectangle(canvas, sel_x, y-2, sel_w, 11, SELECTION_FILL, SELECTION_BORDER);
		drawSmallFont(canvas, wlanText, x - strlen(wlanText)*3, y);
		wlanTicker += 1;
	}

	if (volumeTicker < showTime) {
		y = 210;
		char volumeText[255];
		sprintf(volumeText, "VOLUME: %02d/30", getVolume());
		fillRectangle(canvas, sel_x, y-2, sel_w, 11, SELECTION_FILL, SELECTION_BORDER);
		drawSmallFont(canvas, volumeText, x - strlen(volumeText)*3, y);
		volumeTicker += 1;
	}

	if (brightnessTicker < showTime) {
		y = 220;
		char brightnessText[255];
		sprintf(brightnessText, "BRIGHTNESS: %03d", getBrightness());
		fillRectangle(canvas, sel_x, y-2, sel_w, 11, SELECTION_FILL, SELECTION_BORDER);
		drawSmallFont(canvas, brightnessText, x - strlen(brightnessText)*3, y);
		brightnessTicker += 1;
	}

	if (holdTicker < showTime) {
		y = 230;
		char holdText[255];
		sprintf(holdText, "HOLD: %s", getHoldStatus());
		fillRectangle(canvas, sel_x, y-2, sel_w, 11, SELECTION_FILL, SELECTION_BORDER);
		drawSmallFont(canvas, holdText, x - strlen(holdText)*3, y);
		holdTicker += 1;
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//  DISPLAY HOOKING
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int fps = 0;

SceUInt32 fps_last_clock = 0;
int fps_counter = 0;
int fps_last_counter = 0;
int last_fps = -1;
int getFPS()
{
	int current_fps = last_fps;
	SceKernelSysClock clock;
	sceKernelGetSystemTime(&clock);
	if((clock.low - fps_last_clock) >= 1000000) {
		if(fps_last_clock > 0 && fps_last_clock < clock.low) {
			int el_clock = clock.low - fps_last_clock;
			if(el_clock > 0) current_fps = (int)((fps_counter - fps_last_counter) * 1000000 / el_clock);
		}
		fps_last_clock = clock.low;
		fps_last_counter = fps_counter;
	}
	last_fps = current_fps;
	return current_fps;
}


int display_hooked = 0;

int display_set_frame_buf(void *topaddr, int bufferwidth, int pixelformat, int sync)
{
	fps_counter++;
	if(drawing_mode == 1 && wait == 0 && screenshot_timer < 0) {
		wait = 1;
		int mode = 0;
		sceDisplayGetMode(&mode, &(displayCanvas.width), &(displayCanvas.height));
		displayCanvas.buffer = topaddr;
		displayCanvas.lineWidth = bufferwidth;
		displayCanvas.pixelFormat = pixelformat;
		display_width = displayCanvas.width;
		display_height = displayCanvas.height;
		drawToast(&displayCanvas);
		wait = 0;
	}
	return sceDisplaySetFrameBuf(topaddr, bufferwidth, pixelformat, sync);
}

void hookDisplay()
{
	SceModule *pMod = sceKernelFindModuleByName("sceDisplay_Service");
	if(pMod != NULL) {
		if(apiHookByName(pMod->modid, "sceDisplay", "sceDisplaySetFrameBuf", display_set_frame_buf) != 0) display_hooked = 1;
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//  MAIN THREAD
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int getConfig(Config *cfg)
{
	SceUID fd;
	u32 k1;
   
	k1 = pspSdkSetK1(0);
	memset(cfg, 0, sizeof(*cfg));
	fd = sceIoOpen(configPath, PSP_O_RDONLY, 0644);

	if (fd < 0) {
		pspSdkSetK1(k1);
		return -1;
	}

	if (sceIoRead(fd, cfg, sizeof(*cfg)) != sizeof(*cfg)) {
		sceIoClose(fd);
		pspSdkSetK1(k1);
		return -2;
	}

	sceIoClose(fd);
	pspSdkSetK1(k1);

	return 0;
}

int setConfig(Config *cfg)
{
	u32 k1;
	SceUID fd;

	k1 = pspSdkSetK1(0);
	sceIoRemove(configPath);
	fd = sceIoOpen(configPath, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);

	if (fd < 0) {
		pspSdkSetK1(k1);
		return -1;
	}

	if (sceIoWrite(fd, cfg, sizeof(*cfg)) != sizeof(*cfg)) {
		sceIoClose(fd);
		pspSdkSetK1(k1);
		return -1;
	}

	sceIoClose(fd);
	pspSdkSetK1(k1);

	return 0;
}

void saveBrightness(void) {
	// Save configs if need
	if (lastSavedBrightness != config.brightness) {
		setConfig(&config);
		lastSavedBrightness = config.brightness;
	}
}


int main_thread(SceSize argc, void* argp)
{

	if (getConfig(&config) != 0) {
		config.brightness = getBrightness();
	} else {
		 // Check for min and max brightness values
		if (config.brightness < 11) config.brightness = 11;
		if (config.brightness > 100) config.brightness = 100;
		// Set the brightness
		setBrightness(config.brightness);
	}
	lastSavedBrightness = config.brightness;


	SceKernelSysClock clock_start, clock_end;

	sceKernelDelayThread(2000000);
	hookDisplay();
	initInputs();
	install_syscon_hook();

	while(1) {

		saveBrightness();

		sceKernelGetSystemTime(&clock_start);
		if(screenshot_timer > 0) {
			screenshot_timer--;
		} else if(screenshot_timer == 0) {
			screenshot();
			screenshot_timer = -1;
		}

		drawing_mode = getFPS() > 0;

		if(drawing_mode == 0) {
			if(screenshot_timer < 0 && wait == 0) {
				sceDisplayWaitVblankStart();
				wait = 1;
				if(getCanvas(&displayCanvas)) {
					display_width = displayCanvas.width;
					display_height = displayCanvas.height;

					int vblank = (int)(1000000.0f / sceDisplayGetFramePerSec());
					int vblank_min = vblank / 10;
					int vblank_max = vblank - vblank_min;

					sceKernelGetSystemTime(&clock_end);
					int calc_speed = (int)clock_end.low - (int)clock_start.low;

					// draw info and menu, measure time for drawing
					sceKernelGetSystemTime(&clock_start);
					drawToast(&displayCanvas);
					sceKernelGetSystemTime(&clock_end);

					// calculate delay time
					int draw_speed = (int)clock_end.low - (int)clock_start.low;
					int delay = vblank - draw_speed*3 - calc_speed;
					if(delay < vblank_min) delay = vblank_min;
					if(delay > vblank_max) delay = vblank_max;
		
					sceKernelDelayThread(delay);
					drawToast(&displayCanvas);
				} else {
					sceKernelDelayThread(10000);
				}
				wait = 0;
			} else {
				sceKernelDelayThread(10000);
			}
		} else {
			sceKernelDelayThread(10000);
		}
	}
	sceKernelExitDeleteThread(0);
	return 0;
}


void setConfigPath(const char *argp)
{
	strcpy(configPath, argp);
	strrchr(configPath, '/')[1] = 0;
	strcpy(configPath + strlen(configPath), BRIGHTNESS_CONFIG_FILE);
}

 
int module_start(SceSize args, void *argp)
{
	nkLoad();
	setConfigPath(argp);
	SceUID thid = sceKernelCreateThread("main_thread", main_thread, 1, 0x8000, 0, NULL);
	if(thid >= 0) sceKernelStartThread(thid, args, argp);
	return 0;
}


int module_stop(SceSize args, void *argp)
{
	uninstall_syscon_hook();
	return 0;
}
