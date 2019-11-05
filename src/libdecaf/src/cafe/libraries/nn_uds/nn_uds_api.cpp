#include "nn_uds.h"

#include "cafe/libraries/coreinit/coreinit_mutex.h"
#include "cafe/libraries/coreinit/coreinit_ios.h"

using namespace cafe::coreinit;

namespace cafe::nn_uds
{

static virt_ptr<OSMutex> sLock = nullptr;
static virt_ptr<IOSHandle> sUdsIpcHandle = nullptr;

void
udsApiCppGlobalConstructor()
{
   *sUdsIpcHandle = -1;
   OSInitMutex(sLock);
}

void
Library::registerApiSymbols()
{
   RegisterFunctionExportName("__sti___11_uds_Api_cpp_f5d9abb2", udsApiCppGlobalConstructor);

   RegisterDataExportName("s_Lock__Q3_2nn3uds4Cafe", sLock);
   RegisterDataExportName("s_UdsIpc__Q3_2nn3uds4Cafe", sUdsIpcHandle);
}

} // namespace cafe::nn_uds
