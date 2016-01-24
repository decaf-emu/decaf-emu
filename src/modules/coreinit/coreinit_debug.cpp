#include "coreinit.h"
#include "coreinit_debug.h"
#include "coreinit_thread.h"
#include "cpu/trace.h"
#include "processor.h"
#include "utils/log.h"
#include "utils/make_array.h"
#include "utils/strutils.h"

BOOL
OSIsDebuggerPresent()
{
   return FALSE;
}

BOOL
OSIsDebuggerInitialized()
{
   return FALSE;
}

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

static void
formatString(const char *fmt, ppctypes::VarList &args, std::string &output)
{
   output.reserve(strlen(fmt));

   for (auto i = 0; i < strlen(fmt); ) {
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
         case 'g':
         case 'G':
         case 'f':
         case 'F':
         case 'e':
         case 'E':
         case 'a':
         case 'A':
         case 'c':
         case 'p':
         case 'n':
            formatter = "%" + flags + width + precision + length + specifier;
            output.append(format_string(formatter.c_str(), args.next<uint32_t>()));
            break;
         case 's':
            output.append(args.next<const char*>());
            break;
         default:
            gLog->error("Unimplemented format specifier: {}", specifier);
            break;
         }
      } else {
         output.push_back(fmt[i]);
         ++i;
      }
   }
}

static void
OSPanic(const char *file, int line, const char *fmt, ppctypes::VarList& args)
{
   std::string str;
   formatString(fmt, args, str);
   gLog->error("OSPanic {}:{} {}", file, line, str);
   tracePrint(&OSGetCurrentThread()->fiber->state, 0, 0);
}

static void
OSReport(const char *fmt, ppctypes::VarList& args)
{
   std::string str;
   formatString(fmt, args, str);
   gLog->debug("OSReport {}", str);
}

static void
OSVReport(const char *fmt, ppctypes::VarList& args)
{
   std::string str;
   formatString(fmt, args, str);
   gLog->debug("OSVReport {}", str);
}

static void
COSWarn(uint32_t module, const char *fmt, ppctypes::VarList& args)
{
   std::string str;
   formatString(fmt, args, str);
   gLog->debug("COSWarn {} {}", module, str);
}

static void
OSConsoleWrite(const char *msg, uint32_t size)
{
   auto str = std::string { msg, size };
   gLog->debug("OSConsoleWrite {}", str);
}

static int
coreinit__os_snprintf(char *buffer, uint32_t size, const char *fmt, ppctypes::VarList& args)
{
   std::string str;
   formatString(fmt, args, str);
   return snprintf(buffer, size, "%s", str.c_str());
}

void
CoreInit::registerDebugFunctions()
{
   RegisterKernelFunction(OSIsDebuggerPresent);
   RegisterKernelFunction(OSIsDebuggerInitialized);
   RegisterKernelFunction(OSPanic);
   RegisterKernelFunction(OSReport);
   RegisterKernelFunction(OSVReport);
   RegisterKernelFunction(COSWarn);
   RegisterKernelFunction(OSConsoleWrite);
   RegisterKernelFunctionName("__os_snprintf", coreinit__os_snprintf);
}
