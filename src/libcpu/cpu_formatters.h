#pragma once
#include "address.h"
#include "be2_val.h"
#include "pointer.h"

#include <fmt/format.h>

namespace fmt
{

/*
 * fmtlib formatter for cpu::Address
 */

template<typename AddressType, typename Char>
struct formatter<cpu::Address<AddressType>, Char, void>
{
   template<typename ParseContext>
   constexpr auto parse(ParseContext &ctx)
   {
      return ctx.begin();
   }

   template<typename FormatContext>
   auto format(const cpu::Address<AddressType> &addr, FormatContext &ctx)
   {
      return format_to(ctx.out(), "0x{:08X}", addr.getAddress());
   }
};

/*
 * fmtlib formatter for be2_val<T>
 */

template<typename ValueType, typename Char>
struct formatter<be2_val<ValueType>, Char, void>
   : formatter<typename safe_underlying_type<ValueType>::type, Char, void>
{
   using value_type = typename safe_underlying_type<ValueType>::type;

   template<typename ParseContext>
   constexpr auto parse(ParseContext &ctx)
   {
      return formatter<value_type, Char, void>::parse(ctx);
   }

   template<typename FormatContext>
   auto format(const be2_val<ValueType> &val, FormatContext &ctx)
   {
      return formatter<value_type, Char, void>::format(val, ctx);
   }
};

/*
 * fmtlib formatter for cpu::FunctionPointer
 */

template<typename AddressType, typename FunctionType, typename Char>
struct formatter<cpu::FunctionPointer<AddressType, FunctionType>, Char, void>
{
   template<typename ParseContext>
   constexpr auto parse(ParseContext &ctx)
   {
      return ctx.begin();
   }

   template<typename FormatContext>
   auto format(const cpu::FunctionPointer<AddressType, FunctionType> &ptr, FormatContext &ctx)
   {
      auto addr = cpu::func_pointer_cast_impl<AddressType, FunctionType>::cast(ptr);
      return format_to(ctx.out(), "{:08X}", static_cast<uint32_t>(addr));
   }
};

/*
 * fmtlib formatter for cpu::Pointer
 */

template<typename OutputIt>
inline auto
format_escaped_string(OutputIt iter, const char *data)
{
   iter = format_to(iter, "\"");

   auto hasMoreBytes = true;
   for (auto i = 0; i < 128; ++i) {
      auto c = data[i];
      if (c == 0) {
         hasMoreBytes = false;
         break;
      }

      if (c >= ' ' && c <= '~' && c != '\\' && c != '"') {
         iter = format_to(iter, "{}", c);
      } else {
         switch (c) {
         case '"': iter = format_to(iter, "\\\""); break;
         case '\\': iter = format_to(iter, "\\\\"); break;
         case '\t': iter = format_to(iter, "\\t"); break;
         case '\r': iter = format_to(iter, "\\r"); break;
         case '\n': iter = format_to(iter, "\\n"); break;
         default: iter = format_to(iter, "\\x{:02x}", c); break;
         }
      }
   }

   if (!hasMoreBytes) {
      iter = format_to(iter, "\"");
   } else {
      iter = format_to(iter, "\"...");
   }

   return iter;
}

template<typename AddressType, typename Char>
struct formatter<cpu::Pointer<char, AddressType>, Char, void>
{
   template<typename ParseContext>
   constexpr auto parse(ParseContext &ctx)
   {
      return ctx.begin();
   }

   template<typename FormatContext>
   auto format(const cpu::Pointer<char, AddressType> &ptr, FormatContext &ctx)
   {
      if (!ptr) {
         return format_to(ctx.out(), "<NULL>");
      } else {
         auto bytes = ptr.getRawPointer();
         return format_escaped_string(ctx.out(), bytes);
      }
   }
};

template<typename AddressType, typename Char>
struct formatter<cpu::Pointer<const char, AddressType>, Char, void>
{
   template<typename ParseContext>
   constexpr auto parse(ParseContext &ctx)
   {
      return ctx.begin();
   }

   template<typename FormatContext>
   auto format(const cpu::Pointer<const char, AddressType> &ptr, FormatContext &ctx)
   {
      if (!ptr) {
         return format_to(ctx.out(), "<NULL>");
      } else {
         const char *bytes = ptr.getRawPointer();
         return format_escaped_string(ctx.out(), bytes);
      }
   }
};

template<typename ValueType, typename AddressType, typename Char>
struct formatter<cpu::Pointer<ValueType, AddressType>, Char, void>
{
   template<typename ParseContext>
   constexpr auto parse(ParseContext &ctx)
   {
      return ctx.begin();
   }

   template<typename FormatContext>
   auto format(const cpu::Pointer<ValueType, AddressType> &ptr, FormatContext &ctx)
   {
      auto addr = cpu::pointer_cast_impl<AddressType, ValueType *, AddressType>::cast(ptr);
      return format_to(ctx.out(), "0x{:08X}", static_cast<uint32_t>(addr));
   }
};

} // namespace fmt
