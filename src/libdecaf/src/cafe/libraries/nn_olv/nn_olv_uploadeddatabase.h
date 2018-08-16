#pragma once
#include "nn_olv_result.h"
#include "cafe/libraries/cafe_hle_library_typeinfo.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn::olv
{

class UploadedDataBase
{
public:
   static virt_ptr<hle::VirtualTable> VirtualTable;
   static virt_ptr<hle::TypeDescriptor> TypeDescriptor;

   enum Flags
   {
      Empty          = 0,
      HasBodyText    = 1 << 0,
      HasBodyMemo    = 1 << 1,
      HasAppData     = 1 << 2,
   };

public:
   UploadedDataBase();
   ~UploadedDataBase();

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

   nn::Result
   GetCommonData(virt_ptr<uint32_t> unk,
                 virt_ptr<uint8_t> buffer,
                 virt_ptr<uint32_t> outDataSize,
                 uint32_t bufferSize);

   int32_t
   GetFeeling();

   virt_ptr<const char>
   GetPostId();

   bool
   TestFlags(uint32_t flag);

protected:
   be2_val<uint32_t> mFlags;
   be2_array<char, 32> mPostID;
   be2_array<char16_t, 256> mBodyText;
   be2_val<uint32_t> mBodyTextLength;
   be2_array<uint8_t, 0xA000> mBodyMemo;
   be2_val<uint32_t> mBodyMemoLength;
   be2_array<uint8_t, 1024> mAppData;
   be2_val<uint32_t> mAppDataLength;
   be2_val<int8_t> mFeeling;
   UNKNOWN(3);
   be2_val<uint32_t> mCommonDataUnknown;
   be2_val<uint32_t> mCommonDataLength;
   be2_array<uint8_t, 0x1000> mCommonData;
   UNKNOWN(0x9C8);
   be2_virt_ptr<hle::VirtualTable> mVirtualTable;

private:
   CHECK_MEMBER_OFFSET_START
   CHECK_OFFSET(UploadedDataBase, 0x00, mFlags);
   CHECK_OFFSET(UploadedDataBase, 0x04, mPostID);
   CHECK_OFFSET(UploadedDataBase, 0x24, mBodyText);
   CHECK_OFFSET(UploadedDataBase, 0x224, mBodyTextLength);
   CHECK_OFFSET(UploadedDataBase, 0x228, mBodyMemo);
   CHECK_OFFSET(UploadedDataBase, 0xA228, mBodyMemoLength);
   CHECK_OFFSET(UploadedDataBase, 0xA22C, mAppData);
   CHECK_OFFSET(UploadedDataBase, 0xA62C, mAppDataLength);
   CHECK_OFFSET(UploadedDataBase, 0xA630, mFeeling);
   CHECK_OFFSET(UploadedDataBase, 0xA634, mCommonDataUnknown);
   CHECK_OFFSET(UploadedDataBase, 0xA638, mCommonDataLength);
   CHECK_OFFSET(UploadedDataBase, 0xA63C, mCommonData);
   CHECK_OFFSET(UploadedDataBase, 0xC004, mVirtualTable);
   CHECK_MEMBER_OFFSET_END
};
CHECK_SIZE(UploadedDataBase, 0xC008);

}  // namespace cafe::nn::olv
