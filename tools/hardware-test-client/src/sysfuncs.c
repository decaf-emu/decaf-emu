#include "sysfuncs.h"
#include "../../../libwiiu/src/coreinit.h"

void loadSysFuncs(struct SystemFunctions *sysFuncs)
{
   unsigned coreinit, vpad, nsysnet;

   OSDynLoad_Acquire("coreinit.rpl", &coreinit);
   OSDynLoad_FindExport(coreinit, 0, "memset", &sysFuncs->memset);
   OSDynLoad_FindExport(coreinit, 0, "_Exit", &sysFuncs->_Exit);
   OSDynLoad_FindExport(coreinit, 0, "ICInvalidateRange", &sysFuncs->ICInvalidateRange);
   OSDynLoad_FindExport(coreinit, 0, "DCFlushRange", &sysFuncs->DCFlushRange);
   OSDynLoad_FindExport(coreinit, 0, "OSAllocFromSystem", &sysFuncs->OSAllocFromSystem);
   OSDynLoad_FindExport(coreinit, 0, "OSFreeToSystem", &sysFuncs->OSFreeToSystem);
   OSDynLoad_FindExport(coreinit, 0, "OSEffectiveToPhysical", &sysFuncs->OSEffectiveToPhysical);
   OSDynLoad_FindExport(coreinit, 0, "PPCMtpmc4", &sysFuncs->PPCMtpmc4);
   sysFuncs->coreinit_handle = coreinit;

   OSDynLoad_Acquire("vpad.rpl", &vpad);
   OSDynLoad_FindExport(vpad, 0, "VPADRead", &sysFuncs->VPADRead);
   sysFuncs->vpad_handle = vpad;

   OSDynLoad_Acquire("nsysnet.rpl", &nsysnet);
   OSDynLoad_FindExport(nsysnet, 0, "socket_lib_init", &sysFuncs->socket_lib_init);
   OSDynLoad_FindExport(nsysnet, 0, "socket_lib_finish", &sysFuncs->socket_lib_finish);
   OSDynLoad_FindExport(nsysnet, 0, "socket", &sysFuncs->socket);
   OSDynLoad_FindExport(nsysnet, 0, "connect", &sysFuncs->connect);
   OSDynLoad_FindExport(nsysnet, 0, "recv", &sysFuncs->recv);
   OSDynLoad_FindExport(nsysnet, 0, "send", &sysFuncs->send);
   sysFuncs->nsysnet_handle = nsysnet;
}
