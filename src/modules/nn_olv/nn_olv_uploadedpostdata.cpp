#include "nn_olv.h"
#include "nn_olv_uploadedpostdata.h"

namespace nn
{

namespace olv
{

ghs::VirtualTableEntry *
UploadedPostData::VirtualTable = nullptr;

ghs::TypeDescriptor *
UploadedPostData::TypeInfo = nullptr;

UploadedPostData::UploadedPostData()
{
}

UploadedPostData::~UploadedPostData()
{
}

uint32_t
UploadedPostData::GetAppDataSize()
{
   return UploadedDataBase::GetAppDataSize();
}

nn::Result
UploadedPostData::GetAppData(uint8_t *buffer, be_val<uint32_t> *size, uint32_t bufferSize)
{
   return UploadedDataBase::GetAppData(buffer, size, bufferSize);
}

nn::Result
UploadedPostData::GetBodyText(char16_t *buffer, uint32_t bufferSize)
{
   return UploadedDataBase::GetBodyText(buffer, bufferSize);
}

nn::Result
UploadedPostData::GetBodyMemo(uint8_t *buffer, be_val<uint32_t> *size, uint32_t bufferSize)
{
   return UploadedDataBase::GetBodyMemo(buffer, size, bufferSize);
}

int32_t
UploadedPostData::GetFeeling()
{
   return UploadedDataBase::GetFeeling();
}

const char *
UploadedPostData::GetPostId()
{
   return UploadedDataBase::GetPostId();
}

bool
UploadedPostData::TestFlags(uint32_t flag)
{
   return UploadedDataBase::TestFlags(flag);
}

void
Module::registerUploadedPostData()
{
   RegisterKernelFunctionConstructor("__ct__Q3_2nn3olv16UploadedPostDataFv", UploadedPostData);
   RegisterKernelFunctionDestructor("__dt__Q3_2nn3olv16UploadedPostDataFv", UploadedPostData);
   RegisterKernelFunctionName("GetAppDataSize__Q3_2nn3olv16UploadedPostDataCFv", &UploadedPostData::GetAppDataSize);
   RegisterKernelFunctionName("GetAppData__Q3_2nn3olv16UploadedPostDataCFPUcPUiUi", &UploadedPostData::GetAppData);
   RegisterKernelFunctionName("GetBodyMemo__Q3_2nn3olv16UploadedPostDataCFPUcPUiUi", &UploadedPostData::GetBodyMemo);
   RegisterKernelFunctionName("GetBodyText__Q3_2nn3olv16UploadedPostDataCFPwUi", &UploadedPostData::GetBodyText);
   RegisterKernelFunctionName("GetFeeling__Q3_2nn3olv16UploadedPostDataCFv", &UploadedPostData::GetFeeling);
   RegisterKernelFunctionName("GetPostId__Q3_2nn3olv16UploadedPostDataCFv", &UploadedPostData::GetPostId);
   RegisterKernelFunctionName("TestFlags__Q3_2nn3olv16UploadedPostDataCFUi", &UploadedPostData::TestFlags);
}

void
Module::initialiseUploadedPostData()
{
   UploadedPostData::TypeInfo = ghs::internal::makeTypeDescriptor("nn::olv::UploadedPostData", {
      { UploadedDataBase::TypeInfo, 0x1600 },
   });

   UploadedPostData::VirtualTable = ghs::internal::makeVirtualTable({
      { 0, UploadedPostData::TypeInfo },
      { 0, findExportAddress("__dt__Q3_2nn3olv16UploadedPostDataFv") },
   });
}

}  // namespace olv

}  // namespace nn
