#include "coreinit.h"
#include "coreinit_debug.h"
#include "log.h"

BOOL
OSIsDebuggerPresent(void)
{
   return TRUE;
}

BOOL
OSIsDebuggerInitialized(void)
{
   return FALSE;
}

static std::vector<char> c_flags = {
   '-', '+', ' ', '#', '0'
};

static std::vector<char> c_width = {
   '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '*'
};

static std::vector<char> c_precision = {
   '.', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '*'
};

static std::vector<char> c_length = {
   'h', 'l', 'j', 'z', 't', 'L'
};

static std::vector<char> c_specifier = {
   'd', 'i', 'u', 'o', 'x', 'X', 'f', 'F', 'e',
   'E', 'g', 'G', 'a', 'A', 'c', 's', 'p', 'n'
};

static void
formatString(ThreadState *state, std::string &output, unsigned reg = 3)
{
   char buffer[32];
   const char *fmt = reinterpret_cast<const char*>(gMemory.translate(state->gpr[reg++]));
   output.reserve(strlen(fmt));

   for (auto i = 0; i < strlen(fmt); ) {
      if (fmt[i] == '%') {
         i++;

         if (fmt[i] == '%') {
            output.push_back('%');
            ++i;
            continue;
         }

         std::string flags, width, length, precision;
         char specifier;

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

         std::string formatter = "%";
         formatter += flags + width + precision + length + specifier;
         buffer[0] = 0;

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
            sprintf_s(buffer, 32, formatter.c_str(), state->gpr[reg]);
            output.append(buffer);
            break;
         case 's':
            output.append(reinterpret_cast<char*>(gMemory.translate(state->gpr[reg])));
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
OSPanic(ThreadState *state)
{
   char *file = reinterpret_cast<char*>(gMemory.translate(state->gpr[3]));
   int line = state->gpr[4];
   std::string str;
   formatString(state, str, 5);
   gLog->error("OSPanic {}:{} {}", file, line, str);
}

static void
OSReport(ThreadState *state)
{
   std::string str;
   formatString(state, str);
   gLog->debug("OSReport {}", str);
}

static void
OSVReport(ThreadState *state)
{
   std::string str;
   formatString(state, str);
   gLog->debug("OSVReport {}", str);
}

void
CoreInit::registerDebugFunctions()
{
   RegisterKernelFunction(OSIsDebuggerPresent);
   RegisterKernelFunction(OSIsDebuggerInitialized);
   RegisterKernelFunctionManual(OSPanic);
   RegisterKernelFunctionManual(OSReport);
   RegisterKernelFunctionManual(OSVReport);
}
