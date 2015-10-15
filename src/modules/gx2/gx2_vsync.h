#pragma once
#include "modules/coreinit/coreinit_time.h"
#include "wfunc_ptr.h"

extern OSTime
gLastVsync;

namespace GX2EventCallback
{
enum Type
{
   Vsync = 2,
   Flip = 3,
   Max = 5
};
}

using GX2EventCallbackFunction = wfunc_ptr<void, GX2EventCallback::Type, void *>;
using be_GX2EventCallbackFunction = be_wfunc_ptr<void, GX2EventCallback::Type, void *>;

void
_GX2InitVsync();

void
GX2WaitForVsync();

void
GX2SetEventCallback(GX2EventCallback::Type type, GX2EventCallbackFunction func, void *userData);

void
GX2GetEventCallback(GX2EventCallback::Type type, be_GX2EventCallbackFunction *funcOut, be_ptr<void> *userDataOut);
