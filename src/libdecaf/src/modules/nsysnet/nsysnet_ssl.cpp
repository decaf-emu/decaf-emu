#include "nsysnet.h"
#include "nsysnet_ssl.h"

namespace nsysnet
{

int
NSSLInit()
{
   decaf_warn_stub();

   return 0;
}

int
NSSLFinish()
{
   decaf_warn_stub();

   return 0;
}

void
Module::registerSSLFunctions()
{
   RegisterKernelFunction(NSSLInit);
   RegisterKernelFunction(NSSLFinish);
}

} // namespace nsysnet
