#pragma once
#include "ios/ios_enum.h"
#include "ios_crypto_enum.h"

#include <common/structsize.h>
#include <libcpu/be2_struct.h>

namespace ios::crypto
{

/**
 * \ingroup ios_crypto
 * @{
 */

#pragma pack(push, 1)

using IOSCKeyHandle = uint32_t;

struct IOSCRequestDecrypt
{
   be2_val<uint32_t> unknown0x00;
   be2_val<uint32_t> unknown0x04;
   be2_val<IOSCKeyHandle> keyHandle;
   PADDING(4);
};
CHECK_OFFSET(IOSCRequestDecrypt, 0x00, unknown0x00);
CHECK_OFFSET(IOSCRequestDecrypt, 0x04, unknown0x04);
CHECK_OFFSET(IOSCRequestDecrypt, 0x08, keyHandle);
CHECK_SIZE(IOSCRequestDecrypt, 0x10);

#pragma pack(pop)

/** @} */

} // namespace ios::crypto
