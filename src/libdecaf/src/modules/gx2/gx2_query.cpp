#include "gx2_query.h"
#include "gx2_mem.h"
#include "gpu/pm4_writer.h"
#include "modules/coreinit/coreinit_cache.h"

namespace gx2
{

static uint32_t
gGpuTimeout = 10000;

void
GX2SampleTopGPUCycle(be_val<uint64_t> *result)
{
   *result = -1;

   auto addrLo = pm4::MW_ADDR_LO::get(0)
      .ADDR_LO(mem::untranslate(result) >> 2)
      .ENDIAN_SWAP(latte::CB_ENDIAN_8IN64);

   auto addrHi = pm4::MW_ADDR_HI::get(0)
      .CNTR_SEL(pm4::MW_WRITE_CLOCK);

   pm4::write(pm4::MemWrite { addrLo, addrHi, 0, 0 });
}

void
GX2SampleBottomGPUCycle(be_val<uint64_t> *result)
{
   *result = -1;

   auto eventInitiator = latte::VGT_EVENT_INITIATOR::get(0)
      .EVENT_TYPE(latte::VGT_EVENT_TYPE_BOTTOM_OF_PIPE_TS);

   auto addrLo = pm4::EW_ADDR_LO::get(0)
      .ADDR_LO(mem::untranslate(result) >> 2)
      .ENDIAN_SWAP(latte::CB_ENDIAN_8IN64);

   auto addrHi = pm4::EW_ADDR_HI::get(0)
      .DATA_SEL(pm4::EW_DATA_CLOCK);

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

void beginOcclusionQuery(GX2QueryData *data, bool gpuMemoryWrite)
{
   decaf_check(mem::untranslate(data) % 4 == 0);

   // There is some magic number read from their global gx2 state + 0xB0C, no
   // fucking clue what it is, so let's just set it to 0?
   auto magicNumber = 0u;

   if (gpuMemoryWrite) {
      coreinit::DCInvalidateRange(data, sizeof(GX2QueryData));

      for (auto i = 0u; i < 8; ++i) {
         auto addr = mem::untranslate(data) + 8 * i;

         auto addrLo = pm4::MW_ADDR_LO::get(0)
            .ADDR_LO(addr >> 2);

         auto addrHi = pm4::MW_ADDR_HI::get(0)
            .WR_CONFIRM(true);

         auto dataHi = 0u;

         if (i < magicNumber) {
            dataHi = 0x80000000;
         }

         pm4::write(pm4::MemWrite { addrLo, addrHi, 0, dataHi });
      }

      pm4::write(pm4::PfpSyncMe {});
   } else {
      std::memset(data, 0, sizeof(GX2QueryData));

      auto dataWords = reinterpret_cast<uint32_t *>(data);
      dataWords[magicNumber + 0] = 0x4F435055;
      dataWords[magicNumber + 1] = 0;
      GX2Invalidate(GX2InvalidateMode::StreamOutBuffer, dataWords, sizeof(GX2QueryData));
   }

   // DB_RENDER_CONTROL
   auto render_control = latte::DB_RENDER_CONTROL::get(0)
      .PERFECT_ZPASS_COUNTS(true);

   pm4::write(pm4::SetContextReg { latte::Register::DB_RENDER_CONTROL, render_control.value });

   // EVENT_WRITE
   auto eventInitiator = latte::VGT_EVENT_INITIATOR::get(0)
      .EVENT_TYPE(latte::VGT_EVENT_TYPE_ZPASS_DONE)
      .EVENT_INDEX(latte::VGT_EVENT_INDEX_ZPASS_DONE);

   auto addrLo = pm4::EW_ADDR_LO::get(0)
      .ADDR_LO(mem::untranslate(data) >> 2);

   auto addrHi = pm4::EW_ADDR_HI::get(0);

   pm4::write(pm4::EventWrite { eventInitiator, addrLo, addrHi });
}

void endOcclusionQuery(GX2QueryData *data, bool gpuMemoryWrite)
{
   // EVENT_WRITE
   auto eventInitiator = latte::VGT_EVENT_INITIATOR::get(0)
      .EVENT_TYPE(latte::VGT_EVENT_TYPE_ZPASS_DONE)
      .EVENT_INDEX(latte::VGT_EVENT_INDEX_ZPASS_DONE);

   auto addrLo = pm4::EW_ADDR_LO::get(0)
      .ADDR_LO((mem::untranslate(data) + 8) >> 2);

   auto addrHi = pm4::EW_ADDR_HI::get(0);

   pm4::write(pm4::EventWrite { eventInitiator, addrLo, addrHi });

   // DB_RENDER_CONTROL
   auto render_control = latte::DB_RENDER_CONTROL::get(0)
      .PERFECT_ZPASS_COUNTS(false);

   pm4::write(pm4::SetContextReg { latte::Register::DB_RENDER_CONTROL, render_control.value });
}

void beginStreamOutStatsQuery(GX2QueryData *data, bool gpuMemoryWrite)
{
   decaf_check(mem::untranslate(data) % 4 == 0);

   // There is some magic number read from their global gx2 state + 0xB0C, no
   // fucking clue what it is, so let's just set it to 0?
   auto magicNumber = 0u;
   auto endianSwap = latte::CB_ENDIAN {};

   if (gpuMemoryWrite) {
      coreinit::DCInvalidateRange(data, sizeof(GX2QueryData));

      for (auto i = 0u; i < 4; ++i) {
         auto addr = mem::untranslate(data) + 8 * i;

         auto addrLo = pm4::MW_ADDR_LO::get(0)
            .ADDR_LO(addr >> 2);

         auto addrHi = pm4::MW_ADDR_HI::get(0)
            .WR_CONFIRM(true);

         pm4::write(pm4::MemWrite { addrLo, addrHi, 0, 0 });
      }

      pm4::write(pm4::PfpSyncMe {});

      endianSwap = latte::CB_ENDIAN_8IN32;
   } else {
      std::memset(data, 0, sizeof(GX2QueryData));

      auto dataWords = reinterpret_cast<uint32_t *>(data);
      dataWords[magicNumber + 0] = 0x53435055;
      dataWords[magicNumber + 1] = 0;
      GX2Invalidate(GX2InvalidateMode::StreamOutBuffer, dataWords, sizeof(GX2QueryData));

      endianSwap = latte::CB_ENDIAN_8IN64;
   }

   // EVENT_WRITE
   auto eventInitiator = latte::VGT_EVENT_INITIATOR::get(0)
      .EVENT_TYPE(latte::VGT_EVENT_TYPE_SAMPLE_STREAMOUTSTATS)
      .EVENT_INDEX(latte::VGT_EVENT_INDEX_SAMPLE_STREAMOUTSTAT);

   auto addrLo = pm4::EW_ADDR_LO::get(0)
      .ADDR_LO(mem::untranslate(data) >> 2)
      .ENDIAN_SWAP(endianSwap);

   auto addrHi = pm4::EW_ADDR_HI::get(0);

   pm4::write(pm4::EventWrite { eventInitiator, addrLo, addrHi });
}

void endStreamOutStatsQuery(GX2QueryData *data, bool gpuMemoryWrite)
{
   auto endianSwap = latte::CB_ENDIAN {};

   if (gpuMemoryWrite) {
      endianSwap = latte::CB_ENDIAN_8IN32;
   } else {
      endianSwap = latte::CB_ENDIAN_8IN64;
   }

   // EVENT_WRITE
   auto eventInitiator = latte::VGT_EVENT_INITIATOR::get(0)
      .EVENT_TYPE(latte::VGT_EVENT_TYPE_SAMPLE_STREAMOUTSTATS)
      .EVENT_INDEX(latte::VGT_EVENT_INDEX_SAMPLE_STREAMOUTSTAT);

   auto addrLo = pm4::EW_ADDR_LO::get(0)
      .ADDR_LO(mem::untranslate(data) >> 2)
      .ENDIAN_SWAP(endianSwap);

   auto addrHi = pm4::EW_ADDR_HI::get(0);

   pm4::write(pm4::EventWrite { eventInitiator, addrLo, addrHi });
}

void
GX2QueryBegin(GX2QueryType type,
              GX2QueryData *data)
{
   switch (type) {
   case GX2QueryType::OcclusionQuery:
      beginOcclusionQuery(data, false);
      break;
   case GX2QueryType::OcclusionQueryGpuMem:
      beginOcclusionQuery(data, true);
      break;
   case GX2QueryType::StreamOutStats:
      beginStreamOutStatsQuery(data, false);
      break;
   case GX2QueryType::StreamOutStatsGpuMem:
      beginStreamOutStatsQuery(data, true);
      break;
   }
}

void
GX2QueryEnd(GX2QueryType type,
            GX2QueryData *data)
{
   switch (type) {
   case GX2QueryType::OcclusionQuery:
      endOcclusionQuery(data, false);
      break;
   case GX2QueryType::OcclusionQueryGpuMem:
      endOcclusionQuery(data, true);
      break;
   case GX2QueryType::StreamOutStats:
      endStreamOutStatsQuery(data, false);
      break;
   case GX2QueryType::StreamOutStatsGpuMem:
      endStreamOutStatsQuery(data, true);
      break;
   }
}

} // namespace gx2
