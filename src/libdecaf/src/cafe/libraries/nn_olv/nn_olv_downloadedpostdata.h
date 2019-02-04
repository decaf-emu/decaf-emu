#pragma once
#include "nn_olv_downloadeddatabase.h"

#include "cafe/libraries/ghs/cafe_ghs_typeinfo.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_olv
{

struct DownloadedPostData : public DownloadedDataBase
{
   static virt_ptr<ghs::VirtualTable> VirtualTable;
   static virt_ptr<ghs::TypeDescriptor> TypeDescriptor;

   be2_val<uint32_t> communityId;
   be2_val<uint32_t> empathyCount;
   be2_val<uint32_t> commentCount;
   UNKNOWN(0x1F4);
};
CHECK_OFFSET(DownloadedPostData, 0xC008, communityId);
CHECK_OFFSET(DownloadedPostData, 0xC00C, empathyCount);
CHECK_OFFSET(DownloadedPostData, 0xC010, commentCount);
CHECK_SIZE(DownloadedPostData, 0xC208);

virt_ptr<DownloadedPostData>
DownloadedPostData_Constructor(virt_ptr<DownloadedPostData> self);

void
DownloadedPostData_Destructor(virt_ptr<DownloadedPostData> self,
                              ghs::DestructorFlags flags);

uint32_t
DownloadedPostData_GetCommentCount(virt_ptr<const DownloadedPostData> self);

uint32_t
DownloadedPostData_GetCommunityId(virt_ptr<const DownloadedPostData> self);

uint32_t
DownloadedPostData_GetEmpathyCount(virt_ptr<const DownloadedPostData> self);

virt_ptr<const uint8_t>
DownloadedPostData_GetPostId(virt_ptr<const DownloadedPostData> self);

}  // namespace cafe::nn_olv
