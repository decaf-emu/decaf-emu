#pragma once
#include "types.h"
#include "coreinit_enum.h"
#include "coreinit_time.h"
#include "coreinit_threadqueue.h"
#include "utils/be_val.h"
#include "utils/structsize.h"
#include "utils/virtual_ptr.h"

namespace coreinit
{

/**
 * \defgroup coreinit_event Event Object
 * \ingroup coreinit
 *
 * Standard event object implementation. There are two supported event object modes, check OSEventMode.
 *
 * Similar to Windows <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/ms682655(v=vs.85).aspx">Event Objects</a>.
 * @{
 */

#pragma pack(push, 1)

struct OSEvent
{
   static const uint32_t Tag = 0x65566E54;

   //! Should always be set to the value OSEvent::Tag.
   be_val<uint32_t> tag;

   //! Name set by OSInitEventEx.
   be_ptr<const char> name;

   UNKNOWN(4);

   //! The current value of the event object.
   be_val<uint32_t> value;

   //! The threads currently waiting on this event object with OSWaitEvent.
   OSThreadQueue queue;

   //! The mode of the event object, set by OSInitEvent.
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

/** @} */

namespace internal
{

void signalEventNoLock(OSEvent *event);

}

} // namespace coreinit
