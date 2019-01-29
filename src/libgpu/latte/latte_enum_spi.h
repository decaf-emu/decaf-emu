#ifndef LATTE_ENUM_SPI_H
#define LATTE_ENUM_SPI_H

#include <common/enum_start.inl>

ENUM_NAMESPACE_ENTER(latte)

ENUM_BEG(SPI_BARYC_CNTL, uint32_t)
   ENUM_VALUE(CENTROIDS_ONLY,             0)
   ENUM_VALUE(CENTERS_ONLY,               1)
   ENUM_VALUE(CENTROIDS_AND_CENTERS,      2)
ENUM_END(SPI_BARYC_CNTL)

ENUM_BEG(SPI_FOG_FUNC, uint32_t)
   ENUM_VALUE(NONE,                       0)
   ENUM_VALUE(EXP,                        1)
   ENUM_VALUE(EXP2,                       2)
   ENUM_VALUE(LINEAR,                     3)
ENUM_END(SPI_FOG_FUNC)

ENUM_BEG(SPI_FOG_SRC_SEL, uint32_t)
   ENUM_VALUE(SEL_Z,                      0)
   ENUM_VALUE(SEL_W,                      1)
ENUM_END(SPI_FOG_SRC_SEL)

ENUM_BEG(SPI_PNT_SPRITE_SEL, uint32_t)
   ENUM_VALUE(SEL_0,                      0)
   ENUM_VALUE(SEL_1,                      1)
   ENUM_VALUE(SEL_S,                      2)
   ENUM_VALUE(SEL_T,                      3)
   ENUM_VALUE(SEL_NONE,                   4)
ENUM_END(SPI_PNT_SPRITE_SEL)

ENUM_NAMESPACE_EXIT(latte)

#include <common/enum_end.inl>

#endif // ifdef LATTE_ENUM_SPI_H
