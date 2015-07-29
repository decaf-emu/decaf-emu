#pragma once
#include "systemtypes.h"
#include "kernelexport.h"
#include "ppc.h"
#include "ppcinvoke.h"
#include "util.h"

// Kernel Function Export
struct KernelFunction : KernelExport
{
   KernelFunction() :
      KernelExport(KernelExport::Function)
   {
   }

   virtual ~KernelFunction()
   {
   }

   uint32_t syscallID;
   uint32_t vaddr;
   virtual void call(ThreadState *state) = 0;
};

namespace kernel
{

namespace functions
{

// Function with non-void return type
template<typename Ret, typename... Args>
struct KernelFunctionImpl : KernelFunction
{
   Ret(*wrapped_function)(Args...);

   virtual void call(ThreadState *thread) override
   {
      ppctypes::invoke(thread, wrapped_function, this->name);
   }
};
   
};

// Create a SystemFunction export from a function pointer
template<typename Ret, typename... Args>
inline KernelFunction *
makeFunction(Ret(*fptr)(Args...))
{
   auto func = new kernel::functions::KernelFunctionImpl<Ret, Args...>();
   func->wrapped_function = fptr;
   return func;
}

};
