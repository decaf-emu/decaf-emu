#ifndef LATTE_ENUM_PA_H
#define LATTE_ENUM_PA_H

#include <common/enum_start.inl>

ENUM_NAMESPACE_ENTER(latte)

ENUM_BEG(PA_FACE, uint32_t)
   ENUM_VALUE(CCW,                        0)
   ENUM_VALUE(CW,                         1)
ENUM_END(PA_FACE)

ENUM_BEG(PA_PTYPE, uint32_t)
   ENUM_VALUE(POINTS, 0)
   ENUM_VALUE(LINES, 1)
   ENUM_VALUE(TRIANGLES, 2)
ENUM_END(PA_PTYPE)

ENUM_BEG(PA_PS_UCP_MODE, uint32_t)
   ENUM_VALUE(CULL_DISTANCE,              0)
   ENUM_VALUE(CULL_RADIUS,                1)
   ENUM_VALUE(CULL_RADIUS_EXPAND,         2)
   ENUM_VALUE(CULL_EXPAND,                3)
ENUM_END(PA_PS_UCP_MODE)

ENUM_BEG(PA_SU_VTX_CNTL_PIX_CENTER, uint32_t)
   ENUM_VALUE(D3D,                        0)
   ENUM_VALUE(OGL,                        1)
ENUM_END(PA_SU_VTX_CNTL_PIX_CENTER)

ENUM_BEG(PA_SU_VTX_CNTL_ROUND_MODE, uint32_t)
   ENUM_VALUE(TRUNCATE,                   0)
   ENUM_VALUE(NEAREST,                    1)
   ENUM_VALUE(TO_EVEN,                    2)
   ENUM_VALUE(TO_ODD,                     3)
ENUM_END(PA_SU_VTX_CNTL_ROUND_MODE)

ENUM_BEG(PA_SU_VTX_CNTL_QUANT_MODE, uint32_t)
   ENUM_VALUE(QUANT_1_16TH,               0)
   ENUM_VALUE(QUANT_1_8TH,                1)
   ENUM_VALUE(QUANT_1_4TH,                2)
   ENUM_VALUE(QUANT_1_2ND,                3)
   ENUM_VALUE(QUANT_1,                    4)
   ENUM_VALUE(QUANT_1_256TH,              5)
ENUM_END(PA_SU_VTX_CNTL_QUANT_MODE)

ENUM_NAMESPACE_EXIT(latte)

#include <common/enum_end.inl>

#endif // ifdef LATTE_ENUM_PA_H
