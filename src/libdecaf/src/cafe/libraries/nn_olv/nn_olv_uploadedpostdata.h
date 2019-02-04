#pragma once
#include "nn_olv_uploadeddatabase.h"

#include "cafe/libraries/ghs/cafe_ghs_typeinfo.h"
#include "nn/nn_result.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_olv
{

struct UploadedPostData : public UploadedDataBase
{
   static virt_ptr<ghs::VirtualTable> VirtualTable;
   static virt_ptr<ghs::TypeDescriptor> TypeDescriptor;

   UNKNOWN(0x200);
};
CHECK_SIZE(UploadedPostData, 0xC208);

virt_ptr<UploadedPostData>
UploadedPostData_Constructor(virt_ptr<UploadedPostData> self);

void
UploadedPostData_Destructor(virt_ptr<UploadedPostData> self,
                            ghs::DestructorFlags flags);

uint32_t
UploadedPostData_GetAppDataSize(virt_ptr<const UploadedPostData> self);

nn::Result
UploadedPostData_GetAppData(virt_ptr<const UploadedPostData> self,
                            virt_ptr<uint8_t> buffer,
                            virt_ptr<uint32_t> outDataSize,
                            uint32_t bufferSize);

nn::Result
UploadedPostData_GetBodyText(virt_ptr<const UploadedPostData> self,
                             virt_ptr<char16_t> buffer,
                             uint32_t bufferSize);

nn::Result
UploadedPostData_GetBodyMemo(virt_ptr<const UploadedPostData> self,
                             virt_ptr<uint8_t> buffer,
                             virt_ptr<uint32_t> outMemoSize,
                             uint32_t bufferSize);

int32_t
UploadedPostData_GetFeeling(virt_ptr<const UploadedPostData> self);

virt_ptr<const char>
UploadedPostData_GetPostId(virt_ptr<const UploadedPostData> self);

bool
UploadedPostData_TestFlags(virt_ptr<const UploadedPostData> self,
                           uint32_t flag);

} // namespace cafe::nn_olv
