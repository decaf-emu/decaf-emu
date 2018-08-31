#include "sndcore2.h"
#include "sndcore2_ai.h"
#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::sndcore2
{

uint32_t
AIGetDMALength()
{
   decaf_warn_stub();
   return 0;
}

uint32_t
AIGetDMAStartAddr()
{
   decaf_warn_stub();
   return 0;
}

uint32_t
AI2GetDMALength()
{
   decaf_warn_stub();
   return 0;
}

uint32_t
AI2GetDMAStartAddr()
{
   decaf_warn_stub();
   return 0;
}

void
Library::registerAiSymbols()
{
   RegisterFunctionExport(AIGetDMALength);
   RegisterFunctionExport(AIGetDMAStartAddr);
   RegisterFunctionExport(AI2GetDMALength);
   RegisterFunctionExport(AI2GetDMAStartAddr);
}

} // namespace cafe::sndcore2
