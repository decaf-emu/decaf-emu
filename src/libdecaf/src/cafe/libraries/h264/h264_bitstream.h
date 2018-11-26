#pragma once
#include <cstdint>

namespace cafe::h264
{

class BitStream
{
public:
   BitStream(const uint8_t *buffer,
             size_t size) :
      mBuffer(buffer),
      mSize(size),
      mBytePosition(0),
      mBitPosition(0)
   {
   }

   uint8_t peekU1()
   {
      if (eof()) {
         return 0;
      }

      return static_cast<uint8_t>((mBuffer[mBytePosition] >> (7 - mBitPosition)) & 1);
   }

   uint8_t readU1()
   {
      if (eof()) {
         return 0;
      }

      auto value = static_cast<uint8_t>((mBuffer[mBytePosition] >> (7 - mBitPosition)) & 1);

      mBitPosition++;
      if (mBitPosition == 8) {
         mBitPosition = 0;
         mBytePosition++;
      }

      return value;
   }

   uint32_t readU(size_t n)
   {
      auto value = uint32_t { 0 };
      for (auto i = 0u; i < n; ++i) {
         value |= readU1() << (n - i - 1);
      }

      return value;
   }

   uint8_t readU2()
   {
      return static_cast<uint8_t>(readU(2));
   }

   uint8_t readU3()
   {
      return static_cast<uint8_t>(readU(3));
   }

   uint8_t readU4()
   {
      return static_cast<uint8_t>(readU(4));
   }

   uint8_t readU5()
   {
      return static_cast<uint8_t>(readU(5));
   }

   uint8_t readU8()
   {
      if (eof()) {
         return 0;
      }

      if (mBitPosition == 0) {
         return mBuffer[mBytePosition++];
      }

      return static_cast<uint8_t>(readU(8));
   }

   uint16_t readU16()
   {
      return static_cast<uint16_t>(readU(16));
   }

   uint32_t readUE()
   {
      auto r = uint32_t { 0 };
      auto i = 0;

      while (readU1() == 0 && i < 32 && !eof()) {
         i++;
      }

      r = readU(i);
      r += (1 << i) - 1;
      return r;
   }

   int32_t readSE()
   {
      auto r = static_cast<int32_t>(readUE());

      if (r & 0x01) {
         r = (r + 1) / 2;
      } else {
         r = -(r / 2);
      }

      return r;
   }

   bool byteAligned()
   {
      return mBitPosition == 0;
   }

   bool eof()
   {
      return mBytePosition >= mSize;
   }

   size_t bytesRead()
   {
      return mBytePosition;
   }

private:
   const uint8_t *mBuffer;
   size_t mSize;
   size_t mBytePosition;
   size_t mBitPosition;
};

} // namespace cafe::h264
