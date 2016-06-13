#pragma once
#include "filesystem_file.h"

namespace fs
{

class FileLink : public File
{
public:
   FileLink(File *file, const std::string &name) :
      File(name),
      mLink(file)
   {
      Node::isLink = true;
   }

   virtual ~FileLink() override = default;

   virtual FileHandle *open(OpenMode mode)  final override
   {
      return mLink->open(mode);
   }

private:
   File *mLink;
};

} // namespace fs
