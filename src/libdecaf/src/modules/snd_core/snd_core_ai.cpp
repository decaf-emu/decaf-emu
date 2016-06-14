#include "snd_core.h"
#include "snd_core_ai.h"

namespace snd_core
{

uint32_t
AIGetDMALength()
{
   return 0;
}

uint32_t
AIGetDMAStartAddr()
{
   return 0;
}

uint32_t
AI2GetDMALength()
{
   return 0;
}

uint32_t
AI2GetDMAStartAddr()
{
   return 0;
}

void
Module::registerAiFunctions()
{
   RegisterKernelFunction(AIGetDMALength);
   RegisterKernelFunction(AIGetDMAStartAddr);
   RegisterKernelFunction(AI2GetDMALength);
   RegisterKernelFunction(AI2GetDMAStartAddr);
}

} // namespace snd_core
