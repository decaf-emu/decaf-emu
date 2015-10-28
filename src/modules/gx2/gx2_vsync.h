#pragma once
#include "modules/coreinit/coreinit_time.h"
#include "modules/gx2/gx2_enum.h"
#include "utils/wfunc_ptr.h"

extern OSTime
gLastVsync;

using GX2EventCallbackFunction = wfunc_ptr<void, GX2EventType::Value, void *>;
using be_GX2EventCallbackFunction = be_wfunc_ptr<void, GX2EventType::Value, void *>;

void
_GX2InitVsync();

void
GX2WaitForVsync();

void
GX2SetEventCallback(GX2EventType::Value type, GX2EventCallbackFunction func, void *userData);

void
GX2GetEventCallback(GX2EventType::Value type, be_GX2EventCallbackFunction *funcOut, be_ptr<void> *userDataOut);
