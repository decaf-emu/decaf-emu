#pragma once
#include "im_enum.h"

#include <cstdint>
#include <common/be_val.h>
#include <common/be_ptr.h>
#include <common/structsize.h>

namespace kernel
{

namespace ios
{

namespace im
{

/**
 * \ingroup kernel_ios_im
 * @{
 */

#pragma pack(push, 1)

struct IMGetParameterRequest
{
   be_val<IMParameter> parameter;
};
CHECK_OFFSET(IMGetParameterRequest, 0x00, parameter);

struct IMGetNvParameterRequest
{
   be_val<IMParameter> parameter;
};
CHECK_OFFSET(IMGetNvParameterRequest, 0x00, parameter);

struct IMGetTimerRemainingRequest
{
   be_val<IMTimer> timer;
};
CHECK_OFFSET(IMGetTimerRemainingRequest, 0x00, timer);

struct IMSetParameterRequest
{
   be_val<IMParameter> parameter;
   be_val<uint32_t> value;
};
CHECK_OFFSET(IMSetParameterRequest, 0x00, parameter);
CHECK_OFFSET(IMSetParameterRequest, 0x04, value);

struct IMSetNvParameterRequest
{
   be_val<IMParameter> parameter;
   be_val<uint32_t> value;
};
CHECK_OFFSET(IMSetNvParameterRequest, 0x00, parameter);
CHECK_OFFSET(IMSetNvParameterRequest, 0x04, value);

#pragma pack(pop)

/** @} */

} // namespace im

} // namespace ios

} // namespace kernel
