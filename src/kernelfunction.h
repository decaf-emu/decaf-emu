#pragma once
#include "kernelexport.h"
#include "kernelfunctionargs.h"
#include "kernelfunctionresult.h"
#include "kernelfunctionlog.h"
#include "log.h"
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

struct DispatchState
{
   ThreadState *thread;
   LogState log;
   size_t r;
   size_t f;
};

// Function with non-void return type
template<typename Ret, typename... Args>
struct KernelFunctionImpl : KernelFunction
{
   Ret(*wrapped_function)(Args...);

   template<class Head, class... Tail, class... Args>
   void dispatch(DispatchState &state, type_list<Head, Tail...>, Args... values)
   {
      auto value = convertArgument<Head>(state.thread, state.r, state.f);
      logArgument(state.log, value);
      dispatch(state, type_list<Tail...>{}, values..., value);
   }

   template<class... Args>
   void dispatch(DispatchState &state, type_list<>, Args... args)
   {
      gLog->trace(logCallEnd(state.log));
      auto result = wrapped_function(args...);
      setResult<Ret>(state.thread, result);
   }

   virtual void call(ThreadState *thread) override
   {
      DispatchState state;
      state.thread = thread;
      state.r = 3;
      state.f = 1;
      logCall(state.log, this->name);
      dispatch(state, type_list<Args...> {});
   }
};

// Function with void return type
template<typename... FuncArgs>
struct KernelFunctionImpl<void, FuncArgs...> : KernelFunction
{
   void(*wrapped_function)(FuncArgs...);

   template<class Head, class... Tail, class... Args>
   void dispatch(DispatchState &state, type_list<Head, Tail...>, Args... values)
   {
      auto value = convertArgument<Head>(state.thread, state.r, state.f);
      logArgument(state.log, value);
      dispatch(state, type_list<Tail...>{}, values..., value);
   }

   template<class... Args>
   void dispatch(DispatchState &state, type_list<>, Args... args)
   {
      gLog->trace(logCallEnd(state.log));
      wrapped_function(args...);
   }

   virtual void call(ThreadState *thread) override
   {
      DispatchState state;
      state.thread = thread;
      state.r = 3;
      state.f = 1;
      logCall(state.log, this->name);
      dispatch(state, type_list<FuncArgs...> {});
   }
};

// Manual system function, can be used for var arg funcs
struct KernelFunctionManual : KernelFunction
{
   void(*wrapped_function)(ThreadState *state);

   virtual void call(ThreadState *thread) override
   {
      LogState log;
      logCall(log, this->name);
      gLog->trace(logCallEnd(log));
      wrapped_function(thread);
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
