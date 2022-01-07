/*
** main.cpp
**
** This file is part of mkxp.
**
** Copyright (C) 2013 - 2021 Amaryllis Kulla <ancurio@mapleshrine.eu>
**
** mkxp is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 2 of the License, or
** (at your option) any later version.
**
** mkxp is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with mkxp.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <alc.h>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_sound.h>

#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <string>

#include "sharedstate.h"
#include "eventthread.h"
#include "gl-debug.h"
#include "debugwriter.h"
#include "exception.h"
#include "gl-fun.h"


#include "binding.h"

#ifdef __WINDOWS__
#include "resource.h"
#endif

#include "icon.png.xxd"

#ifdef __vita__
#include <taihen.h>
#include <psp2/power.h>
#include <psp2/appmgr.h>
#include <psp2/vshbridge.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/processmgr.h> 
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/modulemgr.h>
extern "C"{
#include <gpu_es4/psp2_pvr_hint.h>
}
#include <stdlib.h>
#include <string.h>

#include "vita/fios2.h" // vitasdk headers are broken for userland fios2.. so uh use mine!

#define DO_HARDWARE_TRANSFERS 1

//#define NEWLIB_HEAP_SIZE 209715200
#define NEWLIB_HEAP_SIZE 157286400
#define LIBC_HEAP_SIZE 41943040
#define RGSS_STACK_SIZE 5242880

#define GPU_MEM_SIZE 16777216

#if (NEWLIB_HEAP_SIZE + LIBC_HEAP_SIZE + RGSS_STACK_SIZE) > 382730240
#error Memory usage, exceeds maximum memory for userland applications.
#endif

#if (GPU_MEM_SIZE) > 134217728
#error GPU Memory exceeds maximum memblck size
#endif

// must be less than 365MB total.

int _newlib_heap_size_user = NEWLIB_HEAP_SIZE;
unsigned int sceLibcHeapSize = LIBC_HEAP_SIZE;
unsigned int sceLibcHeapExtendedAlloc = 1;

#endif

static void
rgssThreadError(RGSSThreadData *rtData, const std::string &msg)
{
#ifdef __vita__
	rtData->ethread->showMessageBox(msg.c_str(), 0x1000);
#else
	rtData->rgssErrorMsg = msg;
	rtData->ethread->requestTerminate();
	rtData->rqTermAck.set();
#endif
}

static inline const char*
glGetStringInt(GLenum name)
{
	return (const char*) gl.GetString(name);
}

static void
printGLInfo()
{
	Debug() << "GL Vendor    :" << glGetStringInt(GL_VENDOR);
	Debug() << "GL Renderer  :" << glGetStringInt(GL_RENDERER);
	Debug() << "GL Version   :" << glGetStringInt(GL_VERSION);
	Debug() << "GLSL Version :" << glGetStringInt(GL_SHADING_LANGUAGE_VERSION);
}

int rgssThreadFun(void *userdata)
{
	RGSSThreadData *threadData = static_cast<RGSSThreadData*>(userdata);
	const Config &conf = threadData->config;
	SDL_Window *win = threadData->window;
	SDL_GLContext glCtx;
	
#ifdef __vita__
	SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);
#endif
	/* Setup GL context */
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	if (conf.debugMode)
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

	glCtx = SDL_GL_CreateContext(win);

	if (!glCtx)
	{
		rgssThreadError(threadData, std::string("Error creating context: ") + SDL_GetError());
		return 0;
	}

	try
	{
		initGLFunctions();
	}
	catch (const Exception &exc)
	{
		rgssThreadError(threadData, exc.msg);
		SDL_GL_DeleteContext(glCtx);

		return 0;
	}

	if (!conf.enableBlitting)
		gl.BlitFramebuffer = 0;

	gl.ClearColor(0, 0, 0, 1);
	gl.Clear(GL_COLOR_BUFFER_BIT);
	SDL_GL_SwapWindow(win);

	printGLInfo();

	bool vsync = conf.vsync || conf.syncToRefreshrate;
	SDL_GL_SetSwapInterval(vsync ? 1 : 0);

	GLDebugLogger dLogger;

	/* Setup AL context */
	ALCcontext *alcCtx = alcCreateContext(threadData->alcDev, 0);

	if (!alcCtx)
	{
		rgssThreadError(threadData, "Error creating OpenAL context");
		SDL_GL_DeleteContext(glCtx);

		return 0;
	}

	alcMakeContextCurrent(alcCtx);

	try
	{
		SharedState::initInstance(threadData);
	}
	catch (const Exception &exc)
	{
		rgssThreadError(threadData, exc.msg);
		alcDestroyContext(alcCtx);
		SDL_GL_DeleteContext(glCtx);

		return 0;
	}

	/* Start script execution */
	scriptBinding->execute();

	threadData->rqTermAck.set();
	threadData->ethread->requestTerminate();

	SharedState::finiInstance();

	alcDestroyContext(alcCtx);
	SDL_GL_DeleteContext(glCtx);

	return 0;
}

