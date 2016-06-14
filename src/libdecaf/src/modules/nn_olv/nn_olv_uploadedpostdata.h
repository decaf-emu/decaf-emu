#pragma once
#include "nn_olv_uploadeddatabase.h"

namespace nn
{

namespace olv
{

class UploadedPostData : public UploadedDataBase
{
public:
   static ghs::VirtualTableEntry *VirtualTable;
   static ghs::TypeDescriptor *TypeInfo;

public:
   UploadedPostData();
   ~UploadedPostData();

   uint32_t
   GetAppDataSize();

   nn::Result
   GetAppData(uint8_t *buffer, be_val<uint32_t> *size, uint32_t bufferSize);

   nn::Result
   GetBodyText(char16_t *buffer, uint32_t bufferSize);

   nn::Result
   GetBodyMemo(uint8_t *buffer, be_val<uint32_t> *size, uint32_t bufferSize);

   int32_t
   GetFeeling();

   const char *
   GetPostId();

   bool
   TestFlags(uint32_t flag);

private:
   UNKNOWN(0x200);
};
CHECK_SIZE(UploadedPostData, 0xC208);

} // namespace olv

} // namespace nn
