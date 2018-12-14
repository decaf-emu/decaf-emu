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

void
curl_global_cleanup();

} // namespace cafe::nlibcurl
