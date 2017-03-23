#pragma once
#include "filesystem_file.h"
#include "filesystem_filehandle.h"
#include <cstdio>

namespace fs
{

struct HostFileHandle : public IFileHandle
{
   HostFileHandle(const std::string &path, File::OpenMode mode);

   virtual ~HostFileHandle() override
   {
      close();
   }

   virtual bool
   open() override;

   virtual void
   close() override;

   virtual bool
   eof() override;

   virtual bool
   flush() override;

   virtual bool
   seek(size_t position) override;

   virtual size_t
   size() override;

   virtual size_t
   tell() override;

   virtual size_t
   truncate() override;

   virtual size_t
   read(uint8_t *data,
        size_t size,
        size_t count) override;

   virtual size_t
   write(const uint8_t *data,
         size_t size,
         size_t count) override;

private:
   FILE *mHandle = nullptr;
   File::OpenMode mMode;
};

} // namespace fs
