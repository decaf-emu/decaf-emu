#pragma once
#include "coreinit_enum.h"
#include "coreinit_time.h"
#include "coreinit_thread.h"

#include <libcpu/be2_struct.h>

namespace cafe::coreinit
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
   static constexpr uint32_t Tag = 0x65566E54;

   //! Should always be set to the value OSEvent::Tag.
   be2_val<uint32_t> tag;

   //! Name set by OSInitEventEx.
   be2_ptr<const char> name;

   UNKNOWN(4);

   //! The current value of the event object.
   be2_val<BOOL> value;

   //! The threads currently waiting on this event object with OSWaitEvent.
   be2_struct<OSThreadQueue> queue;

   //! The mode of the event object, set by OSInitEvent.
   be2_val<OSEventMode> mode;
};
CHECK_OFFSET(OSEvent, 0x0, tag);
CHECK_OFFSET(OSEvent, 0x4, name);
CHECK_OFFSET(OSEvent, 0xC, value);
CHECK_OFFSET(OSEvent, 0x10, queue);
CHECK_OFFSET(OSEvent, 0x20, mode);
CHECK_SIZE(OSEvent, 0x24);

#pragma pack(pop)

void
OSInitEvent(virt_ptr<OSEvent> event,
            BOOL value,
            OSEventMode mode);

void
OSInitEventEx(virt_ptr<OSEvent> event,
              BOOL value,
              OSEventMode mode,
              virt_ptr<const char> name);

void
OSSignalEvent(virt_ptr<OSEvent> event);

void
OSSignalEventAll(virt_ptr<OSEvent> event);

void
OSWaitEvent(virt_ptr<OSEvent> event);

void
OSResetEvent(virt_ptr<OSEvent> event);

BOOL
OSWaitEventWithTimeout(virt_ptr<OSEvent> event,
                       OSTimeNanoseconds timeout);

/** @} */

} // namespace cafe::coreinit
