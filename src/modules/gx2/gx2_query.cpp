#include "gx2_query.h"
#include "gpu/pm4_writer.h"

namespace gx2
{

static uint32_t
gGpuTimeout = 10000;

void
GX2SampleTopGPUCycle(be_val<uint64_t> *result)
{
   *result = -1;

   auto addrLo = pm4::MW_ADDR_LO { 0 };
   addrLo.ADDR_LO = memory_untranslate(result) >> 2;
   addrLo.ENDIAN_SWAP = latte::CB_ENDIAN_8IN64;

   auto addrHi = pm4::MW_ADDR_HI { 0 };
   addrHi.CNTR_SEL = pm4::MW_WRITE_CLOCK;

   pm4::write(pm4::MemWrite { addrLo, addrHi, 0, 0 });
}

void
GX2SampleBottomGPUCycle(be_val<uint64_t> *result)
{
   *result = -1;

   auto eventInitiator = latte::VGT_EVENT_INITIATOR::get(0)
      .EVENT_TYPE().set(latte::VGT_EVENT_TYPE::BOTTOM_OF_PIPE_TS);

   auto addrLo = pm4::EW_EOP_ADDR_LO { 0 };
   addrLo.ADDR_LO = memory_untranslate(result) >> 2;
   addrLo.ENDIAN_SWAP = latte::CB_ENDIAN_8IN64;

   auto addrHi = pm4::EW_EOP_ADDR_HI { 0 };
   addrHi.DATA_SEL = pm4::EW_DATA_CLOCK;

   pm4::write(pm4::EventWriteEOP { eventInitiator, addrLo, addrHi, 0, 0 });
}

uint64_t
GX2GPUTimeToCPUTime(uint64_t time)
{
   return time;
}

uint32_t
GX2GetGPUTimeout()
{
   return gGpuTimeout;
}

void
GX2SetGPUTimeout(uint32_t timeout)
{
   gGpuTimeout = timeout;
}

} // namespace gx2
