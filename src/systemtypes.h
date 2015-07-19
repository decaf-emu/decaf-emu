#pragma once
#include <cstdint>
#include "be_val.h"
#include "memory.h"
#include "p32.h"
#include "wfunc_ptr.h"

#ifndef BOOL
typedef int BOOL;
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

// Ensure our structs are correct size & offsets to match WiiU
#define CHECK_SIZE(Type, Size) \
   static_assert(sizeof(Type) == Size, \
                 #Type " must be " #Size " bytes")

#define CHECK_OFFSET(Type, Offset, Field) \
   static_assert(offsetof(Type, Field) == Offset, \
                 #Type "::" #Field " must be at offset " #Offset)

// Workaround weird macro concat ## behaviour
#define PP_CAT(a, b) PP_CAT_I(a, b)
#define PP_CAT_I(a, b) PP_CAT_II(~, a ## b)
#define PP_CAT_II(p, res) res

// Allow us to easily add UNKNOWN / PADDING bytes into our structs,
// generates unique variable names using __COUNTER__
#define UNKNOWN(Size) char PP_CAT(__unk, __COUNTER__) [Size]
#define PADDING(Size) UNKNOWN(Size)

#define UNKNOWN_ARGS void
