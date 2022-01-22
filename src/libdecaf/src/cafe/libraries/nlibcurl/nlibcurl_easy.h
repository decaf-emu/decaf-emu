#pragma once
#include <cstdint>
#include <libcpu/be2_struct.h>
#include "nlibcurl_curl.h"

namespace cafe::nlibcurl
{

using CURLoption = int32_t;

virt_ptr<CURL>
curl_easy_init();

void
curl_easy_cleanup(virt_ptr<CURL> handle);

CURLcode
curl_easy_setopt(virt_ptr<CURL> handle,
                 CURLoption option,
                 var_args args);

} // namespace cafe::nlibcurl
