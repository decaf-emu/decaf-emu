#pragma once
#include "nn_olv_uploadeddatabase.h"
#include "cafe/libraries/cafe_hle_library_typeinfo.h"
#include <libcpu/be2_struct.h>

namespace cafe::nn::olv
{

class UploadedPostData : public UploadedDataBase
{
public:
   static virt_ptr<hle::VirtualTable> VirtualTable;
   static virt_ptr<hle::TypeDescriptor> TypeDescriptor;

public:
   UploadedPostData();
   ~UploadedPostData();

   uint32_t
   GetAppDataSize();

   nn::Result
   GetAppData(virt_ptr<uint8_t> buffer,
              virt_ptr<uint32_t> outDataSize,
              uint32_t bufferSize);

   nn::Result
   GetBodyText(virt_ptr<char16_t> buffer,
               uint32_t bufferSize);

   nn::Result
   GetBodyMemo(virt_ptr<uint8_t> buffer,
               virt_ptr<uint32_t> outMemoSize,
               uint32_t bufferSize);

   int32_t
   GetFeeling();

   virt_ptr<const char>
   GetPostId();

   bool
   TestFlags(uint32_t flag);

private:
   UNKNOWN(0x200);
};
CHECK_SIZE(UploadedPostData, 0xC208);

} // namespace cafe::nn::olv
