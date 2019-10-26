#pragma once
#include "ios_nsec_enum.h"
#include "ios/ios_ipc.h"

#include <array>

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

private:
   Handle mSocketHandle;
   std::array<Context, MaxNumContexts> mContexts;
};

} // namespace ios::nsec::internal
