#pragma once
#include "ios/nsec/ios_nsec_nssl.h"

namespace cafe::nsysnet
{

using NSSLContextHandle = ios::nsec::NSSLContextHandle;
using NSSLError = ios::nsec::NSSLError;
using NSSLVersion = ios::nsec::NSSLVersion;
using NSSLCertID = ios::nsec::NSSLCertID;
using NSSLCertType = ios::nsec::NSSLCertType;
using NSSLPrivateKeyType = ios::nsec::NSSLPrivateKeyType;

NSSLError
NSSLInit();

NSSLError
NSSLFinish();

NSSLError
NSSLCreateContext(NSSLVersion version);

NSSLError
NSSLDestroyContext(NSSLContextHandle context);

NSSLError
NSSLAddServerPKI(NSSLContextHandle context,
                 NSSLCertID certId);

NSSLError
NSSLAddServerPKIExternal(NSSLContextHandle context,
                         virt_ptr<uint8_t> cert,
                         uint32_t certSize,
                         NSSLCertType certType);

NSSLError
NSSLExportInternalClientCertificate(NSSLCertID certId,
                                    virt_ptr<uint8_t> certBuffer,
                                    virt_ptr<uint32_t> certBufferSize,
                                    virt_ptr<NSSLCertType> certType,
                                    virt_ptr<uint8_t> privateKeyBuffer,
                                    virt_ptr<uint32_t> privateKeyBufferSize,
                                    virt_ptr<NSSLPrivateKeyType> privateKeyType);

NSSLError
NSSLExportInternalServerCertificate(NSSLCertID certId,
                                    virt_ptr<uint8_t> certBuffer,
                                    virt_ptr<uint32_t> certBufferSize,
                                    virt_ptr<NSSLCertType> certType);

} // namespace cafe::nsysnet
