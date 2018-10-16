#include "nn_olv.h"
#include "nn_olv_downloadedcommunitydata.h"

#include "nn/olv/nn_olv_result.h"

using namespace nn::olv;
using nn::ffl::FFLStoreData;

namespace cafe::nn_olv
{

DownloadedCommunityData::DownloadedCommunityData() :
   mFlags(0u),
   mCommunityId(0u),
   mOwnerPid(0u),
   mTitleTextLength(0u),
   mDescriptionTextLength(0u),
   mAppDataLength(0u),
   mIconDataLength(0u)
{
}

uint32_t
DownloadedCommunityData::GetAppDataSize()
{
   if (!TestFlags(HasAppData)) {
      return 0;
   }

   return mAppDataLength;
}

nn::Result
DownloadedCommunityData::GetAppData(virt_ptr<uint8_t> buffer,
                                    virt_ptr<uint32_t> outDataSize,
                                    uint32_t bufferSize)
{
   if (!TestFlags(HasAppData)) {
      return ResultNoData;
   }

   if (!buffer) {
      return ResultInvalidPointer;
   }

   if (!bufferSize) {
      return ResultInvalidSize;
   }

   auto length = std::min<uint32_t>(bufferSize, mAppDataLength);
   std::memcpy(buffer.get(), virt_addrof(mAppData).get(), length);

   if (outDataSize) {
      *outDataSize = length;
   }

   return ResultSuccess;
}

uint32_t
DownloadedCommunityData::GetCommunityId()
{
   return mCommunityId;
}

nn::Result
DownloadedCommunityData::GetDescriptionText(virt_ptr<char16_t> buffer,
                                            uint32_t bufferSize)
{
   if (!TestFlags(HasDescriptionText)) {
      return ResultNoData;
   }

   if (!buffer) {
      return ResultInvalidPointer;
   }

   if (!bufferSize) {
      return ResultInvalidSize;
   }

   auto length = std::min<uint32_t>(bufferSize, mDescriptionTextLength);
   std::memcpy(buffer.get(),
               virt_addrof(mDescriptionText).get(),
               length * sizeof(char16_t));

   if (length < bufferSize) {
      buffer[length] = char16_t { 0 };
   }

   return ResultSuccess;
}

nn::Result
DownloadedCommunityData::GetIconData(virt_ptr<uint8_t> buffer,
                                     virt_ptr<uint32_t> outIconSize,
                                     uint32_t bufferSize)
{
   if (!TestFlags(HasIconData)) {
      return ResultNoData;
   }

   if (!buffer) {
      return ResultInvalidPointer;
   }

   if (!bufferSize) {
      return ResultInvalidSize;
   }

   auto length = std::min<uint32_t>(bufferSize, mIconDataLength);
   std::memcpy(buffer.get(), virt_addrof(mIconData).get(), length);

   if (outIconSize) {
      *outIconSize = length;
   }

   return ResultSuccess;
}

nn::Result
DownloadedCommunityData::GetOwnerMiiData(virt_ptr<FFLStoreData> data)
{
   if (!TestFlags(HasOwnerMiiData)) {
      return ResultNoData;
   }

   if (!data) {
      return ResultInvalidPointer;
   }

   std::memcpy(data.get(),
               virt_addrof(mOwnerMiiData).get(),
               sizeof(FFLStoreData));
   return ResultSuccess;
}

virt_ptr<char16_t>
DownloadedCommunityData::GetOwnerMiiNickname()
{
   if (!mOwnerMiiNickname[0]) {
      return nullptr;
   }

   return virt_addrof(mOwnerMiiNickname);
}

uint32_t
DownloadedCommunityData::GetOwnerPid()
{
   return mOwnerPid;
}

nn::Result
DownloadedCommunityData::GetTitleText(virt_ptr<char16_t> buffer,
                                      uint32_t bufferSize)
{
   if (!TestFlags(HasTitleText)) {
      return ResultNoData;
   }

   if (!buffer) {
      return ResultInvalidPointer;
   }

   if (!bufferSize) {
      return ResultInvalidSize;
   }

   auto length = std::min<uint32_t>(bufferSize, mTitleTextLength);
   std::memcpy(buffer.get(),
               virt_addrof(mTitleText).get(),
               length * sizeof(char16_t));

   if (length < bufferSize) {
      buffer[length] = char16_t { 0 };
   }

   return ResultSuccess;
}

bool
DownloadedCommunityData::TestFlags(uint32_t flags)
{
   return !!(mFlags & flags);
}

void
Library::registerDownloadedCommunityDataSymbols()
{
   RegisterConstructorExport("__ct__Q3_2nn3olv23DownloadedCommunityDataFv",
                             DownloadedCommunityData);
   RegisterFunctionExportName("GetAppDataSize__Q3_2nn3olv23DownloadedCommunityDataCFv",
                              &DownloadedCommunityData::GetAppDataSize);
   RegisterFunctionExportName("GetAppData__Q3_2nn3olv23DownloadedCommunityDataCFPUcPUiUi",
                              &DownloadedCommunityData::GetAppData);
   RegisterFunctionExportName("GetCommunityId__Q3_2nn3olv23DownloadedCommunityDataCFv",
                              &DownloadedCommunityData::GetCommunityId);
   RegisterFunctionExportName("GetDescriptionText__Q3_2nn3olv23DownloadedCommunityDataCFPwUi",
                              &DownloadedCommunityData::GetDescriptionText);
   RegisterFunctionExportName("GetIconData__Q3_2nn3olv23DownloadedCommunityDataCFPUcPUiUi",
                              &DownloadedCommunityData::GetIconData);
   RegisterFunctionExportName("GetOwnerMiiData__Q3_2nn3olv23DownloadedCommunityDataCFP12FFLStoreData",
                              &DownloadedCommunityData::GetOwnerMiiData);
   RegisterFunctionExportName("GetOwnerMiiNickname__Q3_2nn3olv23DownloadedCommunityDataCFv",
                              &DownloadedCommunityData::GetOwnerMiiNickname);
   RegisterFunctionExportName("GetOwnerPid__Q3_2nn3olv23DownloadedCommunityDataCFv",
                              &DownloadedCommunityData::GetOwnerPid);
   RegisterFunctionExportName("GetTitleText__Q3_2nn3olv23DownloadedCommunityDataCFPwUi",
                              &DownloadedCommunityData::GetTitleText);
   RegisterFunctionExportName("TestFlags__Q3_2nn3olv23DownloadedCommunityDataCFUi",
                              &DownloadedCommunityData::TestFlags);
}

}  // namespace cafe::nn_olv
