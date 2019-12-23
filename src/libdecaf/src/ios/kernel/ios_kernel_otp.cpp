#include "ios_kernel_enum.h"
#include "ios_kernel_thread.h"
#include "ios_kernel_otp.h"

#include "decaf_config.h"

#include <array>
#include <cstddef>
#include <common/log.h>
#include <fstream>

namespace ios::kernel
{

static bool sOtpLoaded = false;
static std::array<std::byte, 0x400> sOtpData;

Error
IOS_ReadOTP(OtpFieldIndex fieldIndex,
            phys_ptr<uint32_t> buffer,
            uint32_t bufferSize)
{
   auto thread = internal::getCurrentThread();
   if (thread->pid != ProcessId::CRYPTO) {
      return Error::Access;
   }

   if (!sOtpLoaded) {
      return Error::NoExists;
   }

   auto offset = fieldIndex * 4;
   if (offset + bufferSize > sOtpData.size()) {
      return Error::Invalid;
   }

   std::memcpy(buffer.get(), sOtpData.data() + offset, bufferSize);
   return Error::OK;
}

namespace internal
{

Error
initialiseOtp()
{
   auto config = decaf::config();
   if (!config->system.otp_path.empty()) {
      std::ifstream fh { config->system.otp_path, std::fstream::binary };
      if (fh.is_open()) {
         fh.read(reinterpret_cast<char *>(sOtpData.data()), sOtpData.size());
         if (fh) {
            sOtpLoaded = true;
         }
      }
   }

   if (!sOtpLoaded) {
      gLog->warn("Failed to load otp.bin from {}", config->system.otp_path);
      return Error::NoExists;
   }

   return Error::OK;
}

} // namespace internal

} // namespace ios::kernel
