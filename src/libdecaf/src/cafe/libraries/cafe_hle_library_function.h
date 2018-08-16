#pragma once
#include "cafe_hle_library_symbol.h"
#include "cafe/cafe_ppc_interface_invoke.h"
#include "cafe/cafe_ppc_interface_invoke_trace.h"

#include <libcpu/cpu.h>
#include <memory>

namespace cafe::hle
{

struct UnimplementedLibraryFunction
{
   class Library *library;
   std::string name;
   uint32_t syscallID;
};

struct LibraryFunction : public LibrarySymbol
{
   LibraryFunction() :
      LibrarySymbol(LibrarySymbol::Function)
   {
   }

   virtual void call(cpu::Core *state) = 0;

   //! ID number of syscall
   uint32_t syscallID;

   // TODO: Reimplement function tracing!
   bool traceEnabled = true;

   //! Pointer to host function pointer, only set for internal functions
   virt_ptr<void> *hostPtr;
};

namespace internal
{

/**
 * Handles call to both global functions and member functions.
 */
template<typename FunctionType>
struct LibraryFunctionCall : LibraryFunction
{
   FunctionType func;

   virtual void call(cpu::Core *state) override
   {
      if (traceEnabled) {
         invoke_trace(state, func, name.c_str());
      }

      invoke(state, func);
   }
};

/**
 * Handles calls to constructors.
 */
template<typename ObjectType, typename... ArgTypes>
struct LibraryConstructorFunction : LibraryFunction
{
   static void wrapper(virt_ptr<ObjectType> obj, ArgTypes... args)
   {
      ::new(static_cast<void *>(obj.getRawPointer())) ObjectType(args...);
   }

   virtual void call(cpu::Core *state) override
   {
      if (traceEnabled) {
         invoke_trace(state, &LibraryConstructorFunction::wrapper, name.c_str());
      }

      invoke(state, &LibraryConstructorFunction::wrapper);
   }
};

/**
 * Handles calls to destructors.
 */
template<typename ObjectType>
struct LibraryDestructorFunction : LibraryFunction
{
   static void wrapper(virt_ptr<ObjectType> obj)
   {
      (obj.getRawPointer())->~ObjectType();
   }

   virtual void call(cpu::Core *state) override
   {
      if (traceEnabled) {
         invoke_trace(state, &LibraryDestructorFunction::wrapper, name.c_str());
      }

      invoke(state, &LibraryDestructorFunction::wrapper);
   }
};

template<typename FunctionType>
inline std::unique_ptr<LibraryFunction>
makeLibraryFunction(FunctionType func)
{
   auto libraryFunction = new LibraryFunctionCall<FunctionType>();
   libraryFunction->func = func;
   return std::unique_ptr<LibraryFunction> { libraryFunction };
}

template<typename ObjectType, typename... ArgTypes>
inline std::unique_ptr<LibraryFunction>
makeLibraryConstructorFunction()
{
   auto libraryFunction = new LibraryConstructorFunction<ObjectType, ArgTypes...>();
   return std::unique_ptr<LibraryFunction> { libraryFunction };
}

template<typename ObjectType>
inline std::unique_ptr<LibraryFunction>
makeLibraryDestructorFunction()
{
   auto libraryFunction = new LibraryDestructorFunction<ObjectType>();
   return std::unique_ptr<LibraryFunction> { libraryFunction };
}

} // namespace internal

} // cafe::hle
