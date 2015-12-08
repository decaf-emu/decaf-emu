#include "loader.h"

void _start()
{
	/****************************>            Fix Stack            <****************************/
	//Load a good stack
	asm(
		"lis %r1, 0x1ab5 ;"
		"ori %r1, %r1, 0xd138 ;"
		);
	/****************************>           Get Handles           <****************************/
	//Get a handle to coreinit.rpl
	unsigned int coreinit_handle;
	OSDynLoad_Acquire("coreinit.rpl", &coreinit_handle);
	/****************************>       External Prototypes       <****************************/
	//OSScreen functions
	void(*OSScreenInit)();
	unsigned int(*OSScreenGetBufferSizeEx)(unsigned int bufferNum);
	unsigned int(*OSScreenSetBufferEx)(unsigned int bufferNum, void * addr);
	//OS Memory functions
	void*(*memset)(void * dest, uint32_t value, uint32_t bytes);
	void*(*OSAllocFromSystem)(uint32_t size, int align);
	void(*OSFreeToSystem)(void *ptr);
	//IM functions
	int(*IM_Open)();
	int(*IM_Close)(int fd);
	int(*IM_SetDeviceState)(int fd, void *mem, int state, int a, int b);
	/****************************>             Exports             <****************************/
	//OSScreen functions
	OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenInit", &OSScreenInit);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenGetBufferSizeEx", &OSScreenGetBufferSizeEx);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenSetBufferEx", &OSScreenSetBufferEx);
	//OS Memory functions
	OSDynLoad_FindExport(coreinit_handle, 0, "memset", &memset);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSAllocFromSystem", &OSAllocFromSystem);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSFreeToSystem", &OSFreeToSystem);
	//IM functions
	OSDynLoad_FindExport(coreinit_handle, 0, "IM_Open", &IM_Open);
	OSDynLoad_FindExport(coreinit_handle, 0, "IM_Close", &IM_Close);
	OSDynLoad_FindExport(coreinit_handle, 0, "IM_SetDeviceState", &IM_SetDeviceState);
	/****************************>          Initial Setup          <****************************/
	//Restart system to get lib access
	int fd = IM_Open();
	void *mem = OSAllocFromSystem(0x100, 64);
	memset(mem, 0, 0x100);
	//set restart flag to force quit browser
	IM_SetDeviceState(fd, mem, 3, 0, 0); 
	IM_Close(fd);
	OSFreeToSystem(mem);
	//wait a bit for browser end
	unsigned int t1 = 0x1FFFFFFF;
	while(t1--) ;
	//Call the Screen initilzation function.
	OSScreenInit();
	//Grab the buffer size for each screen (TV and gamepad)
	int buf0_size = OSScreenGetBufferSizeEx(0);
	int buf1_size = OSScreenGetBufferSizeEx(1);
	//Set the buffer area.
	OSScreenSetBufferEx(0, (void *)0xF4000000);
	OSScreenSetBufferEx(1, (void *)0xF4000000 + buf0_size);
	//Clear both framebuffers.
	int ii = 0;
	for (ii; ii < 2; ii++)
	{
		fillScreen(0,0,0,0);
		flipBuffers();
	}
	//Jump to entry point.
	_entryPoint();
}