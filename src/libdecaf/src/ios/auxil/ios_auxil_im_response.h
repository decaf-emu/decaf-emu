#pragma once
#include "ios_auxil_enum.h"

#include <cstdint>
#include <common/structsize.h>
#include <libcpu/be2_struct.h>

namespace ios::auxil
{

/**
 * \ingroup ios_auxil
 * @{
 */

#pragma pack(push, 1)

struct IMGetHomeButtonParamResponse
{
   be2_val<IMHomeButtonType> type;
   be2_val<int32_t> index;
};
CHECK_OFFSET(IMGetHomeButtonParamResponse, 0x00, type);
CHECK_OFFSET(IMGetHomeButtonParamResponse, 0x04, index);

struct IMGetParameterResponse
{
   be2_val<IMParameter> parameter;
   be2_val<uint32_t> value;
};
CHECK_OFFSET(IMGetParameterResponse, 0x00, parameter);
CHECK_OFFSET(IMGetParameterResponse, 0x04, value);

struct IMGetNvParameterResponse
{
   be2_val<IMParameter> parameter;
   be2_val<uint32_t> value;
};
CHECK_OFFSET(IMGetNvParameterResponse, 0x00, parameter);
CHECK_OFFSET(IMGetNvParameterResponse, 0x04, value);

struct IMGetTimerRemainingResponse
{
   be2_val<IMTimer> timer;
   be2_val<uint32_t> value;
};
CHECK_OFFSET(IMGetTimerRemainingResponse, 0x00, timer);
CHECK_OFFSET(IMGetTimerRemainingResponse, 0x04, value);

struct IMResponse
{
   union
   {
      be2_struct<IMGetHomeButtonParamResponse> getHomeButtonParam;
      be2_struct<IMGetParameterResponse> getParameter;
      be2_struct<IMGetNvParameterResponse> getNvParameter;
      be2_struct<IMGetTimerRemainingResponse> getTimerRemaining;
   };
};
CHECK_OFFSET(IMResponse, 0x00, getHomeButtonParam);
CHECK_OFFSET(IMResponse, 0x00, getParameter);
CHECK_OFFSET(IMResponse, 0x00, getNvParameter);
CHECK_OFFSET(IMResponse, 0x00, getTimerRemaining);
CHECK_SIZE(IMResponse, 0x08);

#pragma pack(pop)

/** @} */

} // namespace ios::auxil
