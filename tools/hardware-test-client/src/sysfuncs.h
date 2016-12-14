#ifndef SYSFUNCS_H
#define SYSFUNCS_H
#include "../../../libwiiu/src/types.h"
#include "../../../libwiiu/src/vpad.h"
#include "../../../libwiiu/src/socket.h"

struct SystemFunctions
{
   unsigned int coreinit_handle;
   void *(*OSAllocFromSystem)(uint32_t size, int align);
   void (*OSFreeToSystem)(void *ptr);
   void *(*OSEffectiveToPhysical)(const void *);
   void (*ICInvalidateRange)(const void *, int);
   void (*DCFlushRange)(const void *, int);
   void (*PPCMtpmc4)();
   void (*memset)(void *dst, char val, int bytes);
   void (*_Exit)();

   unsigned int vpad_handle;
   int(*VPADRead)(int controller, VPADData *buffer, unsigned int num, int *error);

   unsigned int nsysnet_handle;
   void (*socket_lib_init)();
   void (*socket_lib_finish)();
   int (*socket)(int family, int type, int proto);
   int (*connect)(int fd, struct sockaddr *addr, int addrlen);
   int (*recv)(int fd, void *buffer, int len, int flags);
   int (*send)(int fd, const void *buffer, int len, int flags);
};

void loadSysFuncs(struct SystemFunctions *sysFuncs);

#endif
