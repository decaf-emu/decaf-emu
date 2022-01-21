#include "gx2.h"
#include "gx2_cbpool.h"
#include "gx2_fence.h"

#include "cafe/libraries/coreinit/coreinit_memory.h"

#include <common/decaf_assert.h>
#include <libcpu/be2_struct.h>
#include <libgpu/latte/latte_pm4.h>
#include <libgpu/latte/latte_pm4_writer.h>

using namespace cafe::coreinit;

namespace cafe::gx2
{

void
GX2SetGPUFence(virt_ptr<uint32_t> memory,
               uint32_t mask,
               GX2CompareFunction op,
               uint32_t value)
{
   using namespace latte;
   using namespace latte::pm4;

   if (op == GX2CompareFunction::Never) {
      return;
   }

   WRM_FUNCTION function;
   switch (op) {
   case GX2CompareFunction::Never:
      return;
   case GX2CompareFunction::Less:
      function = WRM_FUNCTION::FUNCTION_LESS_THAN;
      break;
   case GX2CompareFunction::Equal:
      function = WRM_FUNCTION::FUNCTION_EQUAL;
      break;
   case GX2CompareFunction::LessOrEqual:
      function = WRM_FUNCTION::FUNCTION_LESS_THAN_EQUAL;
      break;
   case GX2CompareFunction::Greater:
      function = WRM_FUNCTION::FUNCTION_GREATER_THAN;
      break;
   case GX2CompareFunction::NotEqual:
      function = WRM_FUNCTION::FUNCTION_NOT_EQUAL;
      break;
   case GX2CompareFunction::GreaterOrEqual:
      function = WRM_FUNCTION::FUNCTION_GREATER_THAN_EQUAL;
      break;
   case GX2CompareFunction::Always:
      function = WRM_FUNCTION::FUNCTION_ALWAYS;
      break;
   default:
      function = WRM_FUNCTION::FUNCTION_ALWAYS;
   }

   auto addr = OSEffectiveToPhysical(virt_cast<virt_addr>(memory));
   internal::writePM4(WaitMem {
      MEM_SPACE_FUNCTION::get(0)
         .MEM_SPACE(WRM_MEM_SPACE::MEM_SPACE_MEMORY)
         .FUNCTION(function)
         .ENGINE(WRM_ENGINE::ENGINE_ME),
      WRM_ADDR_LO::get(0)
         .ADDR_LO(addr >> 2)
         .ENDIAN_SWAP(CB_ENDIAN::SWAP_8IN32),
      WRM_ADDR_HI::get(0),
      value, mask, 10,
   });
}

void
Library::registerFenceSymbols()
{
   RegisterFunctionExport(GX2SetGPUFence);
}

} // namespace cafe::gx2
