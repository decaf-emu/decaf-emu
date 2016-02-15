#pragma once
#include "nn_olv.h"
#include "utils/be_val.h"
#include "utils/structsize.h"

namespace nn
{

namespace olv
{

class DownloadedTopicData
{
public:
   DownloadedTopicData();

   uint32_t
   GetCommunityId();

   uint32_t
   GetUserCount();

protected:
   be_val<uint32_t> mUnk1;
   be_val<uint32_t> mCommunityId;
   UNKNOWN(0xFF8);
};
CHECK_SIZE(DownloadedTopicData, 0x1000);

}  // namespace olv

}  // namespace nn
