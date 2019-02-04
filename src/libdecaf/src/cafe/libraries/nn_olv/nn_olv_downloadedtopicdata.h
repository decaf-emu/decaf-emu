#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::nn_olv
{

struct DownloadedTopicData
{
   be2_val<uint32_t> unk1;
   be2_val<uint32_t> communityId;
   UNKNOWN(0xFF8);
};
CHECK_OFFSET(DownloadedTopicData, 0x00, unk1);
CHECK_OFFSET(DownloadedTopicData, 0x04, communityId);
CHECK_SIZE(DownloadedTopicData, 0x1000);

virt_ptr<DownloadedTopicData>
DownloadedTopicData_Constructor(virt_ptr<DownloadedTopicData> self);

uint32_t
DownloadedTopicData_GetCommunityId(virt_ptr<const DownloadedTopicData> self);

uint32_t
DownloadedTopicData_GetUserCount(virt_ptr<const DownloadedTopicData> self);

}  // namespace cafe::nn_olv
