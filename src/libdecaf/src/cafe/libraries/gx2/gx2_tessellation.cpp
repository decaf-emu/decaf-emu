#include "gx2.h"
#include "gx2_internal_cbpool.h"
#include "gx2_tessellation.h"
#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::gx2
{

void
GX2SetTessellation(GX2TessellationMode tessellationMode,
                   GX2PrimitiveMode primitiveMode,
                   GX2IndexType indexType)
{
   decaf_warn_stub();
   // TODO: Set registers 0xA285, 0xA289, 0xA28A, 0xA28B, 0xA28C, 0xA28E, 0xA28D, 0xA28F
}

void
GX2SetMinTessellationLevel(float min)
{
   auto vgt_hos_min_tess_level = latte::VGT_HOS_MIN_TESS_LEVEL::get(0);

   vgt_hos_min_tess_level = vgt_hos_min_tess_level
      .MIN_TESS(min);

   internal::writePM4(latte::pm4::SetContextReg {
      latte::Register::VGT_HOS_MIN_TESS_LEVEL,
      vgt_hos_min_tess_level.value
   });
}

void
GX2SetMaxTessellationLevel(float max)
{
   auto vgt_hos_max_tess_level = latte::VGT_HOS_MAX_TESS_LEVEL::get(0);

   vgt_hos_max_tess_level = vgt_hos_max_tess_level
      .MAX_TESS(max);

   internal::writePM4(latte::pm4::SetContextReg {
      latte::Register::VGT_HOS_MAX_TESS_LEVEL,
      vgt_hos_max_tess_level.value
   });
}

void
Library::registerTessellationSymbols()
{
   RegisterFunctionExport(GX2SetTessellation);
   RegisterFunctionExport(GX2SetMinTessellationLevel);
   RegisterFunctionExport(GX2SetMaxTessellationLevel);
}

} // namespace cafe::gx2
