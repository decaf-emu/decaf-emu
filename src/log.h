#pragma once
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>

class Log
{
public:
   struct hex
   {
      template<typename Type>
      hex(Type number, unsigned pad = sizeof(Type) * 2) :
         number(number), pad(pad)
      {
      }

      int64_t number;
      unsigned pad;
   };

   struct hexstr
   {
      template<typename Type, unsigned N>
      hexstr(Type (&buffer)[N]) :
         buffer(reinterpret_cast<const char*>(buffer)), size(N)
      {
      }

      hexstr(const char *buffer, unsigned size) :
         buffer(buffer), size(size)
      {
      }

      const char *buffer;
      unsigned size;
   };

   struct bin
   {
      bin(uint32_t number, unsigned pad = 0) :
         number(number), pad(pad)
      {
      }

      uint32_t number;
      unsigned pad;
   };

   class Output
   {
   public:
      Output(const std::string &pre, bool newLine = true)
      {
         if (newLine) {
            std::cout << std::endl;
         }

         if (pre.length()) {
            std::cout << pre << " ";
         }
      }

      template<typename T>
      Output& operator << (const T &val)
      {
         std::cout << val;
         return *this;
      }

      template<>
      Output& operator << (const Log::hexstr &val)
      {
         std::cout << std::hex << std::setfill('0');

         for (auto i = 0u; i < val.size; ++i) {
            std::cout << std::setw(2) << (unsigned)val.buffer[i];
         }

         std::cout << std::dec;
         return *this;
      }

      template<>
      Output& operator << (const Log::hex &val)
      {
         if (val.pad) {
            std::cout << std::hex << std::setfill('0') << std::setw(val.pad) << val.number;
         } else {
            std::cout << std::hex << val.number;
         }

         std::cout << std::dec;
         return *this;
      }

      template<>
      Output& operator << (const Log::bin &val)
      {
         for (unsigned i = 0; i < val.pad; ++i) {
            std::cout << ((val.number >> i) & 1);
         }

         return *this;
      }

   private:
   };

public:
   static Output custom(const std::string &pre, bool newLine = true)
   {
      return Output(pre, newLine);
   }

   static Output debug(bool newLine = true)
   {
      return Output("[DEBUG]", newLine);
   }

   static Output warning(bool newLine = true)
   {
      return Output("[WARNING]", newLine);
   }

   static Output error(bool newLine = true)
   {
      return Output("[ERROR]", newLine);
   }

private:
};

extern Log g_log;

#define xLog() Log::custom("", true)
#define xDebug() Log::debug()
#define xWarning() Log::warning()
#define xError() Log::error()
