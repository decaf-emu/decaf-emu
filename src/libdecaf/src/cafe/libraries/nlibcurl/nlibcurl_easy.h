#pragma once
#include <cstdint>
#include <libcpu/be2_struct.h>
#include "nlibcurl_curl.h"

namespace cafe::nlibcurl
{

virt_ptr<CURL>
curl_easy_init();

void
curl_easy_cleanup(virt_ptr<CURL> handle);

} // namespace cafe::nlibcurl
