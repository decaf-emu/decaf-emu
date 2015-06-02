#include "coreinit.h"
#include "coreinit_debug.h"
#include "log.h"

// TODO: Return TRUE so we get OSDebug messages!

BOOL
OSIsDebuggerPresent(void)
{
   return FALSE;
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
   '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '*'
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
         formatter += flags + width + length + precision + specifier;
         buffer[0] = 0;

         switch (specifier) {
         case 'd':
         case 'i':
            sprintf_s(buffer, 32, formatter.c_str(), state->gpr[reg]);
            output.append(buffer);
            break;
         case 's':
            output.append(reinterpret_cast<char*>(gMemory.translate(state->gpr[reg])));
            break;
         case 'u':
         case 'o':
         case 'x':
         case 'X':
         case 'f':
         case 'F':
         case 'e':
         case 'E':
         case 'g':
         case 'G':
         case 'a':
         case 'A':
         case 'c':
         case 'p':
         case 'n':
         default:
            xLog() << "Unimplemented format specifier: " << specifier;
            break;
         }

      } else {
         output.push_back(fmt[i]);
         ++i;
      }
   }

   xLog() << "Formatted " << fmt << " to " << output;
}

void
OSReport(ThreadState *state)
{
   std::string str;
   formatString(state, str);
   xLog() << str;
}

void
CoreInit::registerDebugFunctions()
{
   RegisterSystemFunction(OSIsDebuggerPresent);
   RegisterSystemFunction(OSIsDebuggerInitialized);
   RegisterSystemFunctionManual(OSReport);
}
