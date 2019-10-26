#pragma once
#include "ios_nsec_enum.h"

#include <cstdint>
#include <libcpu/be2_struct.h>
#include <common/structsize.h>

namespace ios::nsec
{

/**
 * \ingroup ios_nsec
 * @{
 */

#pragma pack(push, 1)

struct NSSLCreateContextRequest
{
   be2_val<NSSLVersion> version;
};
CHECK_OFFSET(NSSLCreateContextRequest, 0x00, version);
CHECK_SIZE(NSSLCreateContextRequest, 0x04);

#pragma pack(pop)

/** @} */

} // namespace ios::net
