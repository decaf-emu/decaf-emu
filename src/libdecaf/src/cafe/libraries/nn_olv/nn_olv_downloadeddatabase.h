#pragma once
#include "cafe/libraries/cafe_hle_library_typeinfo.h"
#include "nn/nn_result.h"
#include "nn/ffl/nn_ffl_miidata.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_olv
{

class DownloadedDataBase
{
public:
   static virt_ptr<hle::VirtualTable> VirtualTable;
   static virt_ptr<hle::TypeDescriptor> TypeDescriptor;

private:
   enum Flags
   {
      Empty                   = 0,
      HasBodyText             = 1 << 0,
      HasBodyMemo             = 1 << 1,
      HasExternalImageData    = 1 << 2,
      HasExternalBinaryData   = 1 << 3,
      HasMiiData              = 1 << 4,
      HasExternalUrl          = 1 << 5,
      HasAppData              = 1 << 6,
   };

public:
   DownloadedDataBase();
   ~DownloadedDataBase();

   nn::Result
   DownloadExternalBinaryData(virt_ptr<void> dataBuffer,
                              virt_ptr<uint32_t> outDataSize,
                              uint32_t dataBufferSize) const;
   nn::Result
   DownloadExternalImageData(virt_ptr<void> dataBuffer,
                             virt_ptr<uint32_t> outDataSize,
                             uint32_t dataBufferSize) const;

   nn::Result
   GetAppData(virt_ptr<uint32_t> dataBuffer,
              virt_ptr<uint32_t> outSize,
              uint32_t dataBufferSize) const;

   uint32_t
   GetAppDataSize() const;

   nn::Result
   GetBodyMemo(virt_ptr<uint8_t> memoBuffer,
               virt_ptr<uint32_t> outSize,
               uint32_t memoBufferSize) const;

   nn::Result
   GetBodyText(virt_ptr<char16_t> textBuffer,
               uint32_t textBufferSize) const;

   uint8_t
   GetCountryId() const;

   uint32_t
   GetExternalBinaryDataSize() const;

   uint32_t
   GetExternalImageDataSize() const;

   virt_ptr<const char>
   GetExternalUrl() const;

   uint8_t
   GetFeeling() const;

   uint8_t
   GetLanguageId() const;

   nn::Result
   GetMiiData(virt_ptr<nn::ffl::FFLStoreData> outData) const;

   virt_ptr<nn::ffl::FFLStoreData>
   GetMiiData() const;

   virt_ptr<const char16_t>
   GetMiiNickname() const;

   uint8_t
   GetPlatformId() const;

   uint64_t
   GetPostDate() const;

   virt_ptr<const uint8_t>
   GetPostId() const;

   uint8_t
   GetRegionId() const;

   virt_ptr<const uint8_t>
   GetTopicTag() const;

   uint32_t
   GetUserPid() const;

   bool
   TestFlags(uint32_t flagMask) const;

protected:
   be2_val<uint32_t> mFlags;
   be2_val<uint32_t> mUserPid;
   be2_array<uint8_t, 32> mPostId;
   be2_val<uint64_t> mPostDate;
   be2_val<uint8_t> mFeeling;
   PADDING(3);
   be2_val<uint32_t> mRegionId;
   be2_val<uint8_t> mPlatformId;
   be2_val<uint8_t> mLanguageId;
   be2_val<uint8_t> mCountryId;
   PADDING(1);
   be2_array<char16_t, 256> mBodyText;
   be2_val<uint32_t> mBodyTextLength;
   be2_array<uint8_t, 40960> mBodyMemo;
   be2_val<uint32_t> mBodyMemoSize;
   be2_array<uint8_t, 304> mTopicTag;
   be2_array<uint8_t, 1024> mAppData;
   be2_val<uint32_t> mAppDataSize;
   be2_array<char, 256> mExternalBinaryUrl;
   be2_val<uint32_t> mExternalBinaryDataSize;
   be2_array<char, 256> mExternalImageUrl;
   be2_val<uint32_t> mExternalImageDataSize;
   be2_array<char, 256> mExternalUrl;
   be2_struct<nn::ffl::FFLStoreData> mMiiData;
   be2_array<char16_t, 128> mMiiNickname; // Actual size unknown
   UNKNOWN(0xC000 - 0xABE0);
   be2_virt_ptr<hle::VirtualTable> mVirtualTable;
   UNKNOWN(4);

private:
   CHECK_MEMBER_OFFSET_BEG
   CHECK_OFFSET(DownloadedDataBase, 0x00, mFlags);
   CHECK_OFFSET(DownloadedDataBase, 0x04, mUserPid);
   CHECK_OFFSET(DownloadedDataBase, 0x08, mPostId);
   CHECK_OFFSET(DownloadedDataBase, 0x28, mPostDate);
   CHECK_OFFSET(DownloadedDataBase, 0x30, mFeeling);
   CHECK_OFFSET(DownloadedDataBase, 0x34, mRegionId);
   CHECK_OFFSET(DownloadedDataBase, 0x38, mPlatformId);
   CHECK_OFFSET(DownloadedDataBase, 0x39, mLanguageId);
   CHECK_OFFSET(DownloadedDataBase, 0x3A, mCountryId);
   CHECK_OFFSET(DownloadedDataBase, 0x3C, mBodyText);
   CHECK_OFFSET(DownloadedDataBase, 0x23C, mBodyTextLength);
   CHECK_OFFSET(DownloadedDataBase, 0x240, mBodyMemo);
   CHECK_OFFSET(DownloadedDataBase, 0xA240, mBodyMemoSize);
   CHECK_OFFSET(DownloadedDataBase, 0xA244, mTopicTag);
   CHECK_OFFSET(DownloadedDataBase, 0xA374, mAppData);
   CHECK_OFFSET(DownloadedDataBase, 0xA774, mAppDataSize);
   CHECK_OFFSET(DownloadedDataBase, 0xA778, mExternalBinaryUrl);
   CHECK_OFFSET(DownloadedDataBase, 0xA878, mExternalBinaryDataSize);
   CHECK_OFFSET(DownloadedDataBase, 0xA87C, mExternalImageUrl);
   CHECK_OFFSET(DownloadedDataBase, 0xA97C, mExternalImageDataSize);
   CHECK_OFFSET(DownloadedDataBase, 0xA980, mExternalUrl);
   CHECK_OFFSET(DownloadedDataBase, 0xAA80, mMiiData);
   CHECK_OFFSET(DownloadedDataBase, 0xAAE0, mMiiNickname);
   // CHECK_OFFSET(DownloadedDataBase, 0xC000, mMiiNickname);
   CHECK_MEMBER_OFFSET_END
};
CHECK_SIZE(DownloadedDataBase, 0xC008);

}  // namespace cafe::nn_olv
