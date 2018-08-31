#include "nn_olv.h"
#include "nn_olv_uploadedpostdata.h"
#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::nn::olv
{

virt_ptr<hle::VirtualTable>
UploadedPostData::VirtualTable = nullptr;

virt_ptr<hle::TypeDescriptor>
UploadedPostData::TypeDescriptor = nullptr;

UploadedPostData::UploadedPostData()
{
   mVirtualTable = UploadedPostData::VirtualTable;
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
UploadedPostData::GetAppData(virt_ptr<uint8_t> buffer,
                             virt_ptr<uint32_t> outDataSize,
                             uint32_t bufferSize)
{
   return UploadedDataBase::GetAppData(buffer, outDataSize, bufferSize);
}

nn::Result
UploadedPostData::GetBodyText(virt_ptr<char16_t> buffer,
                              uint32_t bufferSize)
{
   return UploadedDataBase::GetBodyText(buffer, bufferSize);
}

nn::Result
UploadedPostData::GetBodyMemo(virt_ptr<uint8_t> buffer,
                              virt_ptr<uint32_t> outMemoSize,
                              uint32_t bufferSize)
{
   return UploadedDataBase::GetBodyMemo(buffer, outMemoSize, bufferSize);
}

int32_t
UploadedPostData::GetFeeling()
{
   return UploadedDataBase::GetFeeling();
}

virt_ptr<const char>
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
Library::registerUploadedPostDataSymbols()
{
   RegisterConstructorExport("__ct__Q3_2nn3olv16UploadedPostDataFv",
                             UploadedPostData);
   RegisterDestructorExport("__dt__Q3_2nn3olv16UploadedPostDataFv",
                            UploadedPostData);
   RegisterFunctionExportName("GetAppDataSize__Q3_2nn3olv16UploadedPostDataCFv",
                              &UploadedPostData::GetAppDataSize);
   RegisterFunctionExportName("GetAppData__Q3_2nn3olv16UploadedPostDataCFPUcPUiUi",
                              &UploadedPostData::GetAppData);
   RegisterFunctionExportName("GetBodyMemo__Q3_2nn3olv16UploadedPostDataCFPUcPUiUi",
                              &UploadedPostData::GetBodyMemo);
   RegisterFunctionExportName("GetBodyText__Q3_2nn3olv16UploadedPostDataCFPwUi",
                              &UploadedPostData::GetBodyText);
   RegisterFunctionExportName("GetFeeling__Q3_2nn3olv16UploadedPostDataCFv",
                              &UploadedPostData::GetFeeling);
   RegisterFunctionExportName("GetPostId__Q3_2nn3olv16UploadedPostDataCFv",
                              &UploadedPostData::GetPostId);
   RegisterFunctionExportName("TestFlags__Q3_2nn3olv16UploadedPostDataCFUi",
                              &UploadedPostData::TestFlags);

   registerTypeInfo<UploadedPostData>(
      "nn::olv::UploadedPostData",
      {
         "__dt__Q3_2nn3olv16UploadedPostDataFv",
      },
      {
         "nn::olv::UploadedDataBase",
      });
}

}  // namespace cafe::nn::olv
