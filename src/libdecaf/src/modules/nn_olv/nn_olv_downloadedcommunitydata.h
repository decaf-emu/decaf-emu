#pragma once
#include "nn_olv.h"
#include "nn_olv_result.h"
#include "modules/nn_ffl.h"
#include "modules/nn_result.h"
#include <common/be_val.h>
#include <common/structsize.h>

/*
Unimplemented functions:
   nn::olv::DownloadedCommunityData::GetCommunityCode(char *, unsigned int) const
   GetCommunityCode__Q3_2nn3olv23DownloadedCommunityDataCFPcUi
*/

namespace nn
{

namespace olv
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
   GetAppData(uint8_t *buffer, be_val<uint32_t> *size, uint32_t bufferSize);

   uint32_t
   GetCommunityId();

   nn::Result
   GetDescriptionText(char16_t *buffer, uint32_t bufferSize);

   nn::Result
   GetIconData(uint8_t *buffer, be_val<uint32_t> *size, uint32_t bufferSize);

   nn::Result
   GetOwnerMiiData(FFLStoreData *data);

   char16_t *
   GetOwnerMiiNickname();

   uint32_t
   GetOwnerPid();

   nn::Result
   GetTitleText(char16_t *buffer, uint32_t bufferSize);

   bool
   TestFlags(uint32_t flags);

protected:
   be_val<uint32_t> mFlags;
   be_val<uint32_t> mCommunityId;
   be_val<uint32_t> mOwnerPid;
   char16_t mTitleText[128];
   be_val<uint32_t> mTitleTextLength;
   char16_t mDescriptionText[256];
   be_val<uint32_t> mDescriptionTextLength;
   uint8_t mAppData[1024];
   be_val<uint32_t> mAppDataLength;
   uint8_t mIconData[0x1002C];
   be_val<uint32_t> mIconDataLength;
   uint8_t mOwnerMiiData[96];
   char16_t mOwnerMiiNickname[32];
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

}  // namespace olv

}  // namespace nn
