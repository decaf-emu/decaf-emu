#include "nn_olv.h"
#include "nn_olv_downloadedtopicdata.h"
#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::nn::olv
{

DownloadedTopicData::DownloadedTopicData()
{
   decaf_warn_stub();

   mUnk1 = 0u;
   mCommunityId = 0u;
}

uint32_t
DownloadedTopicData::GetCommunityId()
{
   decaf_warn_stub();
   return mCommunityId;
}

uint32_t
DownloadedTopicData::GetUserCount()
{
   decaf_warn_stub();
   return 0;
}

void
Library::registerDownloadedTopicDataSymbols()
{
   RegisterConstructorExport("__ct__Q3_2nn3olv19DownloadedTopicDataFv",
                             DownloadedTopicData);
   RegisterFunctionExportName("GetCommunityId__Q3_2nn3olv19DownloadedTopicDataCFv",
                              &DownloadedTopicData::GetCommunityId);
   RegisterFunctionExportName("GetUserCount__Q3_2nn3olv19DownloadedTopicDataCFv",
                              &DownloadedTopicData::GetUserCount);
}

}  // namespace namespace cafe::nn::olv
