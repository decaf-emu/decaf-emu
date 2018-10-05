#include "coreinit.h"
#include "coreinit_snprintf.h"
#include "cafe/cafe_ppc_interface_varargs.h"

#include <common/log.h>
#include <common/make_array.h>
#include <common/strutils.h>
#include <fmt/format.h>
#include <fmt/printf.h>

namespace cafe::coreinit
{

static int32_t
os_snprintf(virt_ptr<char> buffer,
            uint32_t size,
            virt_ptr<const char> fmt,
            var_args va_args)
{
   auto list = make_va_list(va_args);
   auto result = internal::formatStringV(buffer, size, fmt, list);
   free_va_list(list);
   return result;
}

namespace internal
{

static const char c_flags[] = {
   '-', '+', ' ', '#', '0'
};

static const char c_width[] = {
   '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '*'
};

static const char c_precision[] = {
   '.', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '*'
};

static const char c_length[] = {
   'h', 'l', 'j', 'z', 't', 'L'
};

static const char c_specifier[] = {
   'd', 'i', 'u', 'o', 'x', 'X', 'f', 'F', 'e',
   'E', 'g', 'G', 'a', 'A', 'c', 's', 'p', 'n'
};

bool
formatStringV(virt_ptr<const char> fmt,
              virt_ptr<va_list> list,
              fmt::memory_buffer &output)
{
   std::string flags, width, length, precision, formatter;
   auto args = list->begin();
   auto fmtLen = strlen(fmt.getRawPointer());

   for (auto i = 0; fmt[i]; ) {
      if (fmt[i] != '%') {
         output.push_back(fmt[i++]);
         continue;
      }

      ++i;

      if (fmt[i] == '%') {
         // %% becomes %
         output.push_back('%');
         ++i;
         continue;
      }

      char specifier = 0;
      flags.clear();
      width.clear();
      length.clear();
      precision.clear();
      formatter.clear();

      while (std::find(std::begin(c_flags), std::end(c_flags), fmt[i]) != std::end(c_flags)) {
         flags.push_back(fmt[i]);
         ++i;
      }

      while (std::find(std::begin(c_width), std::end(c_width), fmt[i]) != std::end(c_width)) {
         width.push_back(fmt[i]);
         ++i;
      }

      if (fmt[i] == '.') {
         while (std::find(std::begin(c_precision), std::end(c_precision), fmt[i]) != std::end(c_precision)) {
            precision.push_back(fmt[i]);
            ++i;
         }
      }

      while (std::find(std::begin(c_length), std::end(c_length), fmt[i]) != std::end(c_length)) {
         length.push_back(fmt[i]);
         ++i;
      }

      if (std::find(std::begin(c_specifier), std::end(c_specifier), fmt[i]) != std::end(c_specifier)) {
         specifier = fmt[i];
         ++i;
      }

      switch (specifier) {
      case 'd':
      case 'i':
      case 'u':
      case 'o':
      case 'x':
      case 'X':
      case 'c':
         formatter = "%" + flags + width + precision + length + specifier;
         if (length.compare("ll") == 0 && specifier != 'c') {
            fmt::format_to(output, "{}", fmt::sprintf(formatter, args.next<uint64_t>()));
         } else {
            fmt::format_to(output, "{}", fmt::sprintf(formatter, args.next<uint32_t>()));
         }
         break;
      case 'g':
      case 'G':
      case 'f':
      case 'F':
      case 'e':
      case 'E':
      case 'a':
      case 'A':
         formatter = "%" + flags + width + precision + length + specifier;
         if (length.compare("L") == 0) {
            fmt::format_to(output, "{}", fmt::sprintf(formatter, static_cast<long double>(args.next<double>())));
         } else {
            fmt::format_to(output, "{}", fmt::sprintf(formatter, args.next<double>()));
         }
         break;
      case 'p':
         // We actually ignore formatter and just use %08X for %p
         formatter = "%" + flags + width + precision + length + specifier;
         fmt::format_to(output, "{:08X}", static_cast<uint32_t>(virt_cast<virt_addr>(args.next<virt_ptr<void>>())));
         break;
      case 's': {
         auto s = args.next<virt_ptr<const char>>();
         if (s) {
            fmt::format_to(output, "{}", s.getRawPointer());
         } else {
            fmt::format_to(output, "<NULL>");
         }
      } break;
      case 'n':
         if (length.compare("hh") == 0) {
            *(args.next<virt_ptr<int8_t>>()) = static_cast<int8_t>(output.size());
         } else if (length.compare("h") == 0) {
            *(args.next<virt_ptr<int16_t>>()) = static_cast<int16_t>(output.size());
         } else if (length.compare("ll") == 0) {
            *(args.next<virt_ptr<int64_t>>()) = static_cast<int64_t>(output.size());
         } else {
            *(args.next<virt_ptr<int32_t>>()) = static_cast<int32_t>(output.size());
         }
         break;
      default:
         gLog->error("Unimplemented format specifier: {}", specifier);
         return false;
      }
   }

   return true;
}

int32_t
formatStringV(virt_ptr<char> buffer,
              uint32_t len,
              virt_ptr<const char> fmt,
              virt_ptr<va_list> list)
{
   auto str = fmt::memory_buffer { };
   if (!formatStringV(fmt, list, str)) {
      return -1;
   }

   if (str.size() >= len - 1) {
      // Copy as much as possible
      std::memcpy(buffer.getRawPointer(), str.data(), len - 2);
      buffer[len - 1] = char { 0 };
      len -= 1;
   } else {
      // Copy whole string into buffer
      std::memcpy(buffer.getRawPointer(), str.data(), str.size());
      buffer[str.size()] = char { 0 };
      len = static_cast<uint32_t>(str.size());
   }

   return static_cast<int32_t>(len);
}

} // namespace internal

void
Library::registerSnprintfSymbols()
{
   RegisterFunctionExportName("__os_snprintf", os_snprintf);
}

} // namespace cafe::coreinit
