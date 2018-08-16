#include "nn_olv.h"
#include "nn_olv_downloadedcommunitydata.h"

namespace cafe::nn::olv
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
      return NoData;
   }

   if (!buffer) {
      return InvalidPointer;
   }

   if (!bufferSize) {
      return InvalidSize;
   }

   auto length = std::min<uint32_t>(bufferSize, mAppDataLength);
   std::memcpy(buffer.getRawPointer(),
               virt_addrof(mAppData).getRawPointer(),
               length);

   if (outDataSize) {
      *outDataSize = length;
   }

   return Success;
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
      return NoData;
   }

   if (!buffer) {
      return InvalidPointer;
   }

   if (!bufferSize) {
      return InvalidSize;
   }

   auto length = std::min<uint32_t>(bufferSize, mDescriptionTextLength);
   std::memcpy(buffer.getRawPointer(),
               virt_addrof(mDescriptionText).getRawPointer(),
               length * sizeof(char16_t));

   if (length < bufferSize) {
      buffer[length] = char16_t { 0 };
   }

   return Success;
}

nn::Result
DownloadedCommunityData::GetIconData(virt_ptr<uint8_t> buffer,
                                     virt_ptr<uint32_t> outIconSize,
                                     uint32_t bufferSize)
{
   if (!TestFlags(HasIconData)) {
      return NoData;
   }

   if (!buffer) {
      return InvalidPointer;
   }

   if (!bufferSize) {
      return InvalidSize;
   }

   auto length = std::min<uint32_t>(bufferSize, mIconDataLength);
   std::memcpy(buffer.getRawPointer(),
               virt_addrof(mIconData).getRawPointer(),
               length);

   if (outIconSize) {
      *outIconSize = length;
   }

   return Success;
}

nn::Result
DownloadedCommunityData::GetOwnerMiiData(virt_ptr<FFLStoreData> data)
{
   if (!TestFlags(HasOwnerMiiData)) {
      return NoData;
   }

   if (!data) {
      return InvalidPointer;
   }

   std::memcpy(data.getRawPointer(),
               virt_addrof(mOwnerMiiData).getRawPointer(),
               sizeof(FFLStoreData));
   return Success;
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
      return NoData;
   }

   if (!buffer) {
      return InvalidPointer;
   }

   if (!bufferSize) {
      return InvalidSize;
   }

   auto length = std::min<uint32_t>(bufferSize, mTitleTextLength);
   std::memcpy(buffer.getRawPointer(),
               virt_addrof(mTitleText).getRawPointer(),
               length * sizeof(char16_t));

   if (length < bufferSize) {
      buffer[length] = char16_t { 0 };
   }

   return Success;
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

}  // namespace cafe::nn::olv
