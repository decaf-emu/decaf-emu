#pragma once

// Macro to indicate that a branch is likely or unlikely to be taken.
#ifdef __GNUC__  // Includes Clang.
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#endif

// Macros to force a function to always or never be inlined.
#ifdef __GNUC__
#define ALWAYS_INLINE inline __attribute__((always_inline))
#define NEVER_INLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#define ALWAYS_INLINE __forceinline
#define NEVER_INLINE __declspec(noinline)
#else
#define ALWAYS_INLINE //nothing
#define NEVER_INLINE //nothing
#endif

// Macro to disable optimization when building with Clang on a function which
//  both performs floating-point operations and checks exception flags that
//  could be affected by those operations.  This is required because LLVM is
//  unaware of the fact that floating-point operations can raise exceptions
//  (see http://llvm.org/bugs/show_bug.cgi?id=6050).  Place this immediately
//  before the opening brace for the function.
#ifdef __clang__
#define CLANG_FPU_BUG_WORKAROUND __attribute__((optnone))
#else
#define CLANG_FPU_BUG_WORKAROUND //nothing
#endif
