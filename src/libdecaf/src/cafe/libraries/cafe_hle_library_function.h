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
   LibraryFunction(cpu::SystemCallHandler _invokeHandler,
                   bool& _traceEnabledRef) :
      LibrarySymbol(LibrarySymbol::Function),
      invokeHandler(_invokeHandler),
      traceEnabled(_traceEnabledRef)
   {
   }

   virtual ~LibraryFunction()
   {
   }

   //! The actual handler for this function
   cpu::SystemCallHandler invokeHandler;

   //! Reference to the underlying invoke handler trace wrapper's trace enabled
   // value, specifying whether trace logging is enabled for this function or not.
   bool &traceEnabled;

   //! ID number of syscall.
   uint32_t syscallID = 0xFFFFFFFFu;

   //! Pointer to host function pointer, only set for internal functions.
   virt_ptr<void> *hostPtr = nullptr;
};

namespace internal
{

template<typename FunctionType, FunctionType Func>
struct TracingWrapper
{
   static inline cpu::Core *wrapped(cpu::Core *core, uint32_t kcId)
   {
      if (FunctionTraceEnabled && traceEnabled) {
         invoke_trace<FunctionType>(core, traceName.c_str());
      }

      return invoke<FunctionType, Func>(core);
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

} // namespace internal

} // cafe::hle
