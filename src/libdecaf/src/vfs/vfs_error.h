#pragma once
#include <fmt/format.h>

namespace vfs
{

enum class Error
{
   Success = 0,

   AlreadyExists,
   EndOfDirectory,
   EndOfFile,
   ExecutePermission,
   GenericError,
   InvalidPath,
   InvalidSeekDirection,
   InvalidSeekPosition,
   InvalidTruncatePosition,
   NotDirectory,
   NotFile,
   NotFound,
   NotMountDevice,
   NotOpen,
   OperationNotSupported,
   ReadOnly,
   Permission,
};

} // namespace vfs

template <>
struct fmt::formatter<vfs::Error> :
   fmt::formatter<std::string_view>
{
   template <typename FormatContext>
   auto format(vfs::Error value, FormatContext& ctx) {
      std::string_view name = "unknown";
      switch (value) {
      case vfs::Error::Success:
         name = "Success";
         break;
      case vfs::Error::AlreadyExists:
         name = "AlreadyExists";
         break;
      case vfs::Error::EndOfDirectory:
         name = "EndOfDirectory";
         break;
      case vfs::Error::EndOfFile:
         name = "EndOfFile";
         break;
      case vfs::Error::ExecutePermission:
         name = "ExecutePermission";
         break;
      case vfs::Error::GenericError:
         name = "GenericError";
         break;
      case vfs::Error::InvalidPath:
         name = "InvalidPath";
         break;
      case vfs::Error::InvalidSeekDirection:
         name = "InvalidSeekDirection";
         break;
      case vfs::Error::InvalidSeekPosition:
         name = "InvalidSeekPosition";
         break;
      case vfs::Error::InvalidTruncatePosition:
         name = "InvalidTruncatePosition";
         break;
      case vfs::Error::NotDirectory:
         name = "NotDirectory";
         break;
      case vfs::Error::NotFile:
         name = "NotFile";
         break;
      case vfs::Error::NotFound:
         name = "NotFound";
         break;
      case vfs::Error::NotMountDevice:
         name = "NotMountDevice";
         break;
      case vfs::Error::NotOpen:
         name = "NotOpen";
         break;
      case vfs::Error::OperationNotSupported:
         name = "OperationNotSupported";
         break;
      case vfs::Error::ReadOnly:
         name = "ReadOnly";
         break;
      case vfs::Error::Permission:
         name = "Permission";
         break;
      }
      return fmt::formatter<string_view>::format(name, ctx);
   }
};
