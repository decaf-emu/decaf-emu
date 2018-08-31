#include "gx2.h"
#include "gx2_internal_cbpool.h"
#include "gx2_query.h"
#include "gx2_memory.h"
#include "cafe/libraries/coreinit/coreinit_cache.h"

#include <libcpu/mem.h>

using namespace cafe::coreinit;
using namespace latte::pm4;

namespace cafe::gx2
{

static uint32_t
gGpuTimeout = 10000;

void
GX2SampleTopGPUCycle(virt_ptr<int64_t> writeSamplePtr)
{
   *writeSamplePtr = -1;

   auto addrLo = MW_ADDR_LO::get(0)
      .ADDR_LO(virt_cast<virt_addr>(writeSamplePtr) >> 2)
      .ENDIAN_SWAP(latte::CB_ENDIAN::SWAP_8IN64);

   auto addrHi = MW_ADDR_HI::get(0)
      .CNTR_SEL(MW_WRITE_CLOCK);

   internal::writePM4(MemWrite { addrLo, addrHi, 0, 0 });
}

void
GX2SampleBottomGPUCycle(virt_ptr<int64_t> writeSamplePtr)
{
   *writeSamplePtr = -1;

   auto eventInitiator = latte::VGT_EVENT_INITIATOR::get(0)
      .EVENT_TYPE(latte::VGT_EVENT_TYPE::BOTTOM_OF_PIPE_TS);

   auto addrLo = EW_ADDR_LO::get(0)
      .ADDR_LO(virt_cast<virt_addr>(writeSamplePtr) >> 2)
      .ENDIAN_SWAP(latte::CB_ENDIAN::SWAP_8IN64);

   auto addrHi = EWP_ADDR_HI::get(0)
      .DATA_SEL(EWP_DATA_CLOCK);

   internal::writePM4(EventWriteEOP { eventInitiator, addrLo, addrHi, 0, 0 });
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

static void
beginOcclusionQuery(virt_ptr<GX2QueryData> data,
                    bool gpuMemoryWrite)
{
   decaf_check(virt_cast<virt_addr>(data) % 4 == 0);

   // There is some magic number read from their global gx2 state + 0xB0C, no
   // fucking clue what it is, so let's just set it to 0?
   auto magicNumber = 0u;

   if (gpuMemoryWrite) {
      DCInvalidateRange(data, sizeof(GX2QueryData));

      // Zero GX2QueryData from GPU
      for (auto i = 0u; i < 8; ++i) {
         auto addr = virt_cast<virt_addr>(data) + 8 * i;

         auto addrLo = MW_ADDR_LO::get(0)
            .ADDR_LO(addr >> 2);

         auto addrHi = MW_ADDR_HI::get(0)
            .WR_CONFIRM(true);

         auto dataHi = 0u;

         if (i < magicNumber) {
            dataHi = 0x80000000;
         }

         internal::writePM4(MemWrite { addrLo, addrHi, 0, dataHi });
      }

      internal::writePM4(PfpSyncMe {});
   } else {
      std::memset(data.getRawPointer(), 0, sizeof(GX2QueryData));

      auto dataWords = virt_cast<uint32_t *>(data);
      dataWords[magicNumber + 0] = 0x4F435055u; // "OCPU"
      dataWords[magicNumber + 1] = 0u;
      GX2Invalidate(GX2InvalidateMode::StreamOutBuffer,
                    dataWords,
                    sizeof(GX2QueryData));
   }

   // DB_RENDER_CONTROL
   auto render_control = latte::DB_RENDER_CONTROL::get(0)
      .PERFECT_ZPASS_COUNTS(true);

   internal::writePM4(SetContextReg { latte::Register::DB_RENDER_CONTROL, render_control.value });

   // EVENT_WRITE
   auto eventInitiator = latte::VGT_EVENT_INITIATOR::get(0)
      .EVENT_TYPE(latte::VGT_EVENT_TYPE::ZPASS_DONE)
      .EVENT_INDEX(latte::VGT_EVENT_INDEX::ZPASS_DONE);

   auto addrLo = EW_ADDR_LO::get(0)
      .ADDR_LO(virt_cast<virt_addr>(data) >> 2);

   auto addrHi = EW_ADDR_HI::get(0);

   internal::writePM4(EventWrite { eventInitiator, addrLo, addrHi });
}

static void
endOcclusionQuery(virt_ptr<GX2QueryData> data,
                  bool gpuMemoryWrite)
{
   // EVENT_WRITE
   auto eventInitiator = latte::VGT_EVENT_INITIATOR::get(0)
      .EVENT_TYPE(latte::VGT_EVENT_TYPE::ZPASS_DONE)
      .EVENT_INDEX(latte::VGT_EVENT_INDEX::ZPASS_DONE);

   auto addrLo = EW_ADDR_LO::get(0)
      .ADDR_LO((virt_cast<virt_addr>(data) + 8) >> 2);

   auto addrHi = EW_ADDR_HI::get(0);

   internal::writePM4(EventWrite { eventInitiator, addrLo, addrHi });

   // DB_RENDER_CONTROL
   auto render_control = latte::DB_RENDER_CONTROL::get(0)
      .PERFECT_ZPASS_COUNTS(false);

   internal::writePM4(SetContextReg { latte::Register::DB_RENDER_CONTROL, render_control.value });
}

static void
beginStreamOutStatsQuery(virt_ptr<GX2QueryData> data,
                         bool gpuMemoryWrite)
{
   decaf_check(virt_cast<virt_addr>(data) % 4 == 0);

   // There is some magic number read from their global gx2 state + 0xB0C, no
   // fucking clue what it is, so let's just set it to 0?
   auto magicNumber = 0u;
   auto endianSwap = latte::CB_ENDIAN::NONE;

   if (gpuMemoryWrite) {
      DCInvalidateRange(data, sizeof(GX2QueryData));

      // Zero data first
      for (auto i = 0u; i < 4; ++i) {
         auto addr = virt_cast<virt_addr>(data) + 8 * i;

         auto addrLo = MW_ADDR_LO::get(0)
            .ADDR_LO(addr >> 2);

         auto addrHi = MW_ADDR_HI::get(0)
            .WR_CONFIRM(true);

         internal::writePM4(MemWrite { addrLo, addrHi, 0, 0 });
      }

      internal::writePM4(PfpSyncMe {});

      endianSwap = latte::CB_ENDIAN::SWAP_8IN32;
   } else {
      std::memset(data.getRawPointer(), 0, sizeof(GX2QueryData));

      auto dataWords = virt_cast<uint32_t *>(data);
      dataWords[magicNumber + 0] = 0x53435055u; // "SCPU"
      dataWords[magicNumber + 1] = 0u;
      GX2Invalidate(GX2InvalidateMode::StreamOutBuffer, dataWords, sizeof(GX2QueryData));

      endianSwap = latte::CB_ENDIAN::SWAP_8IN64;
   }

   // EVENT_WRITE
   auto eventInitiator = latte::VGT_EVENT_INITIATOR::get(0)
      .EVENT_TYPE(latte::VGT_EVENT_TYPE::SAMPLE_STREAMOUTSTATS)
      .EVENT_INDEX(latte::VGT_EVENT_INDEX::SAMPLE_STREAMOUTSTAT);

   auto addrLo = EW_ADDR_LO::get(0)
      .ADDR_LO(virt_cast<virt_addr>(data) >> 2)
      .ENDIAN_SWAP(endianSwap);

   auto addrHi = EW_ADDR_HI::get(0);

   internal::writePM4(EventWrite { eventInitiator, addrLo, addrHi });
}

static void
endStreamOutStatsQuery(virt_ptr<GX2QueryData> data,
                       bool gpuMemoryWrite)
{
   auto endianSwap = latte::CB_ENDIAN::NONE;

   if (gpuMemoryWrite) {
      endianSwap = latte::CB_ENDIAN::SWAP_8IN32;
   } else {
      endianSwap = latte::CB_ENDIAN::SWAP_8IN64;
   }

   // EVENT_WRITE
   auto eventInitiator = latte::VGT_EVENT_INITIATOR::get(0)
      .EVENT_TYPE(latte::VGT_EVENT_TYPE::SAMPLE_STREAMOUTSTATS)
      .EVENT_INDEX(latte::VGT_EVENT_INDEX::SAMPLE_STREAMOUTSTAT);

   auto addrLo = EW_ADDR_LO::get(0)
      .ADDR_LO(virt_cast<virt_addr>(data) >> 2)
      .ENDIAN_SWAP(endianSwap);

   auto addrHi = EW_ADDR_HI::get(0);

   internal::writePM4(EventWrite { eventInitiator, addrLo, addrHi });
}

void
GX2QueryBegin(GX2QueryType type,
              virt_ptr<GX2QueryData> data)
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
            virt_ptr<GX2QueryData> data)
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

void
GX2QueryGetOcclusionResult(virt_ptr<GX2QueryData> data,
                           virt_ptr<uint64_t> outResult)
{
   *outResult = *(virt_cast<uint64_t *>(data) + 1);
}

void
GX2QueryBeginConditionalRender(GX2QueryType type,
                               virt_ptr<GX2QueryData> data,
                               BOOL hint,
                               BOOL predicate)
{
   auto addr = virt_cast<virt_addr>(data);
   auto op = SP_PRED_OP_PRIMCOUNT;

   if (type == 2) {
      op = SP_PRED_OP_ZPASS;
   }

   auto set_pred = SET_PRED::get(0)
      .PRED_OP(op)
      .HINT(!!hint)
      .PREDICATE(!!predicate);

   internal::writePM4(SetPredication { static_cast<uint32_t>(addr), set_pred });
}

void
GX2QueryEndConditionalRender()
{
   internal::writePM4(SetPredication { 0, SET_PRED::get(0) });
}

void
Library::registerQuerySymbols()
{
   RegisterFunctionExport(GX2QueryBegin);
   RegisterFunctionExport(GX2QueryEnd);
   RegisterFunctionExport(GX2QueryGetOcclusionResult);
   RegisterFunctionExport(GX2QueryBeginConditionalRender);
   RegisterFunctionExport(GX2QueryEndConditionalRender);
   RegisterFunctionExport(GX2SampleTopGPUCycle);
   RegisterFunctionExport(GX2SampleBottomGPUCycle);
   RegisterFunctionExport(GX2GPUTimeToCPUTime);
   RegisterFunctionExport(GX2GetGPUTimeout);
   RegisterFunctionExport(GX2SetGPUTimeout);
}

} // namespace cafe::gx2
