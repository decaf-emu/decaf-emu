#pragma once
#include "im_enum.h"

#include <cstdint>
#include <common/be_val.h>
#include <common/be_ptr.h>
#include <common/structsize.h>

namespace ios
{

namespace dev
{

namespace im
{

/**
 * \ingroup ios_dev_im
 * @{
 */

#pragma pack(push, 1)

struct IMGetParameterRequest
{
   be_val<IMParameter> parameter;
   UNKNOWN(4);
};
CHECK_OFFSET(IMGetParameterRequest, 0x00, parameter);
CHECK_SIZE(IMGetParameterRequest, 0x08);

struct IMGetNvParameterRequest
{
   be_val<IMParameter> parameter;
   UNKNOWN(4);
};
CHECK_OFFSET(IMGetNvParameterRequest, 0x00, parameter);
CHECK_SIZE(IMGetNvParameterRequest, 0x08);

struct IMGetTimerRemainingRequest
{
   be_val<IMTimer> timer;
   UNKNOWN(4);
};
CHECK_OFFSET(IMGetTimerRemainingRequest, 0x00, timer);
CHECK_SIZE(IMGetTimerRemainingRequest, 0x08);

struct IMSetParameterRequest
{
   be_val<IMParameter> parameter;
   be_val<uint32_t> value;
};
CHECK_OFFSET(IMSetParameterRequest, 0x00, parameter);
CHECK_OFFSET(IMSetParameterRequest, 0x04, value);
CHECK_SIZE(IMSetParameterRequest, 0x08);

struct IMSetNvParameterRequest
{
   be_val<IMParameter> parameter;
   be_val<uint32_t> value;
};
CHECK_OFFSET(IMSetNvParameterRequest, 0x00, parameter);
CHECK_OFFSET(IMSetNvParameterRequest, 0x04, value);
CHECK_SIZE(IMSetNvParameterRequest, 0x08);

#pragma pack(pop)

/** @} */

} // namespace im

} // namespace dev

} // namespace ios
