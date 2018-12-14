#include "nlibcurl.h"
#include "nlibcurl_easy.h"

#include "cafe/cafe_ppc_interface_varargs.h"

#include <curl/curl.h>

namespace cafe::nlibcurl
{

struct StaticEasyData
{
   be2_array<CURL, 128> handles;
};

static virt_ptr<StaticEasyData>
sEasyData = nullptr;

virt_ptr<CURL>
curl_easy_init()
{
   auto handle = ::curl_easy_init();
   if (!handle) {
      return nullptr;
   }

   for (auto i = 0u; i < sEasyData->handles.size(); ++i) {
      if (!sEasyData->handles[i].hostHandle) {
         sEasyData->handles[i].hostHandle = handle;
         return virt_addrof(sEasyData->handles[i]);
      }
   }

   ::curl_easy_cleanup(handle);
   return nullptr;
}

void
curl_easy_cleanup(virt_ptr<CURL> handle)
{
   ::curl_easy_cleanup(handle->hostHandle);
   handle->hostHandle = nullptr;
}

CURLcode
curl_easy_setopt(virt_ptr<CURL> handle,
                 CURLoption option,
                 var_args args)
{
   auto vaList = make_va_list(args);
   auto curl = handle->hostHandle;

   // TODO: Translate to ::curl_easy_setopt

   free_va_list(vaList);
   return ::CURLE_OK;
}

void
Library::registerEasySymbols()
{
   RegisterFunctionExport(curl_easy_init);
   RegisterFunctionExport(curl_easy_cleanup);
   RegisterFunctionExport(curl_easy_setopt);

   RegisterDataInternal(sEasyData);
}

} // namespace cafe::nlibcurl
