#pragma once
#include <cstdio>
#include "p32.h"

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

// const char *
static inline void
logArgument(LogState &state, const char *value)
{
   int len;
   logArgumentSeparator(state);

   if (!value) {
      len = sprintf_s(state.buffer + state.pos, state.length, "%s", "NULL");
   } else {
      len = sprintf_s(state.buffer + state.pos, state.length, "\"%s\"", value);
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

// p32<Type>
template<typename Type>
static inline void
logArgument(LogState &state, p32<Type> value)
{
   int len;
   logArgumentSeparator(state);

   len = sprintf_s(state.buffer + state.pos, state.length, "%08X", static_cast<uint32_t>(value));
   state.length -= len;
   state.pos += len;
}

// Type *
template<typename Type>
static inline void
logArgument(LogState &state, Type *value)
{
   logArgument(state, make_p32<Type>(value));
}

// Log generic (not enum)
template<typename Type>
static inline
typename std::enable_if<!std::is_enum<Type>::value, void>::type
logArgument(LogState &state, Type value)
{
   int len;
   logArgumentSeparator(state);

   len = sprintf_s(state.buffer + state.pos, state.length, "%d", static_cast<int>(value));
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

}
