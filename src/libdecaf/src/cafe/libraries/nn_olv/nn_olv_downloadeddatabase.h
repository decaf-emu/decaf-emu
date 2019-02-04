#pragma once
#include "cafe/libraries/ghs/cafe_ghs_typeinfo.h"
#include "nn/nn_result.h"
#include "nn/ffl/nn_ffl_miidata.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_olv
{

struct DownloadedDataBase
{
   static virt_ptr<ghs::VirtualTable> VirtualTable;
   static virt_ptr<ghs::TypeDescriptor> TypeDescriptor;

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

   be2_val<uint32_t> flags;
   be2_val<uint32_t> userPid;
   be2_array<uint8_t, 32> postId;
   be2_val<uint64_t> postDate;
   be2_val<uint8_t> feeling;
   PADDING(3);
   be2_val<uint32_t> regionId;
   be2_val<uint8_t> platformId;
   be2_val<uint8_t> languageId;
   be2_val<uint8_t> countryId;
   PADDING(1);
   be2_array<char16_t, 256> bodyText;
   be2_val<uint32_t> bodyTextLength;
   be2_array<uint8_t, 40960> bodyMemo;
   be2_val<uint32_t> bodyMemoSize;
   be2_array<uint8_t, 304> topicTag;
   be2_array<uint8_t, 1024> appData;
   be2_val<uint32_t> appDataSize;
   be2_array<char, 256> externalBinaryUrl;
   be2_val<uint32_t> externalBinaryDataSize;
   be2_array<char, 256> externalImageUrl;
   be2_val<uint32_t> externalImageDataSize;
   be2_array<char, 256> externalUrl;
   be2_struct<nn::ffl::FFLStoreData> miiData;
   be2_array<char16_t, 128> miiNickname; // Actual size unknown
   UNKNOWN(0xC000 - 0xABE0);
   be2_virt_ptr<ghs::VirtualTable> virtualTable;
   UNKNOWN(4);
};
CHECK_OFFSET(DownloadedDataBase, 0x00, flags);
CHECK_OFFSET(DownloadedDataBase, 0x04, userPid);
CHECK_OFFSET(DownloadedDataBase, 0x08, postId);
CHECK_OFFSET(DownloadedDataBase, 0x28, postDate);
CHECK_OFFSET(DownloadedDataBase, 0x30, feeling);
CHECK_OFFSET(DownloadedDataBase, 0x34, regionId);
CHECK_OFFSET(DownloadedDataBase, 0x38, platformId);
CHECK_OFFSET(DownloadedDataBase, 0x39, languageId);
CHECK_OFFSET(DownloadedDataBase, 0x3A, countryId);
CHECK_OFFSET(DownloadedDataBase, 0x3C, bodyText);
CHECK_OFFSET(DownloadedDataBase, 0x23C, bodyTextLength);
CHECK_OFFSET(DownloadedDataBase, 0x240, bodyMemo);
CHECK_OFFSET(DownloadedDataBase, 0xA240, bodyMemoSize);
CHECK_OFFSET(DownloadedDataBase, 0xA244, topicTag);
CHECK_OFFSET(DownloadedDataBase, 0xA374, appData);
CHECK_OFFSET(DownloadedDataBase, 0xA774, appDataSize);
CHECK_OFFSET(DownloadedDataBase, 0xA778, externalBinaryUrl);
CHECK_OFFSET(DownloadedDataBase, 0xA878, externalBinaryDataSize);
CHECK_OFFSET(DownloadedDataBase, 0xA87C, externalImageUrl);
CHECK_OFFSET(DownloadedDataBase, 0xA97C, externalImageDataSize);
CHECK_OFFSET(DownloadedDataBase, 0xA980, externalUrl);
CHECK_OFFSET(DownloadedDataBase, 0xAA80, miiData);
CHECK_OFFSET(DownloadedDataBase, 0xAAE0, miiNickname);
CHECK_OFFSET(DownloadedDataBase, 0xC000, virtualTable);
CHECK_SIZE(DownloadedDataBase, 0xC008);

virt_ptr<DownloadedDataBase>
DownloadedDataBase_Constructor(virt_ptr<DownloadedDataBase> self);

void
DownloadedDataBase_Destructor(virt_ptr<DownloadedDataBase> self,
                              ghs::DestructorFlags flags);

nn::Result
DownloadedDataBase_DownloadExternalBinaryData(virt_ptr<const DownloadedDataBase> self,
                                              virt_ptr<void> dataBuffer,
                                              virt_ptr<uint32_t> outDataSize,
                                              uint32_t dataBufferSize);

nn::Result
DownloadedDataBase_DownloadExternalImageData(virt_ptr<const DownloadedDataBase> self,
                                             virt_ptr<void> dataBuffer,
                                             virt_ptr<uint32_t> outDataSize,
                                             uint32_t dataBufferSize);

nn::Result
DownloadedDataBase_GetAppData(virt_ptr<const DownloadedDataBase> self,
                              virt_ptr<uint32_t> dataBuffer,
                              virt_ptr<uint32_t> outSize,
                              uint32_t dataBufferSize);

uint32_t
DownloadedDataBase_GetAppDataSize(virt_ptr<const DownloadedDataBase> self);

nn::Result
DownloadedDataBase_GetBodyMemo(virt_ptr<const DownloadedDataBase> self,
                               virt_ptr<uint8_t> memoBuffer,
                               virt_ptr<uint32_t> outSize,
                               uint32_t memoBufferSize);

nn::Result
DownloadedDataBase_GetBodyText(virt_ptr<const DownloadedDataBase> self,
                               virt_ptr<char16_t> textBuffer,
                               uint32_t textBufferSize);

uint8_t
DownloadedDataBase_GetCountryId(virt_ptr<const DownloadedDataBase> self);

uint32_t
DownloadedDataBase_GetExternalBinaryDataSize(virt_ptr<const DownloadedDataBase> self);

uint32_t
DownloadedDataBase_GetExternalImageDataSize(virt_ptr<const DownloadedDataBase> self);

virt_ptr<const char>
DownloadedDataBase_GetExternalUrl(virt_ptr<const DownloadedDataBase> self);

uint8_t
DownloadedDataBase_GetFeeling(virt_ptr<const DownloadedDataBase> self);

uint8_t
DownloadedDataBase_GetLanguageId(virt_ptr<const DownloadedDataBase> self);

nn::Result
DownloadedDataBase_GetMiiData(virt_ptr<const DownloadedDataBase> self,
                              virt_ptr<nn::ffl::FFLStoreData> outData);

virt_ptr<nn::ffl::FFLStoreData>
DownloadedDataBase_GetMiiData(virt_ptr<const DownloadedDataBase> self);

virt_ptr<const char16_t>
DownloadedDataBase_GetMiiNickname(virt_ptr<const DownloadedDataBase> self);

uint8_t
DownloadedDataBase_GetPlatformId(virt_ptr<const DownloadedDataBase> self);

uint64_t
DownloadedDataBase_GetPostDate(virt_ptr<const DownloadedDataBase> self);

virt_ptr<const uint8_t>
DownloadedDataBase_GetPostId(virt_ptr<const DownloadedDataBase> self);

uint8_t
DownloadedDataBase_GetRegionId(virt_ptr<const DownloadedDataBase> self);

virt_ptr<const uint8_t>
DownloadedDataBase_GetTopicTag(virt_ptr<const DownloadedDataBase> self);

uint32_t
DownloadedDataBase_GetUserPid(virt_ptr<const DownloadedDataBase> self);

bool
DownloadedDataBase_TestFlags(virt_ptr<const DownloadedDataBase> self,
                             uint32_t flagMask);

}  // namespace cafe::nn_olv
