#pragma once
#include "ios_nsec_enum.h"
#include "ios_nsec_nssl_types.h"
#include "ios_nsec_nssl_request.h"
#include "ios_nsec_nssl_response.h"
#include "ios/ios_ipc.h"

#include <array>
#include <libcpu/be2_struct.h>

namespace ios::nsec::internal
{

class NSSLDevice
{
   static constexpr const size_t MaxNumContexts = 32;

   struct Context
   {
      bool initialised = false;
      NSSLVersion version = NSSLVersion::Auto;
   };

public:
   NSSLDevice(TitleId titleId, ProcessId processId, uint64_t caps, Handle socketHandle) :
      mTitleId(titleId),
      mProcessId(processId),
      mCapabilities(caps),
      mSocketHandle(socketHandle)
   {
   }

   NSSLError createContext(NSSLVersion version);
   NSSLError addServerPKI(NSSLContextHandle context, NSSLCertID cert);
   NSSLError addServerPKIExternal(NSSLContextHandle context, phys_ptr<uint8_t> cert, uint32_t certSize, NSSLCertType certType);
   NSSLError exportInternalServerCertificate(
      phys_ptr<NSSLExportInternalServerCertificateRequest> request,
      phys_ptr<NSSLExportInternalServerCertificateResponse> response,
      phys_ptr<uint8_t> certBuffer,
      uint32_t certBufferSize);

private:
   TitleId mTitleId;
   ProcessId mProcessId;
   uint64_t mCapabilities;

   Handle mSocketHandle;
   std::array<Context, MaxNumContexts> mContexts;
};

} // namespace ios::nsec::internal
