#include "nlibcurl.h"
#include "nlibcurl_curl.h"

namespace cafe::nlibcurl
{

CURLcode
curl_global_init(int32_t flags)
{
   // Host libcurl initialisation is handled by our application
   return ::CURLE_OK;
}

void
curl_global_cleanup()
{
   // Host libcurl cleanup is handled by our application
}

void
Library::registerCurlSymbols()
{
   RegisterFunctionExport(curl_global_init);
   RegisterFunctionExport(curl_global_cleanup);
}

} // namespace cafe::nlibcurl
