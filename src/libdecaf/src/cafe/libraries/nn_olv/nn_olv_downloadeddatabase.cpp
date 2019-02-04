#include "nn_olv.h"
#include "nn_olv_downloadeddatabase.h"

#include "cafe/libraries/ghs/cafe_ghs_malloc.h"
#include "nn/olv/nn_olv_result.h"

#include <common/strutils.h>

using namespace nn::olv;
using nn::ffl::FFLStoreData;

namespace cafe::nn_olv
{

constexpr auto MinMemoBufferSize = 0x2582Cu;

virt_ptr<ghs::VirtualTable> DownloadedDataBase::VirtualTable = nullptr;
virt_ptr<ghs::TypeDescriptor> DownloadedDataBase::TypeDescriptor = nullptr;


virt_ptr<DownloadedDataBase>
DownloadedDataBase_Constructor(virt_ptr<DownloadedDataBase> self)
{
   if (!self) {
      self = virt_cast<DownloadedDataBase *>(ghs::malloc(sizeof(DownloadedDataBase)));
      if (!self) {
         return nullptr;
      }
   }

   std::memset(self.get(), 0, sizeof(DownloadedDataBase));
   self->virtualTable = DownloadedDataBase::VirtualTable;
   return self;
}

void
DownloadedDataBase_Destructor(virt_ptr<DownloadedDataBase> self,
                              ghs::DestructorFlags flags)
{
   if (!self) {
      return;
   }

   if (flags & ghs::DestructorFlags::FreeMemory) {
      ghs::free(self);
   }
}

nn::Result
DownloadedDataBase_DownloadExternalBinaryData(virt_ptr<const DownloadedDataBase> self,
                                              virt_ptr<void> dataBuffer,
                                              virt_ptr<uint32_t> outDataSize,
                                              uint32_t dataBufferSize)
{
   return ResultNotOnline;
}

nn::Result
DownloadedDataBase_DownloadExternalImageData(virt_ptr<const DownloadedDataBase> self,
                                             virt_ptr<void> dataBuffer,
                                             virt_ptr<uint32_t> outDataSize,
                                             uint32_t dataBufferSize)
{
   return ResultNotOnline;
}

nn::Result
DownloadedDataBase_GetAppData(virt_ptr<const DownloadedDataBase> self,
                              virt_ptr<uint32_t> dataBuffer,
                              virt_ptr<uint32_t> outSize,
                              uint32_t dataBufferSize)
{
   if (!dataBuffer) {
      return ResultInvalidPointer;
   }

   if (!dataBufferSize) {
      return ResultInvalidSize;
   }

   if (!DownloadedDataBase_TestFlags(self, DownloadedDataBase::HasAppData)) {
      return ResultNoData;
   }

   auto copySize = std::min<uint32_t>(self->appDataSize, dataBufferSize);
   std::memcpy(dataBuffer.get(), virt_addrof(self->appData).get(), copySize);

   if (outSize) {
      *outSize = copySize;
   }

   return ResultSuccess;
}

uint32_t
DownloadedDataBase_GetAppDataSize(virt_ptr<const DownloadedDataBase> self)
{
   if (DownloadedDataBase_TestFlags(self, DownloadedDataBase::HasAppData)) {
      return self->appDataSize;
   }

   return 0;
}

nn::Result
DownloadedDataBase_GetBodyMemo(virt_ptr<const DownloadedDataBase> self,
                               virt_ptr<uint8_t> memoBuffer,
                               virt_ptr<uint32_t> outSize,
                               uint32_t memoBufferSize)
{
   if (!memoBuffer) {
      return ResultInvalidPointer;
   }

   if (memoBufferSize < MinMemoBufferSize) {
      return ResultInvalidSize;
   }

   if (!DownloadedDataBase_TestFlags(self, DownloadedDataBase::HasBodyMemo)) {
      return ResultNoData;
   }

   // TODO: TGA uncompress mBodyMemo
   return ResultNoData;
}

nn::Result
DownloadedDataBase_GetBodyText(virt_ptr<const DownloadedDataBase> self,
                               virt_ptr<char16_t> textBuffer,
                               uint32_t textBufferSize)
{
   if (!textBuffer) {
      return ResultInvalidPointer;
   }

   if (!textBufferSize) {
      return ResultInvalidSize;
   }

   if (!DownloadedDataBase_TestFlags(self, DownloadedDataBase::HasBodyText)) {
      return ResultNoData;
   }

   char16_copy(textBuffer.getRawPointer(),
               textBufferSize,
               virt_addrof(self->bodyText).getRawPointer(),
               self->bodyTextLength);
   return ResultSuccess;
}

uint8_t
DownloadedDataBase_GetCountryId(virt_ptr<const DownloadedDataBase> self)
{
   return self->countryId;
}

uint32_t
DownloadedDataBase_GetExternalBinaryDataSize(virt_ptr<const DownloadedDataBase> self)
{
   if (DownloadedDataBase_TestFlags(self, DownloadedDataBase::HasExternalBinaryData)) {
      return self->externalBinaryDataSize;
   }

   return 0;
}

uint32_t
DownloadedDataBase_GetExternalImageDataSize(virt_ptr<const DownloadedDataBase> self)
{
   if (DownloadedDataBase_TestFlags(self, DownloadedDataBase::HasExternalImageData)) {
      return self->externalImageDataSize;
   }

   return 0;
}

virt_ptr<const char>
DownloadedDataBase_GetExternalUrl(virt_ptr<const DownloadedDataBase> self)
{
   if (DownloadedDataBase_TestFlags(self, DownloadedDataBase::HasExternalUrl)) {
      return virt_addrof(self->externalUrl);
   }

   return nullptr;
}

uint8_t
DownloadedDataBase_GetFeeling(virt_ptr<const DownloadedDataBase> self)
{
   return self->feeling;
}

uint8_t
DownloadedDataBase_GetLanguageId(virt_ptr<const DownloadedDataBase> self)
{
   return self->languageId;
}

nn::Result
DownloadedDataBase_GetMiiData(virt_ptr<const DownloadedDataBase> self,
                              virt_ptr<FFLStoreData> outData)
{
   if (!DownloadedDataBase_TestFlags(self, DownloadedDataBase::HasMiiData)) {
      return ResultNoData;
   }

   if (!outData) {
      return ResultInvalidPointer;
   }

   std::memcpy(outData.get(),
               virt_addrof(self->miiData).get(),
               sizeof(FFLStoreData));
   return ResultSuccess;
}

virt_ptr<nn::ffl::FFLStoreData>
DownloadedDataBase_GetMiiData(virt_ptr<const DownloadedDataBase> self)
{
   if (DownloadedDataBase_TestFlags(self, DownloadedDataBase::HasMiiData)) {
      return virt_addrof(self->miiData);
   }

   return nullptr;
}

virt_ptr<const char16_t>
DownloadedDataBase_GetMiiNickname(virt_ptr<const DownloadedDataBase> self)
{
   if (self->miiNickname[0]) {
      return virt_addrof(self->miiNickname);
   }

   return nullptr;
}

uint8_t
DownloadedDataBase_GetPlatformId(virt_ptr<const DownloadedDataBase> self)
{
   return self->platformId;
}

uint64_t
DownloadedDataBase_GetPostDate(virt_ptr<const DownloadedDataBase> self)
{
   return self->postDate;
}

virt_ptr<const uint8_t>
DownloadedDataBase_GetPostId(virt_ptr<const DownloadedDataBase> self)
{
   return virt_addrof(self->postId);
}

uint8_t
DownloadedDataBase_GetRegionId(virt_ptr<const DownloadedDataBase> self)
{
   return self->regionId;
}

virt_ptr<const uint8_t>
DownloadedDataBase_GetTopicTag(virt_ptr<const DownloadedDataBase> self)
{
   return virt_addrof(self->topicTag);
}

uint32_t
DownloadedDataBase_GetUserPid(virt_ptr<const DownloadedDataBase> self)
{
   return self->userPid;
}

bool
DownloadedDataBase_TestFlags(virt_ptr<const DownloadedDataBase> self,
                             uint32_t flagMask)
{
   return (self->flags & flagMask) != 0;
}

void
Library::registerDownloadedDataBaseSymbols()
{
   RegisterFunctionExportName("__ct__Q3_2nn3olv18DownloadedDataBaseFv",
                              DownloadedDataBase_Constructor);
   RegisterFunctionExportName("__dt__Q3_2nn3olv18DownloadedDataBaseFv",
                              DownloadedDataBase_Destructor);
   RegisterFunctionExportName("DownloadExternalBinaryData__Q3_2nn3olv18DownloadedDataBaseCFPvPUiUi",
                              DownloadedDataBase_DownloadExternalBinaryData);
   RegisterFunctionExportName("DownloadExternalImageData__Q3_2nn3olv18DownloadedDataBaseCFPvPUiUi",
                              DownloadedDataBase_DownloadExternalImageData);
   RegisterFunctionExportName("GetAppDataSize__Q3_2nn3olv18DownloadedDataBaseCFv",
                              DownloadedDataBase_GetAppDataSize);
   RegisterFunctionExportName("GetAppData__Q3_2nn3olv18DownloadedDataBaseCFPUcPUiUi",
                              DownloadedDataBase_GetAppData);
   RegisterFunctionExportName("GetBodyMemo__Q3_2nn3olv18DownloadedDataBaseCFPUcPUiUi",
                              DownloadedDataBase_GetBodyMemo);
   RegisterFunctionExportName("GetBodyText__Q3_2nn3olv18DownloadedDataBaseCFPwUi",
                              DownloadedDataBase_GetBodyText);
   RegisterFunctionExportName("GetCountryId__Q3_2nn3olv18DownloadedDataBaseCFv",
                              DownloadedDataBase_GetCountryId);
   RegisterFunctionExportName("GetExternalBinaryDataSize__Q3_2nn3olv18DownloadedDataBaseCFv",
                              DownloadedDataBase_GetExternalBinaryDataSize);
   RegisterFunctionExportName("GetExternalImageDataSize__Q3_2nn3olv18DownloadedDataBaseCFv",
                              DownloadedDataBase_GetExternalImageDataSize);
   RegisterFunctionExportName("GetExternalUrl__Q3_2nn3olv18DownloadedDataBaseCFv",
                              DownloadedDataBase_GetExternalUrl);
   RegisterFunctionExportName("GetFeeling__Q3_2nn3olv18DownloadedDataBaseCFv",
                              DownloadedDataBase_GetFeeling);
   RegisterFunctionExportName("GetLanguageId__Q3_2nn3olv18DownloadedDataBaseCFv",
                              DownloadedDataBase_GetLanguageId);
   RegisterFunctionExportName("GetMiiData__Q3_2nn3olv18DownloadedDataBaseCFP12FFLStoreData",
                              static_cast<nn::Result (*)(virt_ptr<const DownloadedDataBase>, virt_ptr<FFLStoreData>)>(DownloadedDataBase_GetMiiData));
   RegisterFunctionExportName("GetMiiData__Q3_2nn3olv18DownloadedDataBaseCFv",
                              static_cast<virt_ptr<FFLStoreData> (*)(virt_ptr<const DownloadedDataBase>)>(DownloadedDataBase_GetMiiData));
   RegisterFunctionExportName("GetMiiNickname__Q3_2nn3olv18DownloadedDataBaseCFv",
                              DownloadedDataBase_GetMiiNickname);
   RegisterFunctionExportName("GetPlatformId__Q3_2nn3olv18DownloadedDataBaseCFv",
                              DownloadedDataBase_GetPlatformId);
   RegisterFunctionExportName("GetPostDate__Q3_2nn3olv18DownloadedDataBaseCFv",
                              DownloadedDataBase_GetPostDate);
   RegisterFunctionExportName("GetPostId__Q3_2nn3olv18DownloadedDataBaseCFv",
                              DownloadedDataBase_GetPostId);
   RegisterFunctionExportName("GetRegionId__Q3_2nn3olv18DownloadedDataBaseCFv",
                              DownloadedDataBase_GetRegionId);
   RegisterFunctionExportName("GetTopicTag__Q3_2nn3olv18DownloadedDataBaseCFv",
                              DownloadedDataBase_GetTopicTag);
   RegisterFunctionExportName("GetUserPid__Q3_2nn3olv18DownloadedDataBaseCFv",
                              DownloadedDataBase_GetUserPid);
   RegisterFunctionExportName("TestFlags__Q3_2nn3olv18DownloadedDataBaseCFUi",
                              DownloadedDataBase_TestFlags);

   registerTypeInfo<DownloadedDataBase>(
      "nn::olv::DownloadedDataBase",
      {
         "__pure_virtual_called",
      });
}

}  // namespace cafe::nn_olv
