#pragma once
#include <cstdint>
#include "cpu/state.h"
#include "kernelexport.h"
#include "ppcinvoke.h"
#include "utils/type_list.h"

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

   virtual void call(ThreadState *state) = 0;

   bool valid = false;
   uint32_t syscallID = 0;
   uint32_t vaddr = 0;
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

} // namespace functions

// Create a SystemFunction export from a function pointer
template<typename Ret, typename... Args>
inline KernelFunction *
makeFunction(Ret(*fptr)(Args...))
{
   auto func = new kernel::functions::KernelFunctionImpl<Ret, Args...>();
   func->valid = true;
   func->wrapped_function = fptr;
   return func;
}

}  // namespace kernel
