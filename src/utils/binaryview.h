#pragma once
#include <cstring>
#include <exception>
#include <fstream>
#include <string>
#include <vector>
#include <array_view.h>

class BinaryView
{
public:
   BinaryView()
   {
   }

   BinaryView(uint8_t *buffer, std::size_t size) :
      mData(buffer, size)
   {
   }

   BinaryView(gsl::array_view<uint8_t> data) :
      mData(data)
   {
   }

   bool open(gsl::array_view<uint8_t> data)
   {
      mData = data;
      mPosition = 0;
      return true;
   }

   bool
   eof()
   {
      return mPosition >= mData.size();
   }

   void
   seek(std::size_t position)
   {
      mPosition = position;
   }

   std::size_t
   tell() const
   {
      return mPosition;
   }

   template<typename Type>
   Type
   read()
   {
      Type value;

      if (mPosition + sizeof(Type) > mData.size()) {
         throw std::runtime_error("Read past end of data");
      }

      std::memcpy(&value, &mData[mPosition], sizeof(Type));
      mPosition += sizeof(Type);
      return value;
   }

   template<typename Type>
   void
   read(Type &value)
   {
      if (mPosition + sizeof(Type) > mData.size()) {
         throw std::runtime_error("Read past end of data");
      }

      std::memcpy(&value, &mData[mPosition], sizeof(Type));
      mPosition += sizeof(Type);
   }

   gsl::array_view<uint8_t>
   readView(std::size_t size)
   {
      if (mPosition + size > mData.size()) {
         throw std::runtime_error("Read past end of data");
      }

      gsl::array_view<uint8_t> view { mData.data() + mPosition, size };
      mPosition += size;
      return view;
   }

   void
   read(uint8_t *buffer, std::size_t size)
   {
      if (mPosition + size > mData.size()) {
         throw std::runtime_error("Read past end of data");
      }

      std::memcpy(buffer, &mData[mPosition], size);
      mPosition += size;
   }

   void
   read(std::vector<uint8_t> &buffer, std::size_t size)
   {
      buffer.resize(size);
      read(buffer.data(), size);
   }

   operator gsl::array_view<uint8_t>() const
   {
      return gsl::array_view<uint8_t> { mData };
   }

private:
   gsl::array_view<uint8_t> mData;
   std::size_t mPosition = 0;
};
