#pragma once
#include "nn_olv.h"
#include "nn_olv_result.h"
#include "common/be_val.h"
#include "common/structsize.h"
#include "modules/coreinit/coreinit_ghs_typeinfo.h"

namespace nn
{

namespace olv
{

class UploadedDataBase
{
public:
   static ghs::VirtualTableEntry *VirtualTable;
   static ghs::TypeDescriptor *TypeInfo;

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
   GetAppData(uint8_t *buffer, be_val<uint32_t> *size, uint32_t bufferSize);

   nn::Result
   GetBodyText(char16_t *buffer, uint32_t bufferSize);

   nn::Result
   GetBodyMemo(uint8_t *buffer, be_val<uint32_t> *size, uint32_t bufferSize);

   nn::Result
   GetCommonData(uint32_t *unk, uint8_t *buffer, be_val<uint32_t> *size, uint32_t bufferSize);

   int32_t
   GetFeeling();

   const char *
   GetPostId();

   bool
   TestFlags(uint32_t flag);

protected:
   be_val<uint32_t> mFlags;
   char mPostID[32];
   char16_t mBodyText[256];
   be_val<uint32_t> mBodyTextLength;
   uint8_t mBodyMemo[0xA000];
   be_val<uint32_t> mBodyMemoLength;
   uint8_t mAppData[1024];
   be_val<uint32_t> mAppDataLength;
   int8_t mFeeling;
   UNKNOWN(3);
   be_val<uint32_t> mCommonDataUnknown;
   be_val<uint32_t> mCommonDataLength;
   uint8_t mCommonData[0x1000];
   UNKNOWN(0x9C8);
   be_ptr<ghs::VirtualTableEntry> mVirtualTable;

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
   CHECK_MEMBER_OFFSET_END
};
CHECK_SIZE(UploadedDataBase, 0xC008);

}  // namespace olv

}  // namespace nn
