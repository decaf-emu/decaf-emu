#include "sci_spot_pass_settings.h"
#include "modules/coreinit/coreinit_userconfig.h"

#include <common/count_of.h>

using coreinit::UCDataType;
using coreinit::UCError;
using coreinit::UCSysConfig;

namespace sci
{

namespace internal
{

static SCIError
sciCheckSpotPassSettings(SCISpotPassSettings *data)
{
   auto result = SCIError::OK;

   if (data->enable > 1u) {
      data->enable = uint8_t { 1 };
      result = SCIError::Error;
   }

   if (data->auto_dl_app > 1u) {
      data->auto_dl_app = uint8_t { 1 };
      result = SCIError::Error;
   }

   for (auto &upload_console_info : data->upload_console_info) {
      if (upload_console_info > 1u) {
         upload_console_info = uint8_t { 1 };
         result = SCIError::Error;
      }
   }

   return result;
}

} // namespace internal

SCIError
SCIInitSpotPassSettings(SCISpotPassSettings *data)
{
   auto result = coreinit::UCOpen();
   if (result < 0) {
      return SCIError::Error;
   }

   auto handle = static_cast<coreinit::IOSHandle>(result);
   std::memset(data, 0, sizeof(SCISpotPassSettings));

   UCSysConfig settings[] = {
      { "spotpass",                       0x777, UCDataType::Complex, UCError::OK, 0, nullptr },
      { "spotpass.version",               0x777, UCDataType::UnsignedByte, UCError::OK, sizeof(data->version),                   virt_addrof(data->version) },
      { "spotpass.enable",                0x777, UCDataType::UnsignedByte, UCError::OK, sizeof(data->enable),                    virt_addrof(data->enable) },
      { "spotpass.auto_dl_app",           0x777, UCDataType::UnsignedByte, UCError::OK, sizeof(data->auto_dl_app),               virt_addrof(data->auto_dl_app) },
      { "spotpass.upload_console_info1",  0x777, UCDataType::UnsignedByte, UCError::OK, sizeof(data->upload_console_info[0]),    virt_addrof(data->upload_console_info[0]) },
      { "spotpass.upload_console_info2",  0x777, UCDataType::UnsignedByte, UCError::OK, sizeof(data->upload_console_info[1]),    virt_addrof(data->upload_console_info[1]) },
      { "spotpass.upload_console_info3",  0x777, UCDataType::UnsignedByte, UCError::OK, sizeof(data->upload_console_info[2]),    virt_addrof(data->upload_console_info[2]) },
      { "spotpass.upload_console_info4",  0x777, UCDataType::UnsignedByte, UCError::OK, sizeof(data->upload_console_info[3]),    virt_addrof(data->upload_console_info[3]) },
      { "spotpass.upload_console_info5",  0x777, UCDataType::UnsignedByte, UCError::OK, sizeof(data->upload_console_info[4]),    virt_addrof(data->upload_console_info[4]) },
      { "spotpass.upload_console_info6",  0x777, UCDataType::UnsignedByte, UCError::OK, sizeof(data->upload_console_info[5]),    virt_addrof(data->upload_console_info[5]) },
      { "spotpass.upload_console_info7",  0x777, UCDataType::UnsignedByte, UCError::OK, sizeof(data->upload_console_info[6]),    virt_addrof(data->upload_console_info[6]) },
      { "spotpass.upload_console_info8",  0x777, UCDataType::UnsignedByte, UCError::OK, sizeof(data->upload_console_info[7]),    virt_addrof(data->upload_console_info[7]) },
      { "spotpass.upload_console_info9",  0x777, UCDataType::UnsignedByte, UCError::OK, sizeof(data->upload_console_info[8]),    virt_addrof(data->upload_console_info[8]) },
      { "spotpass.upload_console_info10", 0x777, UCDataType::UnsignedByte, UCError::OK, sizeof(data->upload_console_info[9]),    virt_addrof(data->upload_console_info[9]) },
      { "spotpass.upload_console_info11", 0x777, UCDataType::UnsignedByte, UCError::OK, sizeof(data->upload_console_info[10]),   virt_addrof(data->upload_console_info[10]) },
      { "spotpass.upload_console_info12", 0x777, UCDataType::UnsignedByte, UCError::OK, sizeof(data->upload_console_info[11]),   virt_addrof(data->upload_console_info[11]) },
   };

   result = coreinit::UCReadSysConfig(handle, COUNT_OF(settings), settings);
   if (result != UCError::OK) {
      if (result != UCError::KeyNotFound) {
         coreinit::UCDeleteSysConfig(handle, 1, settings);

         data->version = uint8_t { 2 };
         data->enable = uint8_t { 1 };
         data->auto_dl_app = uint8_t { 1 };
         data->upload_console_info.fill(1u);
      }

      result = coreinit::UCWriteSysConfig(handle, COUNT_OF(settings), settings);
      if (result != UCError::OK) {
         coreinit::UCClose(handle);
         return SCIError::WriteError;
      }
   }

   if (!internal::sciCheckSpotPassSettings(data)) {
      result = coreinit::UCWriteSysConfig(handle, COUNT_OF(settings), settings);
      if (result != UCError::OK) {
         coreinit::UCClose(handle);
         return SCIError::WriteError;
      }
   }

   coreinit::UCClose(handle);
   return SCIError::OK;
}

} // namespace sci
