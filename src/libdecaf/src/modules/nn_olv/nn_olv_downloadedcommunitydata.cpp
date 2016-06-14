#include "nn_olv.h"
#include "nn_olv_downloadedcommunitydata.h"

namespace nn
{

namespace olv
{

DownloadedCommunityData::DownloadedCommunityData() :
   mFlags(0),
   mCommunityId(0),
   mOwnerPid(0),
   mTitleTextLength(0),
   mDescriptionTextLength(0),
   mAppDataLength(0),
   mIconDataLength(0)
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
DownloadedCommunityData::GetAppData(uint8_t *buffer, be_val<uint32_t> *size, uint32_t bufferSize)
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
   std::memcpy(buffer, mAppData, length);

   if (size) {
      *size = length;
   }

   return Success;
}

uint32_t
DownloadedCommunityData::GetCommunityId()
{
   return mCommunityId;
}

nn::Result
DownloadedCommunityData::GetDescriptionText(char16_t *buffer, uint32_t bufferSize)
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
   std::memcpy(buffer, mDescriptionText, length * sizeof(char16_t));

   if (length < bufferSize) {
      buffer[length] = 0;
   }

   return Success;
}

nn::Result
DownloadedCommunityData::GetIconData(uint8_t *buffer, be_val<uint32_t> *size, uint32_t bufferSize)
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
   std::memcpy(buffer, mIconData, length);

   if (size) {
      *size = length;
   }

   return Success;
}

nn::Result
DownloadedCommunityData::GetOwnerMiiData(FFLStoreData *data)
{
   if (!TestFlags(HasOwnerMiiData)) {
      return NoData;
   }

   if (!data) {
      return InvalidPointer;
   }

   std::memcpy(data, mOwnerMiiData, sizeof(FFLStoreData));
   return Success;
}

char16_t *
DownloadedCommunityData::GetOwnerMiiNickname()
{
   if (!mOwnerMiiNickname[0]) {
      return nullptr;
   }

   return mOwnerMiiNickname;
}

uint32_t
DownloadedCommunityData::GetOwnerPid()
{
   return mOwnerPid;
}

nn::Result
DownloadedCommunityData::GetTitleText(char16_t *buffer, uint32_t bufferSize)
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
   std::memcpy(buffer, mTitleText, length * sizeof(char16_t));

   if (length < bufferSize) {
      buffer[length] = 0;
   }

   return Success;
}

bool
DownloadedCommunityData::TestFlags(uint32_t flags)
{
   return !!(mFlags & flags);
}

void
Module::registerDownloadedCommunityData()
{
   RegisterKernelFunctionConstructor("__ct__Q3_2nn3olv23DownloadedCommunityDataFv", DownloadedCommunityData);
   RegisterKernelFunctionName("GetAppDataSize__Q3_2nn3olv23DownloadedCommunityDataCFv", &DownloadedCommunityData::GetAppDataSize);
   RegisterKernelFunctionName("GetAppData__Q3_2nn3olv23DownloadedCommunityDataCFPUcPUiUi", &DownloadedCommunityData::GetAppData);
   RegisterKernelFunctionName("GetCommunityId__Q3_2nn3olv23DownloadedCommunityDataCFv", &DownloadedCommunityData::GetCommunityId);
   RegisterKernelFunctionName("GetDescriptionText__Q3_2nn3olv23DownloadedCommunityDataCFPwUi", &DownloadedCommunityData::GetDescriptionText);
   RegisterKernelFunctionName("GetIconData__Q3_2nn3olv23DownloadedCommunityDataCFPUcPUiUi", &DownloadedCommunityData::GetIconData);
   RegisterKernelFunctionName("GetOwnerMiiData__Q3_2nn3olv23DownloadedCommunityDataCFP12FFLStoreData", &DownloadedCommunityData::GetOwnerMiiData);
   RegisterKernelFunctionName("GetOwnerMiiNickname__Q3_2nn3olv23DownloadedCommunityDataCFv", &DownloadedCommunityData::GetOwnerMiiNickname);
   RegisterKernelFunctionName("GetOwnerPid__Q3_2nn3olv23DownloadedCommunityDataCFv", &DownloadedCommunityData::GetOwnerPid);
   RegisterKernelFunctionName("GetTitleText__Q3_2nn3olv23DownloadedCommunityDataCFPwUi", &DownloadedCommunityData::GetTitleText);
   RegisterKernelFunctionName("TestFlags__Q3_2nn3olv23DownloadedCommunityDataCFUi", &DownloadedCommunityData::TestFlags);
}

}  // namespace olv

}  // namespace nn
