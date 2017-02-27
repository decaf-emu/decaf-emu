#pragma once
#include <cstdio>
#include <common/strutils.h>
#include <string>
#include <spdlog/fmt/fmt.h>

namespace ppctypes
{

struct LogState
{
   fmt::MemoryWriter out;
   int argc = 0;
};

inline void
logCall(LogState &state, uint32_t lr, const std::string &name)
{
   state.out.write("0x{:08X} {}(", lr, name.c_str());
}

inline std::string
logCallEnd(LogState &state)
{
   state.out.write(")");
   return state.out.str();
}

inline void
logArgumentSeparator(LogState &state)
{
   if (state.argc > 0) {
      state.out.write(", ");
   }

   state.argc++;
}

// ...
inline void
logArgumentVargs(LogState &state)
{
   logArgumentSeparator(state);
   state.out.write("...");
}

// const char *
inline void
logArgument(LogState &state, const char *value)
{
   logArgumentSeparator(state);

   if (!value) {
      state.out.write("NULL");
   } else {
      state.out.write("\"");

      for (const char *c = value; *c; ++c) {
         if (*c == '\n') {
            state.out.write("\\n");
         } else if (*c == '\r') {
            state.out.write("\\r");
         } else if (*c == '\t') {
            state.out.write("\\t");
         } else {
            state.out << *c;
         }
      }

      state.out.write("\"");
   }
}

// Type *
template<typename Type>
inline void
logArgument(LogState &state, Type *value)
{
   logArgument(state, mem::untranslate(value));
}

// Log generic (not enum)
template<typename Type>
inline
typename std::enable_if<!std::is_enum<Type>::value, void>::type
logArgument(LogState &state, Type value)
{
   logArgumentSeparator(state);

   if (std::is_floating_point<Type>::value) {
      state.out << value;
   } else if (std::is_unsigned<Type>::value) {
      if ((uint32_t)value < 0x10) {
         state.out.write("0x{:X}", value);
      } else if (sizeof(Type) == 1) {
         state.out.write("0x{:02X}", value);
      } else if (sizeof(Type) == 2) {
         state.out.write("0x{:04X}", value);
      } else if (sizeof(Type) == 4) {
         state.out.write("0x{:08X}", value);
      } else if (sizeof(Type) == 8) {
         state.out.write("0x{:016X}", value);
      } else {
         state.out.write("0x{:X}", value);
      }
   } else {
      state.out << value;
   }
}

// Log enum (cast to int so stream out succeeds)
template<typename Type>
inline
typename std::enable_if<std::is_enum<Type>::value, void>::type
logArgument(LogState &state, Type value)
{
   logArgument(state, static_cast<int>(value));
}

template <typename Type>
inline std::string
logCallResult(Type &value)
{
   LogState state;
   state.out << " = ";
   logArgument(state, value);
   return state.out.str();
}

inline std::string
logCallResult()
{
   return " = void";
}

} // namespace ppctypes
