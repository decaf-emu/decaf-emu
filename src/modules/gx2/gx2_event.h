#pragma once
#include "modules/coreinit/coreinit_time.h"
#include "modules/gx2/gx2_enum.h"
#include "utils/be_val.h"
#include "utils/structsize.h"
#include "utils/wfunc_ptr.h"

struct OSAlarm;
struct OSContext;

#pragma pack(push, 1)

struct GX2DisplayListOverrunData
{
   be_ptr<void> oldList;
   be_val<uint32_t> oldSize;
   be_ptr<void> newList;
   be_val<uint32_t> newSize;
   UNKNOWN(8);
};
CHECK_OFFSET(GX2DisplayListOverrunData, 0x00, oldList);
CHECK_OFFSET(GX2DisplayListOverrunData, 0x04, oldSize);
CHECK_OFFSET(GX2DisplayListOverrunData, 0x08, newList);
CHECK_OFFSET(GX2DisplayListOverrunData, 0x0C, newSize);
CHECK_SIZE(GX2DisplayListOverrunData, 0x18);

#pragma pack(pop)

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

std::pair<void *, uint32_t>
displayListOverrun(void *list, uint32_t size);

void
setLastSubmittedTimestamp(OSTime timestamp);

void
setRetiredTimestamp(OSTime timestamp);

} // namespace internal

} // namespace gx2
