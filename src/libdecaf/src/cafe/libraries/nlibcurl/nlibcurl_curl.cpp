#include "nlibcurl.h"
#include "nlibcurl_curl.h"

#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::nlibcurl
{

CURLcode
curl_global_init(int32_t flags)
{
   // Host curl_global_init is called during decaf initialisation
   return ::CURLE_OK;
}

CURLcode
curl_global_init_mem(int32_t flags,
                     virt_ptr<void> mallocCallback,
                     virt_ptr<void> freeCallback,
                     virt_ptr<void> reallocCallback,
                     virt_ptr<void> strdupCallback,
                     virt_ptr<void> callocCallback)
{
   decaf_warn_stub();
   return ::CURLE_OK;
}

void
curl_global_cleanup()
{
   // Host curl_global_cleanup is called during decaf exit
}

void
Library::registerCurlSymbols()
{
   RegisterFunctionExport(curl_global_init);
   RegisterFunctionExport(curl_global_init_mem);
   RegisterFunctionExport(curl_global_cleanup);
}

} // namespace cafe::nlibcurl
