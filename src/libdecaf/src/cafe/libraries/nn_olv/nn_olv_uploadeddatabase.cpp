#include "nn_olv.h"
#include "nn_olv_uploadeddatabase.h"

#include "cafe/libraries/ghs/cafe_ghs_malloc.h"
#include "nn/olv/nn_olv_result.h"

using namespace nn::olv;

namespace cafe::nn_olv
{

virt_ptr<ghs::VirtualTable> UploadedDataBase::VirtualTable = nullptr;
virt_ptr<ghs::TypeDescriptor> UploadedDataBase::TypeDescriptor = nullptr;


virt_ptr<UploadedDataBase>
UploadedDataBase_Constructor(virt_ptr<UploadedDataBase> self)
{
   if (!self) {
      self = virt_cast<UploadedDataBase *>(ghs::malloc(sizeof(UploadedDataBase)));
      if (!self) {
         return nullptr;
      }
   }

   self->virtualTable = UploadedDataBase::VirtualTable;
   self->flags = 0u;
   self->bodyTextLength = 0u;
   self->bodyMemoLength = 0u;
   self->appDataLength = 0u;
   self->feeling = int8_t { 0 };
   self->commonDataUnknown = 0u;
   self->commonDataLength = 0u;
   self->postID[0] = char { 0 };
   return self;
}

void
UploadedDataBase_Destructor(virt_ptr<UploadedDataBase> self,
                            ghs::DestructorFlags flags)
{
   if (!self) {
      return;
   }

   if (flags & ghs::DestructorFlags::FreeMemory) {
      ghs::free(self);
   }
}

uint32_t
UploadedDataBase_GetAppDataSize(virt_ptr<const UploadedDataBase> self)
{
   if (!UploadedDataBase_TestFlags(self, UploadedDataBase::HasAppData)) {
      return 0;
   }

   return self->appDataLength;
}

nn::Result
UploadedDataBase_GetAppData(virt_ptr<const UploadedDataBase> self,
                            virt_ptr<uint8_t> buffer,
                            virt_ptr<uint32_t> outDataSize,
                            uint32_t bufferSize)
{
   if (!UploadedDataBase_TestFlags(self, UploadedDataBase::HasAppData)) {
      return ResultNoData;
   }

   if (!buffer) {
      return ResultInvalidPointer;
   }

   if (!bufferSize) {
      return ResultInvalidSize;
   }

   auto length = std::min<uint32_t>(bufferSize, self->appDataLength);
   std::memcpy(buffer.get(),
               virt_addrof(self->appData).get(),
               length);

   if (outDataSize) {
      *outDataSize = length;
   }

   return ResultSuccess;
}

nn::Result
UploadedDataBase_GetBodyText(virt_ptr<const UploadedDataBase> self,
                             virt_ptr<char16_t> buffer,
                             uint32_t bufferSize)
{
   if (!UploadedDataBase_TestFlags(self, UploadedDataBase::HasBodyText)) {
      return ResultNoData;
   }

   if (!buffer) {
      return ResultInvalidPointer;
   }

   if (!bufferSize) {
      return ResultInvalidSize;
   }

   auto length = std::min<uint32_t>(bufferSize, self->bodyTextLength);
   std::memcpy(buffer.get(),
               virt_addrof(self->bodyText).get(),
               length * sizeof(char16_t));

   if (length < bufferSize) {
      buffer[length] = char16_t { 0 };
   }

   return ResultSuccess;
}

nn::Result
UploadedDataBase_GetBodyMemo(virt_ptr<const UploadedDataBase> self,
                             virt_ptr<uint8_t> buffer,
                             virt_ptr<uint32_t> outMemoSize,
                             uint32_t bufferSize)
{
   if (!UploadedDataBase_TestFlags(self, UploadedDataBase::HasBodyMemo)) {
      return ResultNoData;
   }

   if (!buffer) {
      return ResultInvalidPointer;
   }

   if (!bufferSize) {
      return ResultInvalidSize;
   }

   auto length = std::min<uint32_t>(bufferSize, self->bodyMemoLength);
   std::memcpy(buffer.get(),
               virt_addrof(self->bodyMemo).get(),
               length);

   if (outMemoSize) {
      *outMemoSize = length;
   }

   return ResultSuccess;
}

nn::Result
UploadedDataBase_GetCommonData(virt_ptr<const UploadedDataBase> self,
                               virt_ptr<uint32_t> unk,
                               virt_ptr<uint8_t> buffer,
                               virt_ptr<uint32_t> outDataSize,
                               uint32_t bufferSize)
{
   if (!self->commonDataLength) {
      return ResultNoData;
   }

   if (!buffer) {
      return ResultInvalidPointer;
   }

   if (!bufferSize) {
      return ResultInvalidSize;
   }

   auto length = std::min<uint32_t>(bufferSize, self->commonDataLength);
   std::memcpy(buffer.get(),
               virt_addrof(self->commonData).get(),
               length);

   if (unk) {
      *unk = self->commonDataUnknown;
   }

   if (outDataSize) {
      *outDataSize = length;
   }

   return ResultSuccess;
}

int32_t
UploadedDataBase_GetFeeling(virt_ptr<const UploadedDataBase> self)
{
   return self->feeling;
}

virt_ptr<const char>
UploadedDataBase_GetPostId(virt_ptr<const UploadedDataBase> self)
{
   return virt_addrof(self->postID);
}

bool
UploadedDataBase_TestFlags(virt_ptr<const UploadedDataBase> self,
                           uint32_t flag)
{
   return !!(self->flags & flag);
}

void
Library::registerUploadedDataBaseSymbols()
{
   RegisterFunctionExportName("__dt__Q3_2nn3olv16UploadedDataBaseFv",
                              UploadedDataBase_Destructor);
   RegisterFunctionExportName("GetCommonData__Q3_2nn3olv16UploadedDataBaseFPUiPUcT1Ui",
                              UploadedDataBase_GetCommonData);

   registerTypeInfo<UploadedDataBase>(
      "nn::olv::UploadedDataBase",
      {
         "__pure_virtual_called",
      });
}

}  // namespace cafe::nn_olv
