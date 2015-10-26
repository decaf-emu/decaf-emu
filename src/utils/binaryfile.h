#pragma once
#include <fstream>
#include "binaryview.h"

class BinaryFile : public BinaryView
{
public:
   BinaryFile()
   {
   }

   BinaryFile(const std::string &filename)
   {
      open(filename);
   }

   bool
   open(const std::string &filename)
   {
      std::ifstream file(filename, std::ifstream::binary | std::ifstream::in);

      if (!file.is_open()) {
         return false;
      }

      // Get size
      file.seekg(0, std::ifstream::end);
      auto size = static_cast<size_t>(file.tellg());
      file.seekg(0, std::ifstream::beg);

      // Read data
      mFileData.resize(size);
      file.read(reinterpret_cast<char*>(mFileData.data()), size);

      // Open binary view
      return BinaryView::open(mFileData);
   }

private:
   std::vector<uint8_t> mFileData;
};
