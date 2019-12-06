#pragma once
#include "ios_nsec_enum.h"
#include "ios_nsec_nssl_types.h"

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

struct NSSLExportInternalServerCertificateResponse
{
   be2_val<NSSLCertType> certType;
   be2_val<uint32_t> certSize;
};
CHECK_OFFSET(NSSLExportInternalServerCertificateResponse, 0x00, certType);
CHECK_OFFSET(NSSLExportInternalServerCertificateResponse, 0x04, certSize);
CHECK_SIZE(NSSLExportInternalServerCertificateResponse, 0x08);

#pragma pack(pop)

/** @} */

} // namespace ios::net
