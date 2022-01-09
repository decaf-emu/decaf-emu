#pragma once
#include "ios_crypto_enum.h"
#include "ios_crypto_types.h"

#include "ios/ios_enum.h"
#include "ios/kernel/ios_kernel_messagequeue.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"

namespace ios::crypto
{

using IOSCHandle = kernel::ResourceHandleId;

Error
IOSC_Open();

Error
IOSC_Close(IOSCHandle handle);

IOSCError
IOSC_Decrypt(IOSCHandle handle,
             IOSCKeyHandle keyHandle,
             phys_ptr<const void> ivData, uint32_t ivSize,
             phys_ptr<const void> inputData, uint32_t inputSize,
             phys_ptr<void> outputData, uint32_t outputSize);

} // namespace ios::crypto