static void printRgssVersion(int ver)
{
	const char *const makers[] =
		{ "", "XP", "VX", "VX Ace" };

	char buf[128];
	snprintf(buf, sizeof(buf), "RGSS version %d (%s)", ver, makers[ver]);

	Debug() << buf;
}

static void showInitError(const std::string &msg)
{
	Debug() << msg;
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "mkxp", msg.c_str(), 0);
}

static void setupWindowIcon(const Config &conf, SDL_Window *win)
{
	SDL_RWops *iconSrc;

	if (conf.iconPath.empty())
		iconSrc = SDL_RWFromConstMem(assets_icon_png, assets_icon_png_len);
	else
		iconSrc = SDL_RWFromFile(conf.iconPath.c_str(), "rb");

	SDL_Surface *iconImg = IMG_Load_RW(iconSrc, SDL_TRUE);

	if (iconImg)
	{
		SDL_SetWindowIcon(win, iconImg);
		SDL_FreeSurface(iconImg);
	}
}

int main(int argc, char *argv[])
{

#ifdef __vita__
	scePowerSetArmClockFrequency(444);
        scePowerSetBusClockFrequency(222);
	scePowerSetGpuClockFrequency(222);
        scePowerSetGpuXbarClockFrequency(166);
        
        int ret = 0;
        
        
        // Load Kernel Module
	char titleid[12];        
	char kplugin_path[0x1000];
        sceAppMgrAppParamGetString(0, 12, titleid , 256);
        snprintf(kplugin_path, 0x1000, "ux0:app/%s/module/gpu_fix.skprx", titleid);
  
   	
   	Debug() << "Looking for gpu_fix...";
        
   	int64_t unk;
        SceUID gpuFixId = _vshKernelSearchModuleByName("gpu_fix", &unk);
        
        if(gpuFixId < 0){ // if gpu_fix not loaded...
        	// Load gpu_fix...
        	Debug() << "gpu_fix not found, loading!";
		ret = taiLoadStartKernelModule(kplugin_path, 0, NULL, 0);
		if(ret < 0){
			Debug() << "Failed to load Kernel Module.";
			sceKernelExitProcess(1);
		}
        }
        else{
        	Debug() << "gpu_fix already loaded.";
        }
       
        // Create data folders. 
        sceIoMkdir("ux0:/data", 0777);
        sceIoMkdir("ux0:/data/mkxp", 0777);
        
        sceIoMkdir("ux0:/data/mkxp/xp-rtp", 0777);
        sceIoMkdir("ux0:/data/mkxp/vx-rtp", 0777);
        sceIoMkdir("ux0:/data/mkxp/vxa-rtp", 0777);
        
        sceIoMkdir("ux0:/data/mkxp/shader-cache", 0777);


        // Create the Read/Writable app0: overlay
	SceUID pid = sceKernelGetProcessId();
	SceFiosKernelOverlay ov;
	SceFiosKernelOverlayID ovId;
	
	memset(&ov, 0, sizeof(SceFiosKernelOverlay));
	
	ov.type = SCE_FIOS_OVERLAY_TYPE_WRITABLE;
	ov.order = SCE_FIOS_OVERLAY_ORDER_USER_FIRST;
	ov.pid = pid;
	strncpy(ov.src, "savedata0:", sizeof(ov.src));
	strncpy(ov.dst, "app0:", sizeof(ov.dst));
	ov.src_len = strnlen(ov.src, sizeof(ov.src));
	ov.dst_len = strnlen(ov.dst, sizeof(ov.dst));

	ret = sceFiosKernelOverlayAddForProcess02(pid, &ov, &ovId);
	if(ret < 0){
        	Debug() << "Failed to create fios2 overlay";
        	sceKernelExitProcess(1);
        }
        
#endif

	SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
	SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");
#ifdef __vita__
	SDL_setenv("VITA_PVR_SKIP_INIT", "enable", 1);
	PVRSRV_PSP2_APPHINT hint;
        
        sceKernelLoadStartModule("vs0:sys/external/libfios2.suprx", 0, NULL, 0, NULL, NULL);
        sceKernelLoadStartModule("vs0:sys/external/libc.suprx", 0, NULL, 0, NULL, NULL);
        sceKernelLoadStartModule("app0:/module/libgpu_es4_ext.suprx", 0, NULL, 0, NULL, NULL);
        sceKernelLoadStartModule("app0:/module/libIMGEGL.suprx", 0, NULL, 0, NULL, NULL);
        PVRSRVInitializeAppHint(&hint);
        
        #if DO_HARDWARE_TRANSFERS == 0
        hint.bDisableHWTextureUpload = 1;
        hint.bDisableHWTQBufferBlit = 1;
        hint.bDisableHWTQMipGen = 1;
        hint.bDisableHWTQNormalBlit = 1;
        hint.bDisableHWTQTextureUpload = 1;
        #endif
        
        hint.ui32DriverMemorySize = GPU_MEM_SIZE;
        PVRSRVCreateVirtualAppHint(&hint);
#endif
	/* initialize SDL first */
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0)
	{
		showInitError(std::string("Error initializing SDL: ") + SDL_GetError());
		return 0;
	}

	if (!EventThread::allocUserEvents())
	{
		showInitError("Error allocating SDL user events");
		return 0;
	}

