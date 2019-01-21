#pragma once

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
   NotOpen,
   OperationNotSupported,
   ReadOnly,
   ReadPermission,
   WritePermission,
};

} // namespace vfs
