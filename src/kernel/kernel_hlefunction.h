#pragma once
#include <cstdint>
#include "cpu/state.h"
#include "kernel_hleexport.h"
#include "ppcutils/ppcinvoke.h"
#include "common/type_list.h"

namespace kernel
{

struct HleFunction : HleExport
{
   HleFunction() :
      HleExport(HleExport::Function)
   {
   }

   virtual ~HleFunction() override = default;

   virtual void call(cpu::Core *state) = 0;

   bool valid = false;
   uint32_t syscallID = 0;
   uint32_t vaddr = 0;
};

namespace functions
{

template<typename ReturnType, typename... Args>
struct HleFunctionImpl : HleFunction
{
   ReturnType(*wrapped_function)(Args...);

   virtual void call(cpu::Core *thread) override
   {
      ppctypes::invoke(thread, wrapped_function, name);
   }
};

template<typename ReturnType, typename ObjectType, typename... Args>
struct HleMemberFunctionImpl : HleFunction
{
   ReturnType(ObjectType::*wrapped_function)(Args...);

   virtual void call(cpu::Core *thread) override
   {
      ppctypes::invokeMemberFn(thread, wrapped_function, name);
   }
};

template<typename ObjectType, typename... Args>
struct HleConstructorFunctionImpl : HleFunction
{
   static void trampFunction(ObjectType *object, Args... args)
   {
      new (object) ObjectType(args...);
   }

   virtual void call(cpu::Core *thread) override
   {
      ppctypes::invoke(thread, &trampFunction, name);
   }
};

template<typename ObjectType>
struct HleDestructorFunctionImpl : HleFunction
{
   static void trampFunction(ObjectType *object)
   {
      object->~ObjectType();
   }

   virtual void call(cpu::Core *thread) override
   {
      ppctypes::invoke(thread, &trampFunction, name);
   }
};

} // namespace functions

  // Regular Function
template<typename ReturnType, typename... Args>
inline HleFunction *
makeFunction(ReturnType(*fptr)(Args...))
{
   auto func = new kernel::functions::HleFunctionImpl<ReturnType, Args...>();
   func->valid = true;
   func->wrapped_function = fptr;
   return func;
}

// Member Function
template<typename ReturnType, typename Class, typename... Args>
inline HleFunction *
makeFunction(ReturnType(Class::*fptr)(Args...))
{
   auto func = new kernel::functions::HleMemberFunctionImpl<ReturnType, Class, Args...>();
   func->valid = true;
   func->wrapped_function = fptr;
   return func;
}

// Constructor Args
template<typename Class, typename... Args>
inline HleFunction *
makeConstructor()
{
   auto func = new kernel::functions::HleConstructorFunctionImpl<Class, Args...>();
   func->valid = true;
   return func;
}

// Destructor
template<typename Class>
inline HleFunction *
makeDestructor()
{
   auto func = new kernel::functions::HleDestructorFunctionImpl<Class>();
   func->valid = true;
   return func;
}

}  // namespace kernel
