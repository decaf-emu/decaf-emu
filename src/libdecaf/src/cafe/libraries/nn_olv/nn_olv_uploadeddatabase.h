#pragma once
#include "cafe/libraries/ghs/cafe_ghs_typeinfo.h"
#include "nn/nn_result.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_olv
{

struct UploadedDataBase
{
   static virt_ptr<ghs::VirtualTable> VirtualTable;
   static virt_ptr<ghs::TypeDescriptor> TypeDescriptor;

   enum Flags
   {
      Empty          = 0,
      HasBodyText    = 1 << 0,
      HasBodyMemo    = 1 << 1,
      HasAppData     = 1 << 2,
   };

   be2_val<uint32_t> flags;
   be2_array<char, 32> postID;
   be2_array<char16_t, 256> bodyText;
   be2_val<uint32_t> bodyTextLength;
   be2_array<uint8_t, 0xA000> bodyMemo;
   be2_val<uint32_t> bodyMemoLength;
   be2_array<uint8_t, 1024> appData;
   be2_val<uint32_t> appDataLength;
   be2_val<int8_t> feeling;
   UNKNOWN(3);
   be2_val<uint32_t> commonDataUnknown;
   be2_val<uint32_t> commonDataLength;
   be2_array<uint8_t, 0x1000> commonData;
   UNKNOWN(0x9C8);
   be2_virt_ptr<ghs::VirtualTable> virtualTable;
};
CHECK_OFFSET(UploadedDataBase, 0x00, flags);
CHECK_OFFSET(UploadedDataBase, 0x04, postID);
CHECK_OFFSET(UploadedDataBase, 0x24, bodyText);
CHECK_OFFSET(UploadedDataBase, 0x224, bodyTextLength);
CHECK_OFFSET(UploadedDataBase, 0x228, bodyMemo);
CHECK_OFFSET(UploadedDataBase, 0xA228, bodyMemoLength);
CHECK_OFFSET(UploadedDataBase, 0xA22C, appData);
CHECK_OFFSET(UploadedDataBase, 0xA62C, appDataLength);
CHECK_OFFSET(UploadedDataBase, 0xA630, feeling);
CHECK_OFFSET(UploadedDataBase, 0xA634, commonDataUnknown);
CHECK_OFFSET(UploadedDataBase, 0xA638, commonDataLength);
CHECK_OFFSET(UploadedDataBase, 0xA63C, commonData);
CHECK_OFFSET(UploadedDataBase, 0xC004, virtualTable);
CHECK_SIZE(UploadedDataBase, 0xC008);

virt_ptr<UploadedDataBase>
UploadedDataBase_Constructor(virt_ptr<UploadedDataBase> self);

void
UploadedDataBase_Destructor(virt_ptr<UploadedDataBase> self,
                            ghs::DestructorFlags flags);

uint32_t
UploadedDataBase_GetAppDataSize(virt_ptr<const UploadedDataBase> self);

nn::Result
UploadedDataBase_GetAppData(virt_ptr<const UploadedDataBase> self,
                            virt_ptr<uint8_t> buffer,
                            virt_ptr<uint32_t> outDataSize,
                            uint32_t bufferSize);

nn::Result
UploadedDataBase_GetBodyText(virt_ptr<const UploadedDataBase> self,
                             virt_ptr<char16_t> buffer,
                             uint32_t bufferSize);

nn::Result
UploadedDataBase_GetBodyMemo(virt_ptr<const UploadedDataBase> self,
                             virt_ptr<uint8_t> buffer,
                             virt_ptr<uint32_t> outMemoSize,
                             uint32_t bufferSize);

nn::Result
UploadedDataBase_GetCommonData(virt_ptr<const UploadedDataBase> self,
                               virt_ptr<uint32_t> unk,
                               virt_ptr<uint8_t> buffer,
                               virt_ptr<uint32_t> outDataSize,
                               uint32_t bufferSize);

int32_t
UploadedDataBase_GetFeeling(virt_ptr<const UploadedDataBase> self);

virt_ptr<const char>
UploadedDataBase_GetPostId(virt_ptr<const UploadedDataBase> self);

bool
UploadedDataBase_TestFlags(virt_ptr<const UploadedDataBase> self,
                           uint32_t flag);

}  // namespace cafe::nn_olv
