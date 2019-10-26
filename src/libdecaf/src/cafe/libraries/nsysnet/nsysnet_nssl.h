#pragma once
#include "ios/nsec/ios_nsec_nssl.h"

namespace cafe::nsysnet
{

using NSSLContextHandle = int32_t;
using NSSLError = ios::nsec::NSSLError;
using NSSLVersion = ios::nsec::NSSLVersion;

NSSLError
NSSLInit();

NSSLError
NSSLFinish();

NSSLContextHandle
NSSLCreateContext(NSSLVersion version);

namespace internal
{

void
initialiseNSSL();

} // namespace internal

} // namespace cafe::nsysnet
