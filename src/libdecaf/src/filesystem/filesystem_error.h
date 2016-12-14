#pragma once

namespace fs
{

enum class Error
{
   //! Operation completed successfully.
   OK,

   //! Something bad happened, but not sure what.
   GenericError,

   //! Unsupported operation.
   UnsupportedOperation,

   //! Source not found.
   NotFound,

   //! Target already exists.
   AlreadyExists,

   //! Invalid permissions to perform requested operation.
   InvalidPermission,

   //! A function expected a file finds something else, e.g. a directory.
   NotFile,

   //! A function expected a directory finds something else, e.g. a file.
   NotDirectory,
};

} // namespace fs
