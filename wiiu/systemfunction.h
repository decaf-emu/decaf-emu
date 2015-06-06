#pragma once
#include "systemtypes.h"
#include "integer_sequence.h"
#include "ppc.h"
#include "systemexport.h"

// System Function Export
struct SystemFunction : SystemExport
{
   SystemFunction() :
      SystemExport(SystemExport::Function)
   {
   }

   virtual ~SystemFunction()
   {
   }

   uint32_t syscallID;
   uint32_t vaddr;
   virtual void call(ThreadState *state) = 0;
};

// Convert arguments from gpr
template<typename Type>
struct sysfunc_arg;

template<typename PtrType>
struct sysfunc_arg<p32<PtrType>>
{
   static inline p32<PtrType> convert(uint32_t v)
   {
      p32<PtrType> ptr;
      ptr.value = v;
      return ptr;
   }
};

template<typename Type>
struct sysfunc_arg<Type *>
{
   static inline Type *convert(uint32_t v)
   {
      return reinterpret_cast<Type*>(gMemory.translate(v));
   }
};

template<typename Type>
struct sysfunc_arg
{
   static inline Type convert(uint32_t v)
   {
      return static_cast<Type>(v);
   }
};

// Convert result to gpr
template<typename Type>
struct sysfunc_result;

template<typename PtrType>
struct sysfunc_result<p32<PtrType>>
{
   static inline void update(ThreadState *state, p32<PtrType> v)
   {
      state->gpr[3] = v.value;
   }
};

template<typename Type>
struct sysfunc_result
{
   static inline void update(ThreadState *state, Type v)
   {
      state->gpr[3] = static_cast<uint32_t>(v);
   }
};

// Function with non-void return type
template<typename Ret, typename... Args>
struct SystemFunctionImpl : SystemFunction
{
   Ret(*wrapped_function)(Args...);

   template<unsigned... I>
   void dispatch(ThreadState *state, index_sequence<I...>)
   {
      auto result = wrapped_function(sysfunc_arg<Args>::convert(state->gpr[3 + I])...);
      sysfunc_result<Ret>::update(state, result);
   }

   virtual void call(ThreadState *state) override
   {
      dispatch(state, index_sequence_for<Args...>());
   }
};

// Function with void return type
template<typename... Args>
struct SystemFunctionImpl<void, Args...> : SystemFunction
{
   void(*wrapped_function)(Args...);

   template<unsigned... I>
   void dispatch(ThreadState *state, index_sequence<I...>)
   {
      wrapped_function(sysfunc_arg<Args>::convert(state->gpr[3 + I])...);
   }

   virtual void call(ThreadState *state) override
   {
      dispatch(state, index_sequence_for<Args...>());
   }
};

// Manual system function, can be used for var arg funcs
struct SystemFunctionManual : SystemFunction
{
   void(*wrapped_function)(ThreadState *state);

   virtual void call(ThreadState *state) override
   {
      wrapped_function(state);
   }
};

// Create a SystemFunction export from a function pointer
template<typename Ret, typename... Args>
inline SystemFunction *
make_sysfunc(Ret(*fptr)(Args...))
{
   auto func = new SystemFunctionImpl<Ret, Args...>();
   func->wrapped_function = fptr;
   return func;
}

// Create a SystemFunction export from a function pointer
inline SystemFunction *
make_manual_sysfunc(void(*fptr)(ThreadState*))
{
   auto func = new SystemFunctionManual();
   func->wrapped_function = fptr;
   return func;
}
