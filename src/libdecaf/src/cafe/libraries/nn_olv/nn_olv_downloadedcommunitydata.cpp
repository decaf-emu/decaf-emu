#include "nn_olv.h"
#include "nn_olv_downloadedcommunitydata.h"

#include "cafe/libraries/ghs/cafe_ghs_malloc.h"
#include "nn/olv/nn_olv_result.h"

using namespace nn::olv;
using nn::ffl::FFLStoreData;

namespace cafe::nn_olv
{

virt_ptr<DownloadedCommunityData>
DownloadedCommunityData_Constructor(virt_ptr<DownloadedCommunityData> self)
{
   if (!self) {
      self = virt_cast<DownloadedCommunityData *>(ghs::malloc(sizeof(DownloadedCommunityData)));
      if (!self) {
         return nullptr;
      }
   }

   std::memset(self.get(), 0, sizeof(DownloadedCommunityData));
   return self;
}

uint32_t
DownloadedCommunityData_GetAppDataSize(virt_ptr<DownloadedCommunityData> self)
{
   if (!DownloadedCommunityData_TestFlags(self, DownloadedCommunityData::HasAppData)) {
      return 0;
   }

   return self->appDataLength;
}

nn::Result
DownloadedCommunityData_GetAppData(virt_ptr<DownloadedCommunityData> self,
                                   virt_ptr<uint8_t> buffer,
                                   virt_ptr<uint32_t> outDataSize,
                                   uint32_t bufferSize)
{
   if (!DownloadedCommunityData_TestFlags(self, DownloadedCommunityData::HasAppData)) {
      return ResultNoData;
   }

   if (!buffer) {
      return ResultInvalidPointer;
   }

   if (!bufferSize) {
      return ResultInvalidSize;
   }

   auto length = std::min<uint32_t>(bufferSize, self->appDataLength);
   std::memcpy(buffer.get(), virt_addrof(self->appData).get(), length);

   if (outDataSize) {
      *outDataSize = length;
   }

   return ResultSuccess;
}

uint32_t
DownloadedCommunityData_GetCommunityId(virt_ptr<DownloadedCommunityData> self)
{
   return self->communityId;
}

nn::Result
DownloadedCommunityData_GetDescriptionText(virt_ptr<DownloadedCommunityData> self,
                                           virt_ptr<char16_t> buffer,
                                           uint32_t bufferSize)
{
   if (!DownloadedCommunityData_TestFlags(self, DownloadedCommunityData::HasDescriptionText)) {
      return ResultNoData;
   }

   if (!buffer) {
      return ResultInvalidPointer;
   }

   if (!bufferSize) {
      return ResultInvalidSize;
   }

   auto length = std::min<uint32_t>(bufferSize, self->descriptionTextLength);
   std::memcpy(buffer.get(),
               virt_addrof(self->descriptionText).get(),
               length * sizeof(char16_t));

   if (length < bufferSize) {
      buffer[length] = char16_t { 0 };
   }

   return ResultSuccess;
}

nn::Result
DownloadedCommunityData_GetIconData(virt_ptr<DownloadedCommunityData> self,
                                    virt_ptr<uint8_t> buffer,
                                    virt_ptr<uint32_t> outIconSize,
                                    uint32_t bufferSize)
{
   if (!DownloadedCommunityData_TestFlags(self, DownloadedCommunityData::HasIconData)) {
      return ResultNoData;
   }

   if (!buffer) {
      return ResultInvalidPointer;
   }

   if (!bufferSize) {
      return ResultInvalidSize;
   }

   auto length = std::min<uint32_t>(bufferSize, self->iconDataLength);
   std::memcpy(buffer.get(), virt_addrof(self->iconData).get(), length);

   if (outIconSize) {
      *outIconSize = length;
   }

   return ResultSuccess;
}

nn::Result
DownloadedCommunityData_GetOwnerMiiData(virt_ptr<DownloadedCommunityData> self,
                                        virt_ptr<nn::ffl::FFLStoreData> data)
{
   if (!DownloadedCommunityData_TestFlags(self, DownloadedCommunityData::HasOwnerMiiData)) {
      return ResultNoData;
   }

   if (!data) {
      return ResultInvalidPointer;
   }

   std::memcpy(data.get(),
               virt_addrof(self->ownerMiiData).get(),
               sizeof(FFLStoreData));
   return ResultSuccess;
}

virt_ptr<char16_t>
DownloadedCommunityData_GetOwnerMiiNickname(virt_ptr<DownloadedCommunityData> self)
{
   if (!self->ownerMiiNickname[0]) {
      return nullptr;
   }

   return virt_addrof(self->ownerMiiNickname);
}

uint32_t
DownloadedCommunityData_GetOwnerPid(virt_ptr<DownloadedCommunityData> self)
{
   return self->ownerPid;
}

nn::Result
DownloadedCommunityData_GetTitleText(virt_ptr<DownloadedCommunityData> self,
                                     virt_ptr<char16_t> buffer,
                                     uint32_t bufferSize)
{
   if (!DownloadedCommunityData_TestFlags(self, DownloadedCommunityData::HasTitleText)) {
      return ResultNoData;
   }

   if (!buffer) {
      return ResultInvalidPointer;
   }

   if (!bufferSize) {
      return ResultInvalidSize;
   }

   auto length = std::min<uint32_t>(bufferSize, self->titleTextLength);
   std::memcpy(buffer.get(),
               virt_addrof(self->titleText).get(),
               length * sizeof(char16_t));

   if (length < bufferSize) {
      buffer[length] = char16_t { 0 };
   }

   return ResultSuccess;
}

bool
DownloadedCommunityData_TestFlags(virt_ptr<DownloadedCommunityData> self,
                                  uint32_t flags)
{
   return !!(self->flags & flags);
}

void
Library::registerDownloadedCommunityDataSymbols()
{
   RegisterFunctionExportName("__ct__Q3_2nn3olv23DownloadedCommunityDataFv",
                              DownloadedCommunityData_Constructor);
   RegisterFunctionExportName("GetAppDataSize__Q3_2nn3olv23DownloadedCommunityDataCFv",
                              DownloadedCommunityData_GetAppDataSize);
   RegisterFunctionExportName("GetAppData__Q3_2nn3olv23DownloadedCommunityDataCFPUcPUiUi",
                              DownloadedCommunityData_GetAppData);
   RegisterFunctionExportName("GetCommunityId__Q3_2nn3olv23DownloadedCommunityDataCFv",
                              DownloadedCommunityData_GetCommunityId);
   RegisterFunctionExportName("GetDescriptionText__Q3_2nn3olv23DownloadedCommunityDataCFPwUi",
                              DownloadedCommunityData_GetDescriptionText);
   RegisterFunctionExportName("GetIconData__Q3_2nn3olv23DownloadedCommunityDataCFPUcPUiUi",
                              DownloadedCommunityData_GetIconData);
   RegisterFunctionExportName("GetOwnerMiiData__Q3_2nn3olv23DownloadedCommunityDataCFP12FFLStoreData",
                              DownloadedCommunityData_GetOwnerMiiData);
   RegisterFunctionExportName("GetOwnerMiiNickname__Q3_2nn3olv23DownloadedCommunityDataCFv",
                              DownloadedCommunityData_GetOwnerMiiNickname);
   RegisterFunctionExportName("GetOwnerPid__Q3_2nn3olv23DownloadedCommunityDataCFv",
                              DownloadedCommunityData_GetOwnerPid);
   RegisterFunctionExportName("GetTitleText__Q3_2nn3olv23DownloadedCommunityDataCFPwUi",
                              DownloadedCommunityData_GetTitleText);
   RegisterFunctionExportName("TestFlags__Q3_2nn3olv23DownloadedCommunityDataCFUi",
                              DownloadedCommunityData_TestFlags);
}

}  // namespace cafe::nn_olv
