#pragma once
#include "nn_olv_downloadeddatabase.h"

#include "cafe/libraries/cafe_hle_library_typeinfo.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_olv
{

class DownloadedPostData : public DownloadedDataBase
{
public:
   static virt_ptr<hle::VirtualTable> VirtualTable;
   static virt_ptr<hle::TypeDescriptor> TypeDescriptor;

public:
   DownloadedPostData();
   ~DownloadedPostData();

   uint32_t
   GetCommentCount() const;

   uint32_t
   GetCommunityId() const;

   uint32_t
   GetEmpathyCount() const;

   virt_ptr<const uint8_t>
   GetPostId() const;

protected:
   be2_val<uint32_t> mCommunityId;
   be2_val<uint32_t> mEmpathyCount;
   be2_val<uint32_t> mCommentCount;
   UNKNOWN(0x1F4);

private:
   CHECK_MEMBER_OFFSET_BEG
   CHECK_OFFSET(DownloadedPostData, 0xC008, mCommunityId);
   CHECK_OFFSET(DownloadedPostData, 0xC00C, mEmpathyCount);
   CHECK_OFFSET(DownloadedPostData, 0xC010, mCommentCount);
   CHECK_MEMBER_OFFSET_END
};
CHECK_SIZE(DownloadedPostData, 0xC208);

}  // namespace cafe::nn_olv
