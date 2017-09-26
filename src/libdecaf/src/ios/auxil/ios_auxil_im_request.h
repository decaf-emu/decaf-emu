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

struct IMGetParameterRequest
{
   be2_val<IMParameter> parameter;
   UNKNOWN(4);
};
CHECK_OFFSET(IMGetParameterRequest, 0x00, parameter);
CHECK_SIZE(IMGetParameterRequest, 0x08);

struct IMGetNvParameterRequest
{
   be2_val<IMParameter> parameter;
   UNKNOWN(4);
};
CHECK_OFFSET(IMGetNvParameterRequest, 0x00, parameter);
CHECK_SIZE(IMGetNvParameterRequest, 0x08);

struct IMGetTimerRemainingRequest
{
   be2_val<IMTimer> timer;
   UNKNOWN(4);
};
CHECK_OFFSET(IMGetTimerRemainingRequest, 0x00, timer);
CHECK_SIZE(IMGetTimerRemainingRequest, 0x08);

struct IMSetParameterRequest
{
   be2_val<IMParameter> parameter;
   be2_val<uint32_t> value;
};
CHECK_OFFSET(IMSetParameterRequest, 0x00, parameter);
CHECK_OFFSET(IMSetParameterRequest, 0x04, value);
CHECK_SIZE(IMSetParameterRequest, 0x08);

struct IMSetNvParameterRequest
{
   be2_val<IMParameter> parameter;
   be2_val<uint32_t> value;
};
CHECK_OFFSET(IMSetNvParameterRequest, 0x00, parameter);
CHECK_OFFSET(IMSetNvParameterRequest, 0x04, value);
CHECK_SIZE(IMSetNvParameterRequest, 0x08);

struct IMRequest
{
   union
   {
      be2_struct<IMGetParameterRequest> getParameter;
      be2_struct<IMGetNvParameterRequest> getNvParameter;
      be2_struct<IMGetTimerRemainingRequest>getTimerRemaining;
      be2_struct<IMSetParameterRequest> setParameter;
      be2_struct<IMSetNvParameterRequest> setNvParameter;
   };
};
CHECK_OFFSET(IMRequest, 0x00, getParameter);
CHECK_OFFSET(IMRequest, 0x00, getNvParameter);
CHECK_OFFSET(IMRequest, 0x00, getTimerRemaining);
CHECK_OFFSET(IMRequest, 0x00, setParameter);
CHECK_OFFSET(IMRequest, 0x00, setNvParameter);
CHECK_SIZE(IMRequest, 0x08);

#pragma pack(pop)

/** @} */

} // namespace ios::auxil
