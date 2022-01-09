#include "ios_nsec_nssl_certstore.h"
#include "ios_nsec_nssl_device.h"

#include "ios/crypto/ios_crypto_ipc.h"
#include "ios/kernel/ios_kernel_heap.h"
#include "ios/ios_stackobject.h"

#include <gsl/gsl-lite.hpp>

using namespace ::ios::kernel;

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
NSSLDevice::exportInternalClientCertificate(
   phys_ptr<NSSLExportInternalClientCertificateRequest> request,
   phys_ptr<NSSLExportInternalClientCertificateResponse> response,
   phys_ptr<uint8_t> certBuffer,
   uint32_t certBufferSize,
   phys_ptr<uint8_t> privateKeyBuffer,
   uint32_t privateKeyBufferSize)
{
   auto certMetaData = lookupCertMetaData(request->certId);
   if (!certMetaData) {
      return NSSLError::InvalidCertId2;
   }

   if (!checkCertPermission(certMetaData, mTitleId, mProcessId, mCapabilities)) {
      return NSSLError::CertNoAccess;
   }

   if (!checkCertExportable(certMetaData)) {
      return NSSLError::CertNotExportable;
   }

   if (certMetaData->type != 2) {
      return NSSLError::InvalidCertType;
   }

   if (certMetaData->encoding != NSSLCertEncoding::Unknown1) {
      return NSSLError::InvalidCertEncoding;
   }

   auto fileSize = getCertFileSize(certMetaData, 1);
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

   auto encryptedCertBuffer =
      IOS_HeapAllocAligned(CrossProcessHeapId, certBufferSize, 0x10u);
   auto decryptedCertBuffer =
      IOS_HeapAllocAligned(CrossProcessHeapId, certBufferSize, 0x10u);
   auto _ = gsl::finally([&]() {
         IOS_HeapFree(CrossProcessHeapId, encryptedCertBuffer);
         IOS_HeapFree(CrossProcessHeapId, decryptedCertBuffer);
      });

   fileSize = getCertFileData(certMetaData, 1, encryptedCertBuffer, certBufferSize);
   if (!fileSize.has_value()) {
      return NSSLError::CertReadError;
   }

   StackArray<uint8_t, 16> iv;
   memset(iv.get(), 0, iv.size());

   auto ioscHandle = static_cast<crypto::IOSCHandle>(crypto::IOSC_Open());
   auto error = crypto::IOSC_Decrypt(ioscHandle, crypto::KeyId::SslRsaKey,
                                     iv, iv.size(),
                                     encryptedCertBuffer, certBufferSize,
                                     decryptedCertBuffer, certBufferSize);
   if (error != Error::OK) {
      return NSSLError::CertReadError;
   }

   response->certSize = *fileSize;
   response->certType = NSSLCertType::Unknown0;
   memcpy(certBuffer.get(), decryptedCertBuffer.get(), certBufferSize);

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

   if (!checkCertExportable(certMetaData)) {
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
