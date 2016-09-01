#include "nn_olv.h"
#include "nn_olv_uploadeddatabase.h"

namespace nn
{

namespace olv
{

ghs::VirtualTableEntry *
UploadedDataBase::VirtualTable = nullptr;

ghs::TypeDescriptor *
UploadedDataBase::TypeInfo = nullptr;

UploadedDataBase::UploadedDataBase() :
   mFlags(0),
   mBodyTextLength(0),
   mBodyMemoLength(0),
   mAppDataLength(0),
   mFeeling(0),
   mCommonDataUnknown(0),
   mCommonDataLength(0)
{
   decaf_warn_stub();

   mPostID[0] = 0;
   mVirtualTable = UploadedDataBase::VirtualTable;
}

UploadedDataBase::~UploadedDataBase()
{
   decaf_warn_stub();
}

uint32_t
UploadedDataBase::GetAppDataSize()
{
   decaf_warn_stub();

   if (!TestFlags(HasAppData)) {
      return 0;
   }

   return mAppDataLength;
}

nn::Result
UploadedDataBase::GetAppData(uint8_t *buffer, be_val<uint32_t> *size, uint32_t bufferSize)
{
   decaf_warn_stub();

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

nn::Result
UploadedDataBase::GetBodyText(char16_t *buffer, uint32_t bufferSize)
{
   decaf_warn_stub();

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
   std::memcpy(buffer, mBodyText, length * sizeof(char16_t));

   if (length < bufferSize) {
      buffer[length] = 0;
   }

   return Success;
}

nn::Result
UploadedDataBase::GetBodyMemo(uint8_t *buffer, be_val<uint32_t> *size, uint32_t bufferSize)
{
   decaf_warn_stub();

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
   std::memcpy(buffer, mBodyMemo, length);

   if (size) {
      *size = length;
   }

   return Success;
}

nn::Result
UploadedDataBase::GetCommonData(uint32_t *unk, uint8_t *buffer, be_val<uint32_t> *size, uint32_t bufferSize)
{
   decaf_warn_stub();

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
   std::memcpy(buffer, mCommonData, length);

   if (unk) {
      *unk = mCommonDataUnknown;
   }

   if (size) {
      *size = length;
   }

   return Success;
}

int32_t
UploadedDataBase::GetFeeling()
{
   decaf_warn_stub();

   return mFeeling;
}

const char *
UploadedDataBase::GetPostId()
{
   decaf_warn_stub();

   return mPostID;
}

bool
UploadedDataBase::TestFlags(uint32_t flag)
{
   decaf_warn_stub();

   return !!(mFlags & flag);
}

void
Module::registerUploadedDataBase()
{
   RegisterKernelFunctionDestructor("__dt__Q3_2nn3olv16UploadedDataBaseFv", UploadedDataBase);
   RegisterKernelFunctionName("GetCommonData__Q3_2nn3olv16UploadedDataBaseFPUiPUcT1Ui", &UploadedDataBase::GetCommonData);
}

void
Module::initialiseUploadedDataBase()
{
   UploadedDataBase::TypeInfo = ghs::internal::makeTypeDescriptor("nn::olv::UploadedDataBase");

   UploadedDataBase::VirtualTable = ghs::internal::makeVirtualTable({
      { 0, UploadedDataBase::TypeInfo },
      { 0, ghs::PureVirtualCall },
   });
}

}  // namespace olv

}  // namespace nn
