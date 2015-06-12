#pragma once
#include "systemtypes.h"
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

// Convert arguments from registers
template<typename Type>
struct sysfunc_arg;

template<typename PtrType>
struct sysfunc_arg<p32<PtrType>>
{
   static inline p32<PtrType> convert(ThreadState *state, size_t &r, size_t &f)
   {
      p32<PtrType> ptr;
      ptr.address = state->gpr[r++];
      return ptr;
   }
};

template<typename Type>
struct sysfunc_arg<Type *>
{
   static inline Type *convert(ThreadState *state, size_t &r, size_t &f)
   {
      return reinterpret_cast<Type*>(gMemory.translate(state->gpr[r++]));
   }
};

template<>
struct sysfunc_arg<float>
{
   static inline float convert(ThreadState *state, size_t &r, size_t &f)
   {
      return state->fpr[f++].paired0;
   }
};

template<>
struct sysfunc_arg<double>
{
   static inline double convert(ThreadState *state, size_t &r, size_t &f)
   {
      return state->fpr[f++].value;
   }
};

template<typename Type>
struct sysfunc_arg
{
   static inline Type convert(ThreadState *state, size_t &r, size_t &f)
   {
      return static_cast<Type>(state->gpr[r++]);
   }
};

// Helper cos msvc2013 can't handle me.
template<typename Type>
static inline Type
convert_sysfunc_arg(ThreadState *state, size_t &r, size_t &f)
{
   return sysfunc_arg<Type>::convert(state, r, f);
}

// Convert result to gpr
template<typename Type>
struct sysfunc_result;

template<typename PtrType>
struct sysfunc_result<p32<PtrType>>
{
   static inline void update(ThreadState *state, p32<PtrType> v)
   {
      state->gpr[3] = static_cast<uint32_t>(v);
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

// Just to hold a list of types, nothing fancy...
template<typename...>
struct type_list
{
};

template<typename Out, typename Type>
static inline void
logSyscallArgument(Out &out, p32<Type> &value)
{
   out << Log::hex(static_cast<uint32_t>(value));
}

template<typename Out, typename Type>
static inline void
logSyscallArgument(Out &out, Type *value)
{
   out << Log::hex(static_cast<uint32_t>(make_p32(value)));
}

template<typename Out, typename Type>
static inline
typename std::enable_if<std::is_enum<Type>::value, void>::type
logSyscallArgument(Out &out, Type &value)
{
   out << static_cast<int>(value);
}

template<typename Out, typename Type>
static inline
typename std::enable_if<!std::is_enum<Type>::value, void>::type
logSyscallArgument(Out &out, Type &value)
{
   out << value;
}

// Function with non-void return type
template<typename Ret, typename... Args>
struct SystemFunctionImpl : SystemFunction
{
   Ret(*wrapped_function)(Args...);

   template<class Out, class Head, class... Tail, class... Args>
   void dispatch(ThreadState *state, size_t &r, size_t &f, Out &out, type_list<Head, Tail...>, Args... values)
   {
      if (r != 3 || f != 1) {
         out << ", ";
      }

      auto value = convert_sysfunc_arg<Head>(state, r, f);
      logSyscallArgument(out, value);
      dispatch(state, r, f, out, type_list<Tail...>{}, values..., value);
   }

   template<class Out, class... Args>
   void dispatch(ThreadState *state, size_t &r, size_t &f, Out &out, type_list<>, Args... args)
   {
      auto result = wrapped_function(args...);
      sysfunc_result<Ret>::update(state, result);
   }

   virtual void call(ThreadState *state) override
   {
      size_t r = 3;
      size_t f = 1;
      auto out = Log::custom("SYS");
      out << this->name << "(";
      dispatch(state, r, f, out, type_list<Args...> {});
      out << ")";
   }
};

// Function with void return type
template<typename... FuncArgs>
struct SystemFunctionImpl<void, FuncArgs...> : SystemFunction
{
   void(*wrapped_function)(FuncArgs...);

   template<class Out, class Head, class... Tail, class... Args>
   void dispatch(ThreadState *state, size_t &r, size_t &f, Out &out, type_list<Head, Tail...>, Args... values)
   {
      if (r != 3 || f != 1) {
         out << ", ";
      }

      auto value = convert_sysfunc_arg<Head>(state, r, f);
      logSyscallArgument(out, value);
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
