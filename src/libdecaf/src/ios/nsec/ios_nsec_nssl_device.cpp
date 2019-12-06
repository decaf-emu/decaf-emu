#include "ios_nsec_nssl_certstore.h"
#include "ios_nsec_nssl_device.h"

namespace ios::nsec::internal
{

NSSLError
NSSLDevice::createContext(NSSLVersion version)
{
   for (auto i = 0u; i < mContexts.size(); ++i) {
      auto &context = mContexts[i];
      if (context.initialised) {
         continue;
      }

      context.initialised = true;
      context.version = version;
      return static_cast<NSSLError>(i);
   }

   return NSSLError::ProcessMaxContexts;
}

NSSLError
NSSLDevice::addServerPKI(NSSLContextHandle context, NSSLCertID cert)
{
   return NSSLError::OK;
}

NSSLError
NSSLDevice::addServerPKIExternal(NSSLContextHandle context,
                                 phys_ptr<uint8_t> cert,
                                 uint32_t certSize,
                                 NSSLCertType certType)
{
   return NSSLError::OK;
}

NSSLError
NSSLDevice::exportInternalServerCertificate(
   phys_ptr<NSSLExportInternalServerCertificateRequest> request,
   phys_ptr<NSSLExportInternalServerCertificateResponse> response,
   phys_ptr<uint8_t> certBuffer,
   uint32_t certBufferSize)
{
   auto certMetaData = lookupCertMetaData(request->certId);
   if (!certMetaData) {
      return NSSLError::InvalidCertId2;
   }

   if (!checkCertPermission(certMetaData, mTitleId, mProcessId, mCapabilities)) {
      return NSSLError::CertNoAccess;
   }

   if (checkCertExportable(certMetaData)) {
      return NSSLError::CertNotExportable;
   }

   if (certMetaData->type != 1) {
      return NSSLError::InvalidCertType;
   }

   if (certMetaData->encoding != NSSLCertEncoding::Unknown1) {
      return NSSLError::InvalidCertEncoding;
   }

   auto fileSize = getCertFileSize(certMetaData, 0);
   if (!fileSize.has_value()) {
      return NSSLError::InvalidCertSize;
   }

   if (!certBuffer || !certBufferSize) {
      response->certSize = *fileSize;
      response->certType = NSSLCertType::Unknown0;
      return NSSLError::OK;
   }

   if (certBufferSize < fileSize) {
      return NSSLError::InsufficientSize;
   }

   fileSize = getCertFileData(certMetaData, 0, certBuffer, certBufferSize);
   if (!fileSize.has_value()) {
      return NSSLError::CertReadError;
   }

   response->certSize = *fileSize;
   response->certType = NSSLCertType::Unknown0;
   return NSSLError::OK;
}

} // namespace ios::nsec::internal
