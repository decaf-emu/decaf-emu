#pragma once
#include <cstdint>
#include <curl/curl.h>

namespace cafe::nlibcurl
{

struct CURL
{
   ::CURL *hostHandle;
};

using CURLcode = int32_t;

CURLcode
curl_global_init(int32_t flags);

CURLcode
curl_global_init_mem(int32_t flags,
                     virt_ptr<void> mallocCallback,
                     virt_ptr<void> freeCallback,
                     virt_ptr<void> reallocCallback,
                     virt_ptr<void> strdupCallback,
                     virt_ptr<void> callocCallback);

void
curl_global_cleanup();

} // namespace cafe::nlibcurl
