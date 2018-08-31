#pragma once
#include "cafe/libraries/coreinit/coreinit_alarm.h"
#include "cafe/libraries/coreinit/coreinit_time.h"
#include "gx2_enum.h"

#include <libcpu/be2_struct.h>
#include <utility>

namespace cafe::gx2
{

/**
 * \defgroup gx2_event Event
 * \ingroup gx2
 * @{
 */

#pragma pack(push, 1)

using GX2Timestamp = coreinit::OSTime;

struct GX2DisplayListOverrunData
{
   be2_virt_ptr<void> oldList;
   be2_val<uint32_t> oldSize;
   be2_virt_ptr<void> newList;
   be2_val<uint32_t> newSize;
   UNKNOWN(8);
};
CHECK_OFFSET(GX2DisplayListOverrunData, 0x00, oldList);
CHECK_OFFSET(GX2DisplayListOverrunData, 0x04, oldSize);
CHECK_OFFSET(GX2DisplayListOverrunData, 0x08, newList);
CHECK_OFFSET(GX2DisplayListOverrunData, 0x0C, newSize);
CHECK_SIZE(GX2DisplayListOverrunData, 0x18);

#pragma pack(pop)

using GX2EventCallbackFunction = virt_func_ptr<void(GX2EventType, virt_ptr<void>)>;

BOOL
GX2DrawDone();

void
GX2WaitForVsync();

void
GX2WaitForFlip();

void
GX2SetEventCallback(GX2EventType type,
                    GX2EventCallbackFunction func,
                    virt_ptr<void> userData);

void
GX2GetEventCallback(GX2EventType type,
                    virt_ptr<GX2EventCallbackFunction> outFunc,
                    virt_ptr<virt_ptr<void>> outUserData);

GX2Timestamp
GX2GetRetiredTimeStamp();

GX2Timestamp
GX2GetLastSubmittedTimeStamp();

BOOL
GX2WaitTimeStamp(GX2Timestamp time);

void
GX2GetSwapStatus(virt_ptr<uint32_t> outSwapCount,
                 virt_ptr<uint32_t> outFlipCount,
                 virt_ptr<GX2Timestamp> outLastFlip,
                 virt_ptr<GX2Timestamp> outLastVsync);

namespace internal
{

void
initEvents();

std::pair<virt_ptr<void>, uint32_t>
displayListOverrun(virt_ptr<void> list,
                   uint32_t size,
                   uint32_t neededSize);

void
setLastSubmittedTimestamp(GX2Timestamp timestamp);

void
handleGpuRetireInterrupt();

void
setRetiredTimestamp(GX2Timestamp timestamp);

void
onSwap();

void
handleGpuFlipInterrupt();

void
onFlip();

} // namespace internal

/** @} */

} // namespace cafe::gx2
