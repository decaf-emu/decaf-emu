#include "nn_olv.h"
#include "nn_olv_uploadeddatabase.h"
#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::nn::olv
{

virt_ptr<hle::VirtualTable>
UploadedDataBase::VirtualTable = nullptr;

virt_ptr<hle::TypeDescriptor>
UploadedDataBase::TypeDescriptor = nullptr;

UploadedDataBase::UploadedDataBase() :
   mFlags(0u),
   mBodyTextLength(0u),
   mBodyMemoLength(0u),
   mAppDataLength(0u),
   mFeeling(int8_t { 0 }),
   mCommonDataUnknown(0u),
   mCommonDataLength(0u)
{
   mPostID[0] = char { 0 };
   mVirtualTable = UploadedDataBase::VirtualTable;
}

UploadedDataBase::~UploadedDataBase()
{
}

uint32_t
UploadedDataBase::GetAppDataSize()
{
   if (!TestFlags(HasAppData)) {
      return 0;
   }

   return mAppDataLength;
}

nn::Result
UploadedDataBase::GetAppData(virt_ptr<uint8_t> buffer,
                             virt_ptr<uint32_t> outDatasize,
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

   if (outDatasize) {
      *outDatasize = length;
   }

   return Success;
}

nn::Result
UploadedDataBase::GetBodyText(virt_ptr<char16_t> buffer,
                              uint32_t bufferSize)
{
   if (!TestFlags(HasBodyText)) {
      return NoData;
   }

   if (!buffer) {
      return InvalidPointer;
   }

   if (!bufferSize) {
      return InvalidSize;
   }

   auto length = std::min<uint32_t>(bufferSize, mBodyTextLength);
   std::memcpy(buffer.getRawPointer(),
               virt_addrof(mBodyText).getRawPointer(),
               length * sizeof(char16_t));

   if (length < bufferSize) {
      buffer[length] = char16_t { 0 };
   }

   return Success;
}

nn::Result
UploadedDataBase::GetBodyMemo(virt_ptr<uint8_t> buffer,
                              virt_ptr<uint32_t> outMemoSize,
                              uint32_t bufferSize)
{
   if (!TestFlags(HasBodyMemo)) {
      return NoData;
   }

   if (!buffer) {
      return InvalidPointer;
   }

   if (!bufferSize) {
      return InvalidSize;
   }

   auto length = std::min<uint32_t>(bufferSize, mBodyMemoLength);
   std::memcpy(buffer.getRawPointer(),
               virt_addrof(mBodyMemo).getRawPointer(),
               length);

   if (outMemoSize) {
      *outMemoSize = length;
   }

   return Success;
}

nn::Result
UploadedDataBase::GetCommonData(virt_ptr<uint32_t> unk,
                                virt_ptr<uint8_t> buffer,
                                virt_ptr<uint32_t> outDataSize,
                                uint32_t bufferSize)
{
   if (!mCommonDataLength) {
      return NoData;
   }

   if (!buffer) {
      return InvalidPointer;
   }

   if (!bufferSize) {
      return InvalidSize;
   }

   auto length = std::min<uint32_t>(bufferSize, mCommonDataLength);
   std::memcpy(buffer.getRawPointer(),
               virt_addrof(mCommonData).getRawPointer(),
               length);

   if (unk) {
      *unk = mCommonDataUnknown;
   }

   if (outDataSize) {
      *outDataSize = length;
   }

   return Success;
}

int32_t
UploadedDataBase::GetFeeling()
{
   return mFeeling;
}

virt_ptr<const char>
UploadedDataBase::GetPostId()
{
   return virt_addrof(mPostID);
}

bool
UploadedDataBase::TestFlags(uint32_t flag)
{
   return !!(mFlags & flag);
}

void
Library::registerUploadedDataBaseSymbols()
{
   RegisterDestructorExport("__dt__Q3_2nn3olv16UploadedDataBaseFv",
                            UploadedDataBase);
   RegisterFunctionExportName("GetCommonData__Q3_2nn3olv16UploadedDataBaseFPUiPUcT1Ui",
                              &UploadedDataBase::GetCommonData);

   registerTypeInfo<UploadedDataBase>(
      "nn::olv::UploadedDataBase",
      {
         "__pure_virtual_called",
      });
}

}  // namespace cafe::nn::olv
