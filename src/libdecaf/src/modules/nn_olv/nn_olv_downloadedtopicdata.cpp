#include "nn_olv.h"
#include "nn_olv_downloadedtopicdata.h"

namespace nn
{

namespace olv
{

DownloadedTopicData::DownloadedTopicData()
{
   mUnk1 = 0;
   mCommunityId = 0;
}

uint32_t
DownloadedTopicData::GetCommunityId()
{
   return mCommunityId;
}

uint32_t
DownloadedTopicData::GetUserCount()
{
   return 0;
}

void
Module::registerDownloadedTopicData()
{
   RegisterKernelFunctionConstructor("__ct__Q3_2nn3olv19DownloadedTopicDataFv", DownloadedTopicData);
   RegisterKernelFunctionName("GetCommunityId__Q3_2nn3olv19DownloadedTopicDataCFv", &DownloadedTopicData::GetCommunityId);
   RegisterKernelFunctionName("GetUserCount__Q3_2nn3olv19DownloadedTopicDataCFv", &DownloadedTopicData::GetUserCount);
}

}  // namespace olv

}  // namespace nn
