#pragma once

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
