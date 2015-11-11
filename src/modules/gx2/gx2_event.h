#pragma once
#include "modules/coreinit/coreinit_time.h"
#include "modules/gx2/gx2_enum.h"
#include "utils/wfunc_ptr.h"

struct OSAlarm;
struct OSContext;

using GX2EventCallbackFunction = wfunc_ptr<void, GX2EventType::Value, void *>;
using be_GX2EventCallbackFunction = be_wfunc_ptr<void, GX2EventType::Value, void *>;

BOOL
GX2DrawDone();

void
GX2WaitForVsync();

void
GX2WaitForFlip();

void
GX2SetEventCallback(GX2EventType::Value type, GX2EventCallbackFunction func, void *userData);

void
GX2GetEventCallback(GX2EventType::Value type, be_GX2EventCallbackFunction *funcOut, be_ptr<void> *userDataOut);

OSTime
GX2GetRetiredTimeStamp();

OSTime
GX2GetLastSubmittedTimeStamp();

BOOL
GX2WaitTimeStamp(OSTime time);

namespace gx2
{

namespace internal
{

void
initVsync();

void
vsyncAlarmHandler(OSAlarm *alarm, OSContext *context);

void
setLastSubmittedTimestamp(OSTime timestamp);

void
setRetiredTimestamp(OSTime timestamp);

} // namespace internal

} // namespace gx2
