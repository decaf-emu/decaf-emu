#pragma once
#include "cafe_hle_library_symbol.h"
#include "cafe/cafe_ppc_interface_invoke.h"
#include "cafe/cafe_ppc_interface_invoke_trace.h"

#include <libcpu/cpu.h>

namespace cafe::hle
{

extern volatile bool FunctionTraceEnabled;

struct UnimplementedLibraryFunction
{
   class Library *library = nullptr;
   std::string name;
   uint32_t syscallID = 0xFFFFFFFFu;
   virt_addr value;
};

struct LibraryFunction : public LibrarySymbol
{
   LibraryFunction(cpu::KernelCallHandler _invokeHandler, bool& _traceEnabledRef) :
      LibrarySymbol(LibrarySymbol::Function), invokeHandler(_invokeHandler), traceEnabled(_traceEnabledRef)
   {
   }

   virtual ~LibraryFunction()
   {
   }

   //! The actual handler for this function
   cpu::KernelCallHandler invokeHandler;

   //! Reference to the underlying invoke handler trace wrapper's trace enabled
   // value, specifying whether trace logging is enabled for this function or not.
   bool& traceEnabled;

   //! ID number of syscall.
   uint32_t syscallID = 0xFFFFFFFFu;

   //! Pointer to host function pointer, only set for internal functions.
   virt_ptr<void> *hostPtr = nullptr;
};

namespace internal
{

template<typename FunctionType, FunctionType Func>
struct StackReserveWrapper
{
   static inline cpu::Core* wrapped(cpu::Core *core, uint32_t kcId)
   {
      // Save our original stack pointer for the backchain
      auto backchainSp = core->gpr[1];

      // Allocate callee backchain and lr space.
      auto newSp = backchainSp - 2 * 4;
      core->gpr[1] = newSp;

      // Write the backchain pointer
      *virt_cast<uint32_t *>(virt_addr { newSp }) = backchainSp;

      // Handle the HLE function call
      core = invoke<FunctionType, Func>(core);

      // Release callee backchain and lr space.
      core->gpr[1] = backchainSp;

      return core;
   }
};

template<typename FunctionType, FunctionType Func>
struct TracingWrapper
{
   static inline cpu::Core* wrapped(cpu::Core *core, uint32_t kcId)
   {
      // Perform any tracing that is needed
      if (FunctionTraceEnabled && traceEnabled) {
         invoke_trace<FunctionType>(core, traceName.c_str());
      }

      return StackReserveWrapper<FunctionType, Func>::wrapped(core, kcId);
   }

   static inline std::string traceName = "_missingName";
   static inline bool traceEnabled = false;
};

template<typename FunctionType, FunctionType Func>
inline std::unique_ptr<LibraryFunction>
makeLibraryFunction(const std::string &name)
{
   TracingWrapper<FunctionType, Func>::traceName = name;

   auto libraryFunction = new LibraryFunction(
      TracingWrapper<FunctionType, Func>::wrapped,
      TracingWrapper<FunctionType, Func>::traceEnabled);
   return std::unique_ptr<LibraryFunction> { libraryFunction };
}

/**
 * Handles calls to constructors.
 */
template<typename ObjectType, typename... ArgTypes>
struct ConstructorWrapper
{
   static inline void wrapped(virt_ptr<ObjectType> obj, ArgTypes... args)
   {
      ::new(static_cast<void *>(obj.getRawPointer())) ObjectType(args...);
   }
};

template<typename ObjectType, typename... ArgTypes>
inline std::unique_ptr<LibraryFunction>
makeLibraryConstructorFunction(const std::string &name)
{
   return makeLibraryFunction<
      decltype(&ConstructorWrapper<ObjectType, ArgTypes...>::wrapped),
      ConstructorWrapper<ObjectType, ArgTypes...>::wrapped>(name);
}

/**
 * Handles calls to destructors.
 */
template<typename ObjectType>
struct DestructorWrapper
{
   static inline void wrapped(virt_ptr<ObjectType> obj)
   {
      (obj.getRawPointer())->~ObjectType();
   }
};

template<typename ObjectType>
inline std::unique_ptr<LibraryFunction>
makeLibraryDestructorFunction(const std::string &name)
{
   return makeLibraryFunction<
      decltype(&DestructorWrapper<ObjectType>::wrapped),
      DestructorWrapper<ObjectType>::wrapped>(name);
}

} // namespace internal

} // cafe::hle
