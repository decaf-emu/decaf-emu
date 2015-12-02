#pragma once
#include "types.h"
#include "coreinit_enum.h"
#include "coreinit_time.h"
#include "coreinit_threadqueue.h"
#include "utils/be_val.h"
#include "utils/structsize.h"
#include "utils/virtual_ptr.h"

#pragma pack(push, 1)

struct OSEvent
{
   static const uint32_t Tag = 0x65566E54;

   be_val<uint32_t> tag;
   be_ptr<const char> name;
   UNKNOWN(4);
   be_val<uint32_t> value;
   OSThreadQueue queue;
   be_val<OSEventMode> mode;
};
CHECK_OFFSET(OSEvent, 0x0, tag);
CHECK_OFFSET(OSEvent, 0x4, name);
CHECK_OFFSET(OSEvent, 0xc, value);
CHECK_OFFSET(OSEvent, 0x10, queue);
CHECK_OFFSET(OSEvent, 0x20, mode);
CHECK_SIZE(OSEvent, 0x24);

#pragma pack(pop)

void
OSInitEvent(OSEvent *event, bool value, OSEventMode mode);

void
OSInitEventEx(OSEvent *event, bool value, OSEventMode mode, char *name);

void
OSSignalEvent(OSEvent *event);

void
OSSignalEventAll(OSEvent *event);

void
OSWaitEvent(OSEvent *event);

void
OSResetEvent(OSEvent *event);

BOOL
OSWaitEventWithTimeout(OSEvent *event, OSTime timeout);
