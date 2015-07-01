#pragma once
#include "kernelexport.h"
#include "kernelfunctionargs.h"
#include "kernelfunctionresult.h"
#include "systemtypes.h"
#include "ppc.h"
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

   template<class Out, class Head, class... Tail, class... Args>
   void dispatch(ThreadState *state, size_t &r, size_t &f, Out &out, type_list<Head, Tail...>, Args... values)
   {
      if (r != 3 || f != 1) {
         out << ", ";
      }

      auto value = convertArgument<Head>(state, r, f);
      logArgument(out, value);
      dispatch(state, r, f, out, type_list<Tail...>{}, values..., value);
   }

   template<class Out, class... Args>
   void dispatch(ThreadState *state, size_t &r, size_t &f, Out &out, type_list<>, Args... args)
   {
      out << ")";
      auto result = wrapped_function(args...);
      setResult<Ret>(state, result);
   }

   virtual void call(ThreadState *state) override
   {
      size_t r = 3;
      size_t f = 1;
      auto out = Log::custom("SYS");
      out << this->name << "(";
      dispatch(state, r, f, out, type_list<Args...> {});
   }
};

// Function with void return type
template<typename... FuncArgs>
struct KernelFunctionImpl<void, FuncArgs...> : KernelFunction
{
   void(*wrapped_function)(FuncArgs...);

   template<class Out, class Head, class... Tail, class... Args>
   void dispatch(ThreadState *state, size_t &r, size_t &f, Out &out, type_list<Head, Tail...>, Args... values)
   {
      if (r != 3 || f != 1) {
         out << ", ";
      }

      auto value = convertArgument<Head>(state, r, f);
      logArgument(out, value);
      dispatch(state, r, f, out, type_list<Tail...>{}, values..., value);
   }

   template<class Out, class... Args>
   void dispatch(ThreadState *state, size_t &r, size_t &f, Out &out, type_list<>, Args... args)
   {
      wrapped_function(args...);
   }

   virtual void call(ThreadState *state) override
   {
      size_t r = 3;
      size_t f = 1;
      auto out = Log::custom("SYS");
      out << this->name << "(";
      dispatch(state, r, f, out, type_list<FuncArgs...> {});
      out << ")";
   }
};

// Manual system function, can be used for var arg funcs
struct KernelFunctionManual : KernelFunction
{
   void(*wrapped_function)(ThreadState *state);

   virtual void call(ThreadState *state) override
   {
      wrapped_function(state);
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

// Create a SystemFunction export from a function pointer
inline KernelFunction *
makeManualFunction(void(*fptr)(ThreadState*))
{
   auto func = new kernel::functions::KernelFunctionManual();
   func->wrapped_function = fptr;
   return func;
}

};
