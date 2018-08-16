#pragma once
#include "tcl_enum.h"
#include <libcpu/be2_struct.h>

namespace cafe::tcl
{

using TCLTimestamp = int64_t;

TCLStatus
TCLReadTimestamp(TCLTimestampID id,
                 virt_ptr<TCLTimestamp> outValue);

TCLStatus
TCLSubmit(phys_ptr<void> buffer,
          uint32_t bufferSize,
          virt_ptr<uint32_t> unkFlags,
          virt_ptr<TCLTimestamp> outLastSubmittedTimestamp);

TCLStatus
TCLSubmitToRing(phys_ptr<void> buffer,
                uint32_t numWords,
                virt_ptr<uint32_t> unkFlags,
                virt_ptr<TCLTimestamp> outLastSubmittedTimestamp);

} // namespace cafe::tcl
