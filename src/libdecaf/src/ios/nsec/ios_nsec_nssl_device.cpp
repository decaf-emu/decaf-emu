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

} // namespace ios::nsec::internal