#ifndef WORKDIR_CURRENT
	/* set working directory */
	char *dataDir = SDL_GetBasePath();
	if (dataDir)
	{
		int result = chdir(dataDir);
		(void)result;
		SDL_free(dataDir);
	}
#endif

	/* now we load the config */
	Config conf;
	conf.read(argc, argv);

	if (!conf.gameFolder.empty())
		if (chdir(conf.gameFolder.c_str()) != 0)
		{
			showInitError(std::string("Unable to switch into gameFolder ") + conf.gameFolder);
			return 0;
		}

	conf.readGameINI();

	if (conf.windowTitle.empty())
		conf.windowTitle = conf.game.title;

	assert(conf.rgssVersion >= 1 && conf.rgssVersion <= 3);
	printRgssVersion(conf.rgssVersion);

	int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
	if (IMG_Init(imgFlags) != imgFlags)
	{
		showInitError(std::string("Error initializing SDL_image: ") + SDL_GetError());
		SDL_Quit();

		return 0;
	}

	if (TTF_Init() < 0)
	{
		showInitError(std::string("Error initializing SDL_ttf: ") + SDL_GetError());
		IMG_Quit();
		SDL_Quit();

		return 0;
	}

	if (Sound_Init() == 0)
	{
		showInitError(std::string("Error initializing SDL_sound: ") + Sound_GetError());
		TTF_Quit();
		IMG_Quit();
		SDL_Quit();

		return 0;
	}

	SDL_Window *win;
	Uint32 winFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_INPUT_FOCUS;

	if (conf.winResizable)
		winFlags |= SDL_WINDOW_RESIZABLE;
	if (conf.fullscreen)
		winFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

	win = SDL_CreateWindow(conf.windowTitle.c_str(),
	                       SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
	                       conf.defScreenW, conf.defScreenH, winFlags);

	if (!win)
	{
		showInitError(std::string("Error creating window: ") + SDL_GetError());
		return 0;
	}

	/* OSX and Windows have their own native ways of
	 * dealing with icons; don't interfere with them */
#ifdef __LINUX__
	setupWindowIcon(conf, win);
#else
	(void) setupWindowIcon;
#endif

	ALCdevice *alcDev = alcOpenDevice(0);

	if (!alcDev)
	{
		showInitError("Error opening OpenAL device");
		SDL_DestroyWindow(win);
		TTF_Quit();
		IMG_Quit();
		SDL_Quit();

		return 0;
	}

	SDL_DisplayMode mode;
	SDL_GetDisplayMode(0, 0, &mode);

	/* Can't sync to display refresh rate if its value is unknown */
	if (!mode.refresh_rate)
		conf.syncToRefreshrate = false;

	EventThread eventThread;
	RGSSThreadData rtData(&eventThread, argv[0], win,
	                      alcDev, mode.refresh_rate, conf);

	int winW, winH;
	SDL_GetWindowSize(win, &winW, &winH);
	rtData.windowSizeMsg.post(Vec2i(winW, winH));

	/* Load and post key bindings */
	rtData.bindingUpdateMsg.post(loadBindings(conf));
	
	/* Start RGSS thread */
#ifndef __vita__
	SDL_Thread *rgssThread =
	        SDL_CreateThread(rgssThreadFun, "rgss", &rtData);
#else
	SDL_Thread *rgssThread =
		SDL_CreateThreadWithStackSize(rgssThreadFun, "rgss", RGSS_STACK_SIZE,  &rtData);
#endif
	/* Start event processing */
	eventThread.process(rtData);

	/* Request RGSS thread to stop */
	rtData.rqTerm.set();


	/* Wait for RGSS thread response */
	for (int i = 0; i < 1000; ++i)
	{
		/* We can stop waiting when the request was ack'd */
		if (rtData.rqTermAck)
		{
			Debug() << "RGSS thread ack'd request after" << i*10 << "ms";
			break;
		}

		/* Give RGSS thread some time to respond */
		SDL_Delay(10);
	}

	/* If RGSS thread ack'd request, wait for it to shutdown,
	 * otherwise abandon hope and just end the process as is. */
	if (rtData.rqTermAck)
		SDL_WaitThread(rgssThread, 0);
	else
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, conf.windowTitle.c_str(),
		                         "The RGSS script seems to be stuck and mkxp will now force quit", win);
	

	if (!rtData.rgssErrorMsg.empty())
	{
		Debug() << rtData.rgssErrorMsg;
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, conf.windowTitle.c_str(),
		                         rtData.rgssErrorMsg.c_str(), win);
	}

	/* Clean up any remainin events */
	eventThread.cleanup();

	Debug() << "Shutting down.";

	alcCloseDevice(alcDev);
	SDL_DestroyWindow(win);

	Sound_Quit();
	TTF_Quit();
	IMG_Quit();
	SDL_Quit();

	return 0;
}
