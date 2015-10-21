#pragma once
#include <cstdio>
#include "utils/virtual_ptr.h"

namespace ppctypes
{

struct LogState
{
   char buffer[255];
   int length = 255;
   int pos = 0;
   int argc = 0;
};

static inline void
logCall(LogState &state, uint32_t lr, const char *name)
{
   auto len = sprintf_s(state.buffer + state.pos, state.length, "0x%08X %s(", lr, name);
   state.length -= len;
   state.pos += len;
}

static inline const char *
logCallEnd(LogState &state)
{
   auto len = sprintf_s(state.buffer + state.pos, state.length, ")");
   state.length -= len;
   state.pos += len;
   return state.buffer;
}

static inline void
logArgumentSeparator(LogState &state)
{
   if (state.argc > 0) {
      auto len = sprintf_s(state.buffer + state.pos, state.length, ", ");
      state.length -= len;
      state.pos += len;
   }

   state.argc++;
}

// ...
static inline void
logArgumentVargs(LogState &state)
{
   int len;
   logArgumentSeparator(state);

   len = sprintf_s(state.buffer + state.pos, state.length, "...");
   state.length -= len;
   state.pos += len;
}

// const char *
static inline void
logArgument(LogState &state, const char *value)
{
   int len;
   logArgumentSeparator(state);

   if (!value) {
      len = sprintf_s(state.buffer + state.pos, state.length, "%s", "NULL");
   } else {
      auto newPos = state.pos;

      state.buffer[newPos++] = '\"';
      for (const char *c = value; *c; ++c) {
         if (*c == '\n' || *c == '\r' || *c == '\t') {
            state.buffer[newPos++] = '\\';
            if (*c == '\n') {
               state.buffer[newPos++] = 'n';
            } else if (*c == '\r') {
               state.buffer[newPos++] = 'r';
            } else if (*c == '\t') {
               state.buffer[newPos++] = 't';
            } else {
               state.buffer[newPos++] = '?';
            }
         } else {
            state.buffer[newPos++] = *c;
         }
      }
      state.buffer[newPos++] = '\"';

      len = newPos - state.pos;
   }

   state.length -= len;
   state.pos += len;
}

// char *
static inline void
logArgument(LogState &state, char *value)
{
   logArgument(state, const_cast<const char*>(value));
}

// virtual_ptr<Type>
template<typename Type>
static inline void
logArgument(LogState &state, virtual_ptr<Type> value)
{
   logArgument(state, value.getAddress());
}

// Type *
template<typename Type>
static inline void
logArgument(LogState &state, Type *value)
{
   logArgument(state, make_virtual_ptr<Type>(value));
}

// Log generic (not enum)
template<typename Type>
static inline
typename std::enable_if<!std::is_enum<Type>::value, void>::type
logArgument(LogState &state, Type value)
{
   int len;
   const char *fmt = "";
   logArgumentSeparator(state);

   if (std::is_floating_point<Type>::value) {
      fmt = "%f";
   } else if (std::is_unsigned<Type>::value) {
      if ((uint32_t)value < 0x1000) {
         fmt = "0x%X";
      } else if (sizeof(Type) == 1) {
         fmt = "0x%02X";
      } else if (sizeof(Type) == 2) {
         fmt = "0x%04X";
      } else if (sizeof(Type) == 4) {
         fmt = "0x%08X";
      } else if (sizeof(Type) == 8) {
         fmt = "0x%016llX";
      } else {
         fmt = "0x%X";
      }
   } else {
      fmt = "%d";
   }

   len = sprintf_s(state.buffer + state.pos, state.length, fmt, value);

   state.length -= len;
   state.pos += len;
}

// Log enum (cast to int so stream out succeeds)
template<typename Type>
static inline
typename std::enable_if<std::is_enum<Type>::value, void>::type
logArgument(LogState &state, Type value)
{
   logArgument(state, static_cast<int>(value));
}

} // namespace ppctypes
