#include "nsysnet.h"
#include "nsysnet_socket_lib.h"

namespace nsysnet
{

int
socket_lib_init()
{
   decaf_warn_stub();

   return 0;
}

int
socket_lib_finish()
{
   decaf_warn_stub();

   return 0;
}

void
Module::registerSocketLibFunctions()
{
   RegisterKernelFunction(socket_lib_init);
   RegisterKernelFunction(socket_lib_finish);
}

} // namespace nsysnet
