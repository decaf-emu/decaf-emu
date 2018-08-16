#include "nsysnet.h"
#include "nsysnet_ssl.h"
#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::nsysnet
{

Error
NSSLInit()
{
   decaf_warn_stub();
   return Error::OK;
}

Error
NSSLFinish()
{
   decaf_warn_stub();
   return Error::OK;
}

void
Library::registerSslSymbols()
{
   RegisterFunctionExport(NSSLInit);
   RegisterFunctionExport(NSSLFinish);
}

} // namespace cafe::nsysnet
