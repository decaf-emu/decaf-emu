#pragma once
#include "ios_kernel_enum.h"
#include "ios/ios_error.h"

#include <cstdint>
#include <libcpu/be2_struct.h>

namespace ios::kernel
{

Error
IOS_ReadOTP(OtpFieldIndex fieldIndex,
            phys_ptr<uint32_t> buffer,
            uint32_t bufferSize);

namespace internal
{

Error
initialiseOtp();

} // namespace internal

} // namespace ios::kernel
