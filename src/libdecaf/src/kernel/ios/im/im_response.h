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

struct IMGetHomeButtonParamResponse
{
   be_val<uint32_t> unknown0x00;
   be_val<uint32_t> unknown0x04;
};
CHECK_OFFSET(IMGetHomeButtonParamResponse, 0x00, unknown0x00);
CHECK_OFFSET(IMGetHomeButtonParamResponse, 0x04, unknown0x04);

struct IMGetParameterResponse
{
   be_val<IMParameter> parameter;
   be_val<uint32_t> value;
};
CHECK_OFFSET(IMGetParameterResponse, 0x00, parameter);
CHECK_OFFSET(IMGetParameterResponse, 0x04, value);

struct IMGetNvParameterResponse
{
   be_val<IMParameter> parameter;
   be_val<uint32_t> value;
};
CHECK_OFFSET(IMGetNvParameterResponse, 0x00, parameter);
CHECK_OFFSET(IMGetNvParameterResponse, 0x04, value);

struct IMGetTimerRemainingResponse
{
   be_val<IMTimer> timer;
   be_val<uint32_t> value;
};
CHECK_OFFSET(IMGetTimerRemainingResponse, 0x00, timer);
CHECK_OFFSET(IMGetTimerRemainingResponse, 0x04, value);

#pragma pack(pop)

/** @} */

} // namespace im

} // namespace ios

} // namespace kernel
