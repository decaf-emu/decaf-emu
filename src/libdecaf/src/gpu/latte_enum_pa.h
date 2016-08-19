#pragma once
#include "common/types.h"

namespace latte
{

enum PA_FACE : uint32_t
{
   FACE_CCW = 0,
   FACE_CW  = 1
};

enum PA_PS_UCP_MODE : uint32_t
{
   PA_PS_UCP_CULL_DISTANCE       = 0,
   PA_PS_UCP_CULL_RADIUS         = 1,
   PA_PS_UCP_CULL_RADIUS_EXPAND  = 2,
   PA_PS_UCP_CULL_EXPAND         = 3,
};

enum PA_SU_VTX_CNTL_PIX_CENTER : uint32_t
{
   PA_SU_VTX_CNTL_PIX_CENTER_D3D = 0,
   PA_SU_VTX_CNTL_PIX_CENTER_OGL = 0,
};

enum PA_SU_VTX_CNTL_ROUND_MODE : uint32_t
{
   PA_SU_VTX_CNTL_ROUND_TRUNCATE = 0,
   PA_SU_VTX_CNTL_ROUND          = 1,
   PA_SU_VTX_CNTL_ROUND_TO_EVEN  = 2,
   PA_SU_VTX_CNTL_ROUND_TO_ODD   = 3,
};

enum PA_SU_VTX_CNTL_QUANT_MODE : uint32_t
{
   PA_SU_VTX_CNTL_QUANT_1_16TH   = 0,
   PA_SU_VTX_CNTL_QUANT_1_8TH    = 1,
   PA_SU_VTX_CNTL_QUANT_1_4TH    = 2,
   PA_SU_VTX_CNTL_QUANT_1_2ND    = 3,
   PA_SU_VTX_CNTL_QUANT_1        = 4,
   PA_SU_VTX_CNTL_QUANT_1_256TH  = 5,
};

} // namespace latte
