#pragma once
#include "tcl_enum.h"

#include "cafe/libraries/coreinit/coreinit_time.h"
#include <libcpu/be2_struct.h>

namespace cafe::tcl
{

using TCLTimestamp = uint64_t;

TCLStatus
TCLReadTimestamp(TCLTimestampID id,
                 virt_ptr<TCLTimestamp> outValue);

TCLStatus
TCLWaitTimestamp(TCLTimestampID id,
                 TCLTimestamp timestamp,
                 coreinit::OSTime timeout);

TCLStatus
TCLSubmit(phys_ptr<void> buffer,
          uint32_t bufferSize,
          virt_ptr<TCLSubmitFlags> submitFlags,
          virt_ptr<TCLTimestamp> outLastSubmittedTimestamp);

TCLStatus
TCLSubmitToRing(virt_ptr<uint32_t> buffer,
                uint32_t numWords,
                virt_ptr<TCLSubmitFlags> submitFlags,
                virt_ptr<TCLTimestamp> outLastSubmittedTimestamp);

namespace internal
{

void
initialiseRing();

} // namespace internal

} // namespace cafe::tcl
