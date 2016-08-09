#include "coreinit.h"
#include "coreinit_sprintf.h"
#include "common/log.h"
#include "common/make_array.h"
#include "common/strutils.h"
#include "ppcutils/va_list.h"

namespace coreinit
{

static int
ghs__os_snprintf(char *buffer,
                 uint32_t size,
                 const char *fmt,
                 ppctypes::VarArgs)
{
   auto list = ppctypes::make_va_list(3, 0);
   auto result = internal::formatStringV(buffer, size, fmt, list);
   ppctypes::free_va_list(list);
   return result;
}

void
Module::registerSprintfFunctions()
{
   RegisterKernelFunctionName("__os_snprintf", ghs__os_snprintf);
}

namespace internal
{

static const auto c_flags = make_array<char>(
   '-', '+', ' ', '#', '0'
   );

static const auto c_width = make_array<char>(
   '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '*'
   );

static const auto c_precision = make_array<char>(
   '.', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '*'
   );

static const auto c_length = make_array<char>(
   'h', 'l', 'j', 'z', 't', 'L'
   );

static const auto c_specifier = make_array<char>(
   'd', 'i', 'u', 'o', 'x', 'X', 'f', 'F', 'e',
   'E', 'g', 'G', 'a', 'A', 'c', 's', 'p', 'n'
   );

bool
formatStringV(const char *fmt,
              ppctypes::va_list *list,
              std::string &output)
{
   auto result = true;
   auto args = list->begin();
   output.reserve(strlen(fmt));

   for (auto i = 0; i < strlen(fmt) && result; ) {
      if (fmt[i] == '%') {
         i++;

         if (fmt[i] == '%') {
            output.push_back('%');
            ++i;
            continue;
         }

         std::string flags, width, length, precision, formatter;
         char specifier = 0;

         while (std::find(c_flags.begin(), c_flags.end(), fmt[i]) != c_flags.end()) {
            flags.push_back(fmt[i]);
            ++i;
         }

         while (std::find(c_width.begin(), c_width.end(), fmt[i]) != c_width.end()) {
            width.push_back(fmt[i]);
            ++i;
         }

         if (fmt[i] == '.') {
            while (std::find(c_precision.begin(), c_precision.end(), fmt[i]) != c_precision.end()) {
               precision.push_back(fmt[i]);
               ++i;
            }
         }

         while (std::find(c_length.begin(), c_length.end(), fmt[i]) != c_length.end()) {
            length.push_back(fmt[i]);
            ++i;
         }

         if (std::find(c_specifier.begin(), c_specifier.end(), fmt[i]) != c_specifier.end()) {
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
               output.append(format_string(formatter.c_str(), args.next<uint64_t>()));
            } else {
               output.append(format_string(formatter.c_str(), args.next<uint32_t>()));
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
               output.append(format_string(formatter.c_str(), static_cast<long double>(args.next<double>())));
            } else {
               output.append(format_string(formatter.c_str(), args.next<double>()));
            }
            break;
         case 'p':
            formatter = "%" + flags + width + precision + length + specifier;
            output.append(format_string(formatter.c_str(), bit_cast<void *, uintptr_t>(mem::untranslate(args.next<void *>()))));
            break;
         case 's':
            output.append(args.next<const char *>());
            break;
         case 'n':
            if (length.compare("hh") == 0) {
               *(args.next<int8_t *>()) = static_cast<int8_t>(output.size());
            } else if (length.compare("h") == 0) {
               *(args.next<be_val<int16_t> *>()) = static_cast<int16_t>(output.size());
            } else if (length.compare("ll") == 0) {
               *(args.next<be_val<int64_t> *>()) = static_cast<int64_t>(output.size());
            } else {
               *(args.next<be_val<int32_t> *>()) = static_cast<int32_t>(output.size());
            }
            break;
         default:
            gLog->error("Unimplemented format specifier: {}", specifier);
            result = false;
            break;
         }
      } else {
         output.push_back(fmt[i]);
         ++i;
      }
   }

   return result;
}

int
formatStringV(char *buffer,
              uint32_t len,
              const char *fmt,
              ppctypes::va_list *list)
{
   std::string str;
   str.reserve(len);

   if (!formatStringV(fmt, list, str)) {
      return -1;
   }

   if (str.length() > len - 1) {
      // Copy as much as possible
      std::memcpy(buffer, str.data(), len - 1);
      buffer[len] = 0;
   } else {
      // Copy whole string into buffer
      std::memcpy(buffer, str.data(), str.length());
      buffer[str.length()] = 0;
   }

   return str.length();
}

} // namespace internal

} // namespace coreinit
