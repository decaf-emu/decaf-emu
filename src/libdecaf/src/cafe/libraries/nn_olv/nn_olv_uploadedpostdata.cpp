#include "nn_olv.h"
#include "nn_olv_uploadedpostdata.h"

#include "cafe/libraries/ghs/cafe_ghs_malloc.h"

namespace cafe::nn_olv
{

virt_ptr<ghs::VirtualTable> UploadedPostData::VirtualTable = nullptr;
virt_ptr<ghs::TypeDescriptor> UploadedPostData::TypeDescriptor = nullptr;

virt_ptr<UploadedPostData>
UploadedPostData_Constructor(virt_ptr<UploadedPostData> self)
{
   if (!self) {
      self = virt_cast<UploadedPostData *>(ghs::malloc(sizeof(UploadedPostData)));
      if (!self) {
         return nullptr;
      }
   }

   UploadedDataBase_Constructor(virt_cast<UploadedDataBase *>(self));
   self->virtualTable = UploadedPostData::VirtualTable;
   return self;
}

void
UploadedPostData_Destructor(virt_ptr<UploadedPostData> self,
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
UploadedPostData_GetAppDataSize(virt_ptr<const UploadedPostData> self)
{
   return UploadedDataBase_GetAppDataSize(virt_cast<const UploadedDataBase *>(self));
}

nn::Result
UploadedPostData_GetAppData(virt_ptr<const UploadedPostData> self,
                            virt_ptr<uint8_t> buffer,
                            virt_ptr<uint32_t> outDataSize,
                            uint32_t bufferSize)
{
   return UploadedDataBase_GetAppData(virt_cast<const UploadedDataBase *>(self),
                                      buffer, outDataSize, bufferSize);
}

nn::Result
UploadedPostData_GetBodyText(virt_ptr<const UploadedPostData> self,
                             virt_ptr<char16_t> buffer,
                             uint32_t bufferSize)
{
   return UploadedDataBase_GetBodyText(virt_cast<const UploadedDataBase *>(self),
                                       buffer, bufferSize);
}

nn::Result
UploadedPostData_GetBodyMemo(virt_ptr<const UploadedPostData> self,
                             virt_ptr<uint8_t> buffer,
                             virt_ptr<uint32_t> outMemoSize,
                             uint32_t bufferSize)
{
   return UploadedDataBase_GetBodyMemo(virt_cast<const UploadedDataBase *>(self),
                                       buffer, outMemoSize, bufferSize);
}

int32_t
UploadedPostData_GetFeeling(virt_ptr<const UploadedPostData> self)
{
   return UploadedDataBase_GetFeeling(virt_cast<const UploadedDataBase *>(self));
}

virt_ptr<const char>
UploadedPostData_GetPostId(virt_ptr<const UploadedPostData> self)
{
   return UploadedDataBase_GetPostId(virt_cast<const UploadedDataBase *>(self));
}

bool
UploadedPostData_TestFlags(virt_ptr<const UploadedPostData> self,
                           uint32_t flag)
{
   return UploadedDataBase_TestFlags(virt_cast<const UploadedDataBase *>(self),
                                     flag);
}

void
Library::registerUploadedPostDataSymbols()
{
   RegisterFunctionExportName("__ct__Q3_2nn3olv16UploadedPostDataFv",
                              UploadedPostData_Constructor);
   RegisterFunctionExportName("__dt__Q3_2nn3olv16UploadedPostDataFv",
                              UploadedPostData_Destructor);
   RegisterFunctionExportName("GetAppDataSize__Q3_2nn3olv16UploadedPostDataCFv",
                              UploadedPostData_GetAppDataSize);
   RegisterFunctionExportName("GetAppData__Q3_2nn3olv16UploadedPostDataCFPUcPUiUi",
                              UploadedPostData_GetAppData);
   RegisterFunctionExportName("GetBodyMemo__Q3_2nn3olv16UploadedPostDataCFPUcPUiUi",
                              UploadedPostData_GetBodyMemo);
   RegisterFunctionExportName("GetBodyText__Q3_2nn3olv16UploadedPostDataCFPwUi",
                              UploadedPostData_GetBodyText);
   RegisterFunctionExportName("GetFeeling__Q3_2nn3olv16UploadedPostDataCFv",
                              UploadedPostData_GetFeeling);
   RegisterFunctionExportName("GetPostId__Q3_2nn3olv16UploadedPostDataCFv",
                              UploadedPostData_GetPostId);
   RegisterFunctionExportName("TestFlags__Q3_2nn3olv16UploadedPostDataCFUi",
                              UploadedPostData_TestFlags);

   registerTypeInfo<UploadedPostData>(
      "nn::olv::UploadedPostData",
      {
         "__dt__Q3_2nn3olv16UploadedPostDataFv",
      },
      {
         "nn::olv::UploadedDataBase",
      });
}

}  // namespace cafe::nn_olv
