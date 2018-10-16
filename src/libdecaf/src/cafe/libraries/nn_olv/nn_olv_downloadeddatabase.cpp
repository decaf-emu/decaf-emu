#include "nn_olv.h"
#include "nn_olv_downloadeddatabase.h"

#include "nn/olv/nn_olv_result.h"

#include <common/strutils.h>

using namespace nn::olv;
using nn::ffl::FFLStoreData;

namespace cafe::nn_olv
{

constexpr auto MinMemoBufferSize = 0x2582Cu;

virt_ptr<hle::VirtualTable>
DownloadedDataBase::VirtualTable = nullptr;

virt_ptr<hle::TypeDescriptor>
DownloadedDataBase::TypeDescriptor = nullptr;

DownloadedDataBase::DownloadedDataBase()
{
   std::memset(this, 0, sizeof(DownloadedDataBase));
   mVirtualTable = DownloadedDataBase::VirtualTable;
}

DownloadedDataBase::~DownloadedDataBase()
{
}

nn::Result
DownloadedDataBase::DownloadExternalBinaryData(virt_ptr<void> dataBuffer,
                                               virt_ptr<uint32_t> outDataSize,
                                               uint32_t dataBufferSize) const
{
   return ResultNotOnline;
}

nn::Result
DownloadedDataBase::DownloadExternalImageData(virt_ptr<void> dataBuffer,
                                              virt_ptr<uint32_t> outDataSize,
                                              uint32_t dataBufferSize) const
{
   return ResultNotOnline;
}

nn::Result
DownloadedDataBase::GetAppData(virt_ptr<uint32_t> dataBuffer,
                               virt_ptr<uint32_t> outSize,
                               uint32_t dataBufferSize) const
{
   if (!dataBuffer) {
      return ResultInvalidPointer;
   }

   if (!dataBufferSize) {
      return ResultInvalidSize;
   }

   if (!TestFlags(HasAppData)) {
      return ResultNoData;
   }

   auto copySize = std::min<uint32_t>(mAppDataSize, dataBufferSize);
   std::memcpy(dataBuffer.get(), virt_addrof(mAppData).get(), copySize);

   if (outSize) {
      *outSize = copySize;
   }

   return ResultSuccess;
}

uint32_t
DownloadedDataBase::GetAppDataSize() const
{
   if (TestFlags(HasAppData)) {
      return mAppDataSize;
   }

   return 0;
}

nn::Result
DownloadedDataBase::GetBodyMemo(virt_ptr<uint8_t> memoBuffer,
                                virt_ptr<uint32_t> outSize,
                                uint32_t memoBufferSize) const
{
   if (!memoBuffer) {
      return ResultInvalidPointer;
   }

   if (memoBufferSize < MinMemoBufferSize) {
      return ResultInvalidSize;
   }

   if (!TestFlags(HasBodyMemo)) {
      return ResultNoData;
   }

   // TODO: TGA uncompress mBodyMemo
   return ResultNoData;
}

nn::Result
DownloadedDataBase::GetBodyText(virt_ptr<char16_t> textBuffer,
                                uint32_t textBufferSize) const
{
   if (!textBuffer) {
      return ResultInvalidPointer;
   }

   if (!textBufferSize) {
      return ResultInvalidSize;
   }

   if (!TestFlags(HasBodyText)) {
      return ResultNoData;
   }

   char16_copy(textBuffer.getRawPointer(),
               textBufferSize,
               virt_addrof(mBodyText).getRawPointer(),
               mBodyTextLength);
   return ResultSuccess;
}

uint8_t
DownloadedDataBase::GetCountryId() const
{
   return mCountryId;
}

uint32_t
DownloadedDataBase::GetExternalBinaryDataSize() const
{
   if (TestFlags(HasExternalBinaryData)) {
      return mExternalBinaryDataSize;
   }

   return 0;
}

uint32_t
DownloadedDataBase::GetExternalImageDataSize() const
{
   if (TestFlags(HasExternalImageData)) {
      return mExternalImageDataSize;
   }

   return 0;
}

virt_ptr<const char>
DownloadedDataBase::GetExternalUrl() const
{
   if (TestFlags(HasExternalUrl)) {
      return virt_addrof(mExternalUrl);
   }

   return nullptr;
}

uint8_t
DownloadedDataBase::GetFeeling() const
{
   return mFeeling;
}

uint8_t
DownloadedDataBase::GetLanguageId() const
{
   return mLanguageId;
}

nn::Result
DownloadedDataBase::GetMiiData(virt_ptr<FFLStoreData> outData) const
{
   if (!TestFlags(HasMiiData)) {
      return ResultNoData;
   }

   if (!outData) {
      return ResultInvalidPointer;
   }

   std::memcpy(outData.get(),
               virt_addrof(mMiiData).get(),
               sizeof(FFLStoreData));
   return ResultSuccess;
}

virt_ptr<FFLStoreData>
DownloadedDataBase::GetMiiData() const
{
   if (TestFlags(HasMiiData)) {
      return virt_addrof(mMiiData);
   }

   return nullptr;
}

virt_ptr<const char16_t>
DownloadedDataBase::GetMiiNickname() const
{
   if (mMiiNickname[0]) {
      return virt_addrof(mMiiNickname);
   }

   return nullptr;
}

uint8_t
DownloadedDataBase::GetPlatformId() const
{
   return mPlatformId;
}

uint64_t
DownloadedDataBase::GetPostDate() const
{
   return mPostDate;
}

virt_ptr<const uint8_t>
DownloadedDataBase::GetPostId() const
{
   return virt_addrof(mPostId);
}

uint8_t
DownloadedDataBase::GetRegionId() const
{
   return mRegionId;
}

virt_ptr<const uint8_t>
DownloadedDataBase::GetTopicTag() const
{
   return virt_addrof(mTopicTag);
}

uint32_t
DownloadedDataBase::GetUserPid() const
{
   return mUserPid;
}

bool
DownloadedDataBase::TestFlags(uint32_t flagMask) const
{
   return (mFlags & flagMask) != 0;
}

void
Library::registerDownloadedDataBaseSymbols()
{
   RegisterConstructorExport("__ct__Q3_2nn3olv18DownloadedDataBaseFv",
                             DownloadedDataBase);
   RegisterDestructorExport("__dt__Q3_2nn3olv18DownloadedDataBaseFv",
                            DownloadedDataBase);
   RegisterFunctionExportName("DownloadExternalBinaryData__Q3_2nn3olv18DownloadedDataBaseCFPvPUiUi",
                              &DownloadedDataBase::DownloadExternalBinaryData);
   RegisterFunctionExportName("DownloadExternalImageData__Q3_2nn3olv18DownloadedDataBaseCFPvPUiUi",
                              &DownloadedDataBase::DownloadExternalImageData);
   RegisterFunctionExportName("GetAppDataSize__Q3_2nn3olv18DownloadedDataBaseCFv",
                              &DownloadedDataBase::GetAppDataSize);
   RegisterFunctionExportName("GetAppData__Q3_2nn3olv18DownloadedDataBaseCFPUcPUiUi",
                              &DownloadedDataBase::GetAppData);
   RegisterFunctionExportName("GetBodyMemo__Q3_2nn3olv18DownloadedDataBaseCFPUcPUiUi",
                              &DownloadedDataBase::GetBodyMemo);
   RegisterFunctionExportName("GetBodyText__Q3_2nn3olv18DownloadedDataBaseCFPwUi",
                              &DownloadedDataBase::GetBodyText);
   RegisterFunctionExportName("GetCountryId__Q3_2nn3olv18DownloadedDataBaseCFv",
                              &DownloadedDataBase::GetCountryId);
   RegisterFunctionExportName("GetExternalBinaryDataSize__Q3_2nn3olv18DownloadedDataBaseCFv",
                              &DownloadedDataBase::GetExternalBinaryDataSize);
   RegisterFunctionExportName("GetExternalImageDataSize__Q3_2nn3olv18DownloadedDataBaseCFv",
                              &DownloadedDataBase::GetExternalImageDataSize);
   RegisterFunctionExportName("GetExternalUrl__Q3_2nn3olv18DownloadedDataBaseCFv",
                              &DownloadedDataBase::GetExternalUrl);
   RegisterFunctionExportName("GetFeeling__Q3_2nn3olv18DownloadedDataBaseCFv",
                              &DownloadedDataBase::GetFeeling);
   RegisterFunctionExportName("GetLanguageId__Q3_2nn3olv18DownloadedDataBaseCFv",
                              &DownloadedDataBase::GetLanguageId);
   RegisterFunctionExportName("GetMiiData__Q3_2nn3olv18DownloadedDataBaseCFP12FFLStoreData",
                              static_cast<nn::Result (DownloadedDataBase::*)(virt_ptr<FFLStoreData>) const>(&DownloadedDataBase::GetMiiData));
   RegisterFunctionExportName("GetMiiData__Q3_2nn3olv18DownloadedDataBaseCFv",
                              static_cast<virt_ptr<FFLStoreData> (DownloadedDataBase::*)() const>(&DownloadedDataBase::GetMiiData));
   RegisterFunctionExportName("GetMiiNickname__Q3_2nn3olv18DownloadedDataBaseCFv",
                              &DownloadedDataBase::GetMiiNickname);
   RegisterFunctionExportName("GetPlatformId__Q3_2nn3olv18DownloadedDataBaseCFv",
                              &DownloadedDataBase::GetPlatformId);
   RegisterFunctionExportName("GetPostDate__Q3_2nn3olv18DownloadedDataBaseCFv",
                              &DownloadedDataBase::GetPostDate);
   RegisterFunctionExportName("GetPostId__Q3_2nn3olv18DownloadedDataBaseCFv",
                              &DownloadedDataBase::GetPostId);
   RegisterFunctionExportName("GetRegionId__Q3_2nn3olv18DownloadedDataBaseCFv",
                              &DownloadedDataBase::GetRegionId);
   RegisterFunctionExportName("GetTopicTag__Q3_2nn3olv18DownloadedDataBaseCFv",
                              &DownloadedDataBase::GetTopicTag);
   RegisterFunctionExportName("GetUserPid__Q3_2nn3olv18DownloadedDataBaseCFv",
                              &DownloadedDataBase::GetUserPid);
   RegisterFunctionExportName("TestFlags__Q3_2nn3olv18DownloadedDataBaseCFUi",
                              &DownloadedDataBase::TestFlags);

   registerTypeInfo<DownloadedDataBase>(
      "nn::olv::DownloadedDataBase",
      {
         "__pure_virtual_called",
      });
}

}  // namespace cafe::nn_olv
