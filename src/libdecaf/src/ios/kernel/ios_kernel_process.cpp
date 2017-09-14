#include "ios_kernel_process.h"
#include "ios_kernel_thread.h"
#include <cstring>

namespace ios::kernel
{

Result<ProcessID>
IOS_GetCurrentProcessID()
{
   auto thread = internal::getCurrentThread();
   if (!thread) {
      return Error::Invalid;
   }

   return ProcessID { thread->pid };
}

Error
IOS_GetProcessName(ProcessID process,
                   char *buffer)
{
   const char *name = nullptr;

   switch (process) {
   case ProcessID::KERNEL:
      name = "IOS-KERNEL";
      break;
   case ProcessID::MCP:
      name = "IOS-MCP";
      break;
   case ProcessID::BSP:
      name = "IOS-BSP";
      break;
   case ProcessID::CRYPTO:
      name = "IOS-CRYPTO";
      break;
   case ProcessID::USB:
      name = "IOS-USB";
      break;
   case ProcessID::FS:
      name = "IOS-FS";
      break;
   case ProcessID::PAD:
      name = "IOS-PAD";
      break;
   case ProcessID::NET:
      name = "IOS-NET";
      break;
   case ProcessID::ACP:
      name = "IOS-ACP";
      break;
   case ProcessID::NSEC:
      name = "IOS-NSEC";
      break;
   case ProcessID::AUXIL:
      name = "IOS-AUXIL";
      break;
   case ProcessID::NIM:
      name = "IOS-NIM";
      break;
   case ProcessID::FPD:
      name = "IOS-FPD";
      break;
   case ProcessID::IOSTEST:
      name = "IOS-TEST";
      break;
   case ProcessID::COSKERNEL:
      name = "COS-KERNEL";
      break;
   case ProcessID::COSROOT:
      name = "COS-ROOT";
      break;
   case ProcessID::COS02:
      name = "COS-02";
      break;
   case ProcessID::COS03:
      name = "COS-03";
      break;
   case ProcessID::COSOVERLAY:
      name = "COS-OVERLAY";
      break;
   case ProcessID::COSHBM:
      name = "COS-HBM";
      break;
   case ProcessID::COSERROR:
      name = "COS-ERROR";
      break;
   case ProcessID::COSMASTER:
      name = "COS-MASTER";
      break;
   default:
      return Error::Invalid;
   }

   strcpy(buffer, name);
   return Error::OK;
}

namespace internal
{

ProcessID
getCurrentProcessID()
{
   auto thread = getCurrentThread();
   if (!thread) {
      return ProcessID::KERNEL;
   }

   return thread->pid;
}

} // namespace internal

} // namespace ios::kernel
