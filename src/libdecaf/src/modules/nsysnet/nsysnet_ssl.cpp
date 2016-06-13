#include "nsysnet.h"
#include "nsysnet_ssl.h"

namespace nsysnet
{

int
NSSLInit()
{
   return 0;
}

int
NSSLFinish()
{
   return 0;
}

void
Module::registerSSLFunctions()
{
   RegisterKernelFunction(NSSLInit);
   RegisterKernelFunction(NSSLFinish);
}

} // namespace nsysnet
