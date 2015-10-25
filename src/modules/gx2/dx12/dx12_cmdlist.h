#pragma once
#include <type_traits>
#include "types.h"
#include "utils/type_list.h"

struct CommandListRef {
   uint8_t *data;
   uint32_t size;
   uint32_t &offset;
};

template <typename Type>
inline const Type& commandListReadVal(CommandListRef& cl)
{
   const Type& res = *reinterpret_cast<Type*>(&cl.data[cl.offset]);
   cl.offset += sizeof(Type);
   return res;
}

template <typename... FnArgs, typename... Args>
inline void commandListInvoke2(CommandListRef& cl, void(*fn)(FnArgs...), type_list<>, Args&... args)
{
   fn(args...);
}

template <typename... FnArgs, typename... Args, typename Head, typename... Tail>
inline void commandListInvoke2(CommandListRef& cl, void(*fn)(FnArgs...), type_list<Head, Tail...>, Args&... args)
{
   commandListInvoke2(cl, fn, type_list<Tail...>{}, args..., commandListReadVal<Head>(cl));
}

template <typename ...FnArgs>
inline void commandListInvoke(CommandListRef& cl, void(*fn)(FnArgs...))
{
   commandListInvoke2(cl, fn, type_list<FnArgs...>{});
}

typedef void(*InvokerFn)(CommandListRef& cl);

inline void commandListExecute(CommandListRef& cl)
{
   while (cl.offset < cl.size) {
      auto fn = commandListReadVal<InvokerFn>(cl);
      fn(cl);
   }
}

template <typename FnType, typename Type>
inline void commandListAppendVal(CommandListRef& cl, const Type& val)
{
   static_assert(std::is_convertible<Type, FnType>::value, "Parameter passed does not match function declaration.");
   static_assert(std::is_trivially_copyable<FnType>::value, "All command list functor parameters must be trivial.");

   size_t curIdx = cl.offset;
   cl.offset += sizeof(FnType);
   *reinterpret_cast<FnType*>(&cl.data[curIdx]) = val;
}

inline void commandListAppend3(CommandListRef& cl, type_list<>)
{
}

template <typename FnHead, typename... FnTail, typename Head, typename... Tail>
inline void commandListAppend3(CommandListRef& cl, type_list<FnHead, FnTail...>, Head head, Tail... tail)
{
   commandListAppendVal<FnHead, Head>(cl, head);
   commandListAppend3(cl, type_list<FnTail...>{}, tail...);
}

template <typename... FnArgs, typename... Args>
inline void commandListAppend2(CommandListRef& cl, void(*fn)(FnArgs...), Args... args)
{
   commandListAppend3(cl, type_list<FnArgs...>{}, args...);
}

template <typename FnType, FnType Func, typename... Args>
inline void commandListAppend1(CommandListRef& cl, Args... args)
{
   auto invoker = [](CommandListRef& cl) { commandListInvoke(cl, Func); };
   commandListAppendVal<InvokerFn>(cl, static_cast<InvokerFn>(invoker));
   commandListAppend2(cl, Func, args...);
}
