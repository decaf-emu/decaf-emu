#pragma once
#include "ios_nsec_enum.h"
#include "ios_nsec_nssl_types.h"
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
   NSSLDevice(Handle socketHandle) :
      mSocketHandle(socketHandle)
   {
   }

   NSSLError createContext(NSSLVersion version);
   NSSLError addServerPKI(NSSLContextHandle context, NSSLCertID cert);
   NSSLError addServerPKIExternal(NSSLContextHandle context, phys_ptr<uint8_t> cert, uint32_t certSize, NSSLCertType certType);

private:
   Handle mSocketHandle;
   std::array<Context, MaxNumContexts> mContexts;
};

} // namespace ios::nsec::internal
