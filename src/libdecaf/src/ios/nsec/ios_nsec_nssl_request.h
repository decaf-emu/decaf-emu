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

struct NSSLCreateContextRequest
{
   be2_val<NSSLVersion> version;
};
CHECK_OFFSET(NSSLCreateContextRequest, 0x00, version);
CHECK_SIZE(NSSLCreateContextRequest, 0x04);

struct NSSLDestroyContextRequest
{
   be2_val<NSSLContextHandle> context;
};
CHECK_OFFSET(NSSLDestroyContextRequest, 0x00, context);
CHECK_SIZE(NSSLDestroyContextRequest, 0x04);

struct NSSLAddServerPKIRequest
{
   be2_val<NSSLContextHandle> context;
   be2_val<NSSLCertID> cert;
};
CHECK_OFFSET(NSSLAddServerPKIRequest, 0x00, context);
CHECK_OFFSET(NSSLAddServerPKIRequest, 0x04, cert);
CHECK_SIZE(NSSLAddServerPKIRequest, 0x08);

struct NSSLAddServerPKIExternalRequest
{
   be2_val<NSSLContextHandle> context;
   be2_val<NSSLCertType> certType;
};
CHECK_OFFSET(NSSLAddServerPKIExternalRequest, 0x00, context);
CHECK_OFFSET(NSSLAddServerPKIExternalRequest, 0x04, certType);
CHECK_SIZE(NSSLAddServerPKIExternalRequest, 0x08);

struct NSSLExportInternalServerCertificateRequest
{
   be2_val<NSSLCertID> certId;
};
CHECK_OFFSET(NSSLExportInternalServerCertificateRequest, 0x00, certId);
CHECK_SIZE(NSSLExportInternalServerCertificateRequest, 0x04);

#pragma pack(pop)

/** @} */

} // namespace ios::net
