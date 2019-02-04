#pragma once
#include "nn/nn_result.h"
#include "nn/ffl/nn_ffl_miidata.h"

#include <libcpu/be2_struct.h>

/*
Unimplemented functions:
   nn::olv::DownloadedCommunityData::GetCommunityCode(char *, unsigned int) const
   GetCommunityCode__Q3_2nn3olv23DownloadedCommunityDataCFPcUi
*/

namespace cafe::nn_olv
{

struct DownloadedCommunityData
{
   enum Flags
   {
      Empty                = 0,
      HasTitleText         = 1 << 0,
      HasDescriptionText   = 1 << 1,
      HasAppData           = 1 << 2,
      HasIconData          = 1 << 3,
      HasOwnerMiiData      = 1 << 4,
   };

   be2_val<uint32_t> flags;
   be2_val<uint32_t> communityId;
   be2_val<uint32_t> ownerPid;
   be2_array<char16_t, 128> titleText;
   be2_val<uint32_t> titleTextLength;
   be2_array<char16_t, 256> descriptionText;
   be2_val<uint32_t> descriptionTextLength;
   be2_array<uint8_t, 1024> appData;
   be2_val<uint32_t> appDataLength;
   be2_array<uint8_t, 0x1002C> iconData;
   be2_val<uint32_t> iconDataLength;
   be2_array<uint8_t, 96> ownerMiiData;
   be2_array<char16_t, 32> ownerMiiNickname;
   UNKNOWN(0x1818);
};
CHECK_OFFSET(DownloadedCommunityData, 0x00, flags);
CHECK_OFFSET(DownloadedCommunityData, 0x04, communityId);
CHECK_OFFSET(DownloadedCommunityData, 0x08, ownerPid);
CHECK_OFFSET(DownloadedCommunityData, 0x0C, titleText);
CHECK_OFFSET(DownloadedCommunityData, 0x10C, titleTextLength);
CHECK_OFFSET(DownloadedCommunityData, 0x110, descriptionText);
CHECK_OFFSET(DownloadedCommunityData, 0x310, descriptionTextLength);
CHECK_OFFSET(DownloadedCommunityData, 0x314, appData);
CHECK_OFFSET(DownloadedCommunityData, 0x714, appDataLength);
CHECK_OFFSET(DownloadedCommunityData, 0x718, iconData);
CHECK_OFFSET(DownloadedCommunityData, 0x10744, iconDataLength);
CHECK_OFFSET(DownloadedCommunityData, 0x10748, ownerMiiData);
CHECK_OFFSET(DownloadedCommunityData, 0x107A8, ownerMiiNickname);
CHECK_SIZE(DownloadedCommunityData, 0x12000);

virt_ptr<DownloadedCommunityData>
DownloadedCommunityData_Constructor(virt_ptr<DownloadedCommunityData> self);

uint32_t
DownloadedCommunityData_GetAppDataSize(virt_ptr<DownloadedCommunityData> self);

nn::Result
DownloadedCommunityData_GetAppData(virt_ptr<DownloadedCommunityData> self,
                                   virt_ptr<uint8_t> buffer,
                                   virt_ptr<uint32_t> outDataSize,
                                   uint32_t bufferSize);

uint32_t
DownloadedCommunityData_GetCommunityId(virt_ptr<DownloadedCommunityData> self);

nn::Result
DownloadedCommunityData_GetDescriptionText(virt_ptr<DownloadedCommunityData> self,
                                           virt_ptr<char16_t> buffer,
                                           uint32_t bufferSize);

nn::Result
DownloadedCommunityData_GetIconData(virt_ptr<DownloadedCommunityData> self,
                                    virt_ptr<uint8_t> buffer,
                                    virt_ptr<uint32_t> outIconSize,
                                    uint32_t bufferSize);

nn::Result
DownloadedCommunityData_GetOwnerMiiData(virt_ptr<DownloadedCommunityData> self,
                                        virt_ptr<nn::ffl::FFLStoreData> data);

virt_ptr<char16_t>
DownloadedCommunityData_GetOwnerMiiNickname(virt_ptr<DownloadedCommunityData> self);

uint32_t
DownloadedCommunityData_GetOwnerPid(virt_ptr<DownloadedCommunityData> self);

nn::Result
DownloadedCommunityData_GetTitleText(virt_ptr<DownloadedCommunityData> self,
                                     virt_ptr<char16_t> buffer,
                                     uint32_t bufferSize);

bool
DownloadedCommunityData_TestFlags(virt_ptr<DownloadedCommunityData> self,
                                  uint32_t flags);

}  // namespace cafe::nn_olv
