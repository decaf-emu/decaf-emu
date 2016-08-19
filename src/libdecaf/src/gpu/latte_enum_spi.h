#pragma once
#include "common/types.h"

namespace latte
{

enum SPI_BARYC_CNTL
{
   SPI_CENTROIDS_ONLY         = 0,
   SPI_CENTERS_ONLY           = 1,
   SPI_CENTROIDS_AND_CENTERS  = 2,
};

enum SPI_FOG_FUNC
{
   SPI_FOG_NONE               = 0,
   SPI_FOG_EXP                = 1,
   SPI_FOG_EXP2               = 2,
   SPI_FOG_LINEAR             = 3,
};

enum SPI_FOG_SRC_SEL
{
   SPI_FOG_SRC_SEL_Z          = 0,
   SPI_FOG_SRC_SEL_W          = 1,
};

enum SPI_PNT_SPRITE_SEL
{
   SPI_PNT_SPRITE_SEL_0       = 0,
   SPI_PNT_SPRITE_SEL_1       = 1,
   SPI_PNT_SPRITE_SEL_S       = 2,
   SPI_PNT_SPRITE_SEL_T       = 3,
   SPI_PNT_SPRITE_SEL_NONE    = 4,
};

} // namespace latte
