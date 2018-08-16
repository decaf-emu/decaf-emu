#pragma once
#include "nn_olv_result.h"
#include "cafe/libraries/nn_ffl.h"
#include <libcpu/be2_struct.h>

/*
Unimplemented functions:
   nn::olv::DownloadedCommunityData::GetCommunityCode(char *, unsigned int) const
   GetCommunityCode__Q3_2nn3olv23DownloadedCommunityDataCFPcUi
*/

namespace cafe::nn::olv
{

class DownloadedCommunityData
{
public:
   enum Flags
   {
      Empty                = 0,
      HasTitleText         = 1 << 0,
      HasDescriptionText   = 1 << 1,
      HasAppData           = 1 << 2,
      HasIconData          = 1 << 3,
      HasOwnerMiiData      = 1 << 4,
   };

public:
   DownloadedCommunityData();

   uint32_t
   GetAppDataSize();

   nn::Result
   GetAppData(virt_ptr<uint8_t> buffer,
              virt_ptr<uint32_t> outDataSize,
              uint32_t bufferSize);

   uint32_t
   GetCommunityId();

   nn::Result
   GetDescriptionText(virt_ptr<char16_t> buffer,
                      uint32_t bufferSize);

   nn::Result
   GetIconData(virt_ptr<uint8_t> buffer,
               virt_ptr<uint32_t> outIconSize,
               uint32_t bufferSize);

   nn::Result
   GetOwnerMiiData(virt_ptr<FFLStoreData> data);

   virt_ptr<char16_t>
   GetOwnerMiiNickname();

   uint32_t
   GetOwnerPid();

   nn::Result
   GetTitleText(virt_ptr<char16_t> buffer,
                uint32_t bufferSize);

   bool
   TestFlags(uint32_t flags);

protected:
   be2_val<uint32_t> mFlags;
   be2_val<uint32_t> mCommunityId;
   be2_val<uint32_t> mOwnerPid;
   be2_array<char16_t, 128> mTitleText;
   be2_val<uint32_t> mTitleTextLength;
   be2_array<char16_t, 256> mDescriptionText;
   be2_val<uint32_t> mDescriptionTextLength;
   be2_array<uint8_t, 1024> mAppData;
   be2_val<uint32_t> mAppDataLength;
   be2_array<uint8_t, 0x1002C> mIconData;
   be2_val<uint32_t> mIconDataLength;
   be2_array<uint8_t, 96> mOwnerMiiData;
   be2_array<char16_t, 32> mOwnerMiiNickname;
   UNKNOWN(0x1818);

private:
   CHECK_MEMBER_OFFSET_START
   CHECK_OFFSET(DownloadedCommunityData, 0x00, mFlags);
   CHECK_OFFSET(DownloadedCommunityData, 0x04, mCommunityId);
   CHECK_OFFSET(DownloadedCommunityData, 0x08, mOwnerPid);
   CHECK_OFFSET(DownloadedCommunityData, 0x0C, mTitleText);
   CHECK_OFFSET(DownloadedCommunityData, 0x10C, mTitleTextLength);
   CHECK_OFFSET(DownloadedCommunityData, 0x110, mDescriptionText);
   CHECK_OFFSET(DownloadedCommunityData, 0x310, mDescriptionTextLength);
   CHECK_OFFSET(DownloadedCommunityData, 0x314, mAppData);
   CHECK_OFFSET(DownloadedCommunityData, 0x714, mAppDataLength);
   CHECK_OFFSET(DownloadedCommunityData, 0x718, mIconData);
   CHECK_OFFSET(DownloadedCommunityData, 0x10744, mIconDataLength);
   CHECK_OFFSET(DownloadedCommunityData, 0x10748, mOwnerMiiData);
   CHECK_OFFSET(DownloadedCommunityData, 0x107A8, mOwnerMiiNickname);
   CHECK_MEMBER_OFFSET_END
};
CHECK_SIZE(DownloadedCommunityData, 0x12000);

}  // namespace cafe::nn::olv
