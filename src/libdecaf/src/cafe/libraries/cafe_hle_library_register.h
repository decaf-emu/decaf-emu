#pragma once
#include "cafe_hle_library.h"
#include "cafe_hle_library_function.h"
#include "cafe_hle_library_data.h"

#include "coreinit/coreinit_enum.h"

namespace cafe::coreinit
{

using OSDynLoad_ModuleHandle = uint32_t;

namespace internal
{

OSDynLoad_Error
relocateHleLibrary(OSDynLoad_ModuleHandle moduleHandle);

} // namespace internal

} // namespace cafe::coreinit

namespace cafe::hle
{

typedef int32_t(&RplEntryFunctionType)(coreinit::OSDynLoad_ModuleHandle moduleHandle,
                                       coreinit::OSDynLoad_EntryReason reason);

template<RplEntryFunctionType Fn>
static int32_t
cafe_rpl_crt(coreinit::OSDynLoad_ModuleHandle moduleHandle,
             coreinit::OSDynLoad_EntryReason reason)
{
   coreinit::internal::relocateHleLibrary(moduleHandle);
   return Fn(moduleHandle, reason);
}

template<int dummy>
static int32_t
cafe_generic_rpl_crt(coreinit::OSDynLoad_ModuleHandle moduleHandle,
                     coreinit::OSDynLoad_EntryReason reason)
{
   coreinit::internal::relocateHleLibrary(moduleHandle);
   return 0;
}

template<typename FunctionType, FunctionType Fn>
static void
registerNoCrtEntryPoint(hle::Library *library,
                        const std::string &name)
{
   auto symbol = internal::makeLibraryFunction<FunctionType, Fn>(name);
   symbol->exported = true;
   library->registerSymbol(name, std::move(symbol));
   library->setEntryPointSymbolName(name);
}

template<RplEntryFunctionType Fn>
static void
registerEntryPoint(hle::Library *library,
                   const std::string &name)
{
   auto symbol = internal::makeLibraryFunction<RplEntryFunctionType, Fn>(name);
   symbol->exported = true;
   library->registerSymbol(name, std::move(symbol));

   static const std::string crtEntryName = "__rpl_crt";
   registerNoCrtEntryPoint<RplEntryFunctionType, cafe_rpl_crt<Fn>>(library, crtEntryName);
}

template<typename FunctionType, FunctionType Fn>
static void
registerFunctionInternal(hle::Library *library,
                         const char *name,
                         virt_func_ptr<typename std::remove_pointer<FunctionType>::type>& hostPtr)
{
   auto symbol = internal::makeLibraryFunction<FunctionType, Fn>(name);
   symbol->exported = false;
   symbol->hostPtr = reinterpret_cast<virt_ptr<void>*>(&hostPtr);
   library->registerSymbol(name, std::move(symbol));
}

template<typename FunctionType, FunctionType Fn>
static void
registerFunctionInternal(hle::Library *library,
                         const char *name)
{
   auto symbol = internal::makeLibraryFunction<FunctionType, Fn>(name);
   symbol->exported = false;
   library->registerSymbol(name, std::move(symbol));
}

template<typename FunctionType, FunctionType Fn>
static void
registerFunctionExport(hle::Library *library,
                       const char *name)
{
   auto symbol = internal::makeLibraryFunction<FunctionType, Fn>(name);
   symbol->exported = true;
   library->registerSymbol(name, std::move(symbol));
}

template<typename DataType>
static void
registerDataInternal(hle::Library *library,
                     const char *name,
                     virt_ptr<DataType> &data)
{
   auto symbol = std::make_unique<LibraryData>();
   symbol->exported = false;
   symbol->hostPointer = reinterpret_cast<virt_ptr<void>*>(&data);
   symbol->constructor = [](void* ptr) { new (ptr) DataType(); };
   symbol->size = sizeof(DataType);
   symbol->align = alignof(DataType);
   library->registerSymbol(name, std::move(symbol));
}

template<typename DataType>
static void
registerDataExport(hle::Library *library,
                   const char *name,
                   virt_ptr<DataType> &data)
{
   auto symbol = std::make_unique<LibraryData>();
   symbol->exported = true;
   symbol->hostPointer = reinterpret_cast<virt_ptr<void>*>(&data);
   symbol->constructor = [](void* ptr) { new (ptr) DataType(); };
   symbol->size = sizeof(DataType);
   symbol->align = alignof(DataType);
   library->registerSymbol(name, std::move(symbol));
}

namespace detail
{
template<typename T>
class has_ghs_virtual_table
{
    typedef char yes_type;
    typedef long no_type;
    template <typename U> static yes_type test(decltype(&U::VirtualTable));
    template <typename U> static no_type  test(...);
public:
    static constexpr bool value = sizeof(test<T>(0)) == sizeof(yes_type);
};
} // namespace detail

template<typename ObjectType>
static void
registerTypeInfo(hle::Library *library,
                 const char *typeName,
                 std::vector<const char *> &&virtualTable,
                 std::vector<const char *> &&baseTypes,
                 const char *typeIdSymbol = nullptr)
{
   auto typeInfo = LibraryTypeInfo { };
   typeInfo.name = typeName;
   typeInfo.typeIdSymbol = typeIdSymbol;
   typeInfo.hostTypeDescriptorPtr = &ObjectType::TypeDescriptor;

   typeInfo.virtualTable = std::move(virtualTable);
   typeInfo.baseTypes = std::move(baseTypes);

   if constexpr (detail::has_ghs_virtual_table<ObjectType>::value) {
      typeInfo.hostVirtualTablePtr = &ObjectType::VirtualTable;
   }

   library->registerTypeInfo(std::move(typeInfo));
}

#define fnptr_decltype(Func) \
    std::conditional<std::is_function<decltype(Func)>::value, std::add_pointer<decltype(Func)>::type, decltype(Func)>::type

#define RegisterEntryPoint(fn) \
   cafe::hle::registerEntryPoint<fn>(this, #fn)

#define RegisterNoCrtEntryPoint(fn) \
   cafe::hle::registerNoCrtEntryPoint<fnptr_decltype(fn), fn>(this, #fn)

#define RegisterGenericEntryPoint() \
   RegisterNoCrtEntryPoint(cafe_generic_rpl_crt<0>)

#define RegisterFunctionExport(fn) \
   cafe::hle::registerFunctionExport<fnptr_decltype(fn), fn>(this, #fn)

#define RegisterFunctionExportName(name, fn) \
   cafe::hle::registerFunctionExport<fnptr_decltype(fn), fn>(this, name)

#define RegisterDataExport(data) \
   cafe::hle::registerDataExport(this, #data, data)

#define RegisterDataExportName(name, data) \
   cafe::hle::registerDataExport(this, name, data)

#define RegisterFunctionInternal(fn, ptr) \
   cafe::hle::registerFunctionInternal<fnptr_decltype(fn), fn>(this, "__internal__" # fn, ptr)

#define RegisterFunctionInternalName(name, fn) \
   cafe::hle::registerFunctionInternal<fnptr_decltype(fn), fn>(this, name)

#define RegisterDataInternal(data) \
   cafe::hle::registerDataInternal(this, "__internal__" # data, data)

#define RegisterTypeInfo(type, name, virtualTable, baseTypes, ...) \
   cafe::hle::registerTypeInfo<type>(this, name, virtualTable, baseTypes, __VA_ARGS__);

} // namespace cafe::hle
