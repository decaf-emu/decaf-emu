#include "tcl.h"
#include "tcl_ring.h"

namespace cafe::tcl
{

TCLStatus
TCLReadTimestamp(TCLTimestampID id,
                 virt_ptr<TCLTimestamp> outValue)
{
   return TCLStatus::NotInitialised;
}

TCLStatus
TCLSubmit(phys_ptr<void> buffer,
          uint32_t bufferSize,
          virt_ptr<uint32_t> unkFlags,
          virt_ptr<TCLTimestamp> lastSubmittedTimestamp)
{
   return TCLStatus::NotInitialised;
}

TCLStatus
TCLSubmitToRing(phys_ptr<void> buffer,
                uint32_t numWords,
                virt_ptr<uint32_t> unkFlags,
                virt_ptr<TCLTimestamp> lastSubmittedTimestamp)
{
   return TCLStatus::NotInitialised;
}

void
Library::registerRingSymbols()
{
   RegisterFunctionExport(TCLReadTimestamp);
   RegisterFunctionExport(TCLSubmit);
   RegisterFunctionExport(TCLSubmitToRing);
}

} // namespace cafe::tcl
