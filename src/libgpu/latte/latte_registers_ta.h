#pragma once
#include <common/bitfield.h>
#include <cstdint>

namespace latte
{

// Texture Addresser Common Control
BITFIELD_BEG(TA_CNTL_AUX, uint32_t)
   BITFIELD_ENTRY(0, 1, bool, DISABLE_CUBE_WRAP)
   BITFIELD_ENTRY(1, 1, bool, UNK0)
   BITFIELD_ENTRY(24, 1, bool, SYNC_GRADIENT)
   BITFIELD_ENTRY(25, 1, bool, SYNC_WALKER)
   BITFIELD_ENTRY(26, 1, bool, SYNC_ALIGNER)
   BITFIELD_ENTRY(31, 1, bool, BILINEAR_PRECISION)
BITFIELD_END

} // namespace latte
