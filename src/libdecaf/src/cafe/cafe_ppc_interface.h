#pragma once
#include <cstdint>
#include <common/type_traits.h>
#include <libcpu/be2_struct.h>
#include <type_traits>

namespace cafe::detail
{

/**
 * The register a type is stored in.
 *
 * We must distinguish between gpr for 32 and 64 bit values because a 64 bit
 * value has register index alignment in the PPC calling convention.
 */
enum class RegisterType
{
   Gpr32,
   Gpr64,
   Fpr,
   Void,
};

/**
 * A type T is stored in an FPR register if it is:
 * - A floating point type
 */
template<typename T>
using is_fpr_type = is_true<std::is_floating_point<T>::value>;

/**
 * A type T is stored in a single GPR register if it is:
 * - sizeof(T) <= 4
 * - Not a floating point
 * - If a uint32_t can be constructed from T or it is a virt_ptr
 *
 * TODO: Remove std::is_pointer when CafeOS no longer uses raw pointers.
 */
template<typename T>
using is_gpr32_type = is_true<
   sizeof(T) <= 4 &&
   !std::is_floating_point<T>::value &&
   (std::is_constructible<typename safe_underlying_type<T>::type, uint32_t>::value ||
    is_virt_ptr<T>::value ||
    std::is_pointer<T>::value)>;

/**
 * A type T is stored in two aligned GPR registers if it is:
 * - sizeof(T) == 8
 * - Not a floating point
 * - If a uint64_t can be constructed from T
 */
template<typename T>
using is_gpr64_type = is_true<
   sizeof(T) == 8 &&
   !std::is_floating_point<T>::value &&
   !std::is_pointer<T>::value &&
   std::is_constructible<typename safe_underlying_type<T>::type, uint64_t>::value>;

// Gets the register type for a type T.
template<typename, typename T2 = void>
struct register_type;

template<typename T>
struct register_type<T, typename std::enable_if<std::is_void<T>::value>::type>
{
   static constexpr auto value = RegisterType::Void;
   static constexpr auto return_index = 0;
};

template<typename T>
struct register_type<T, typename std::enable_if<is_fpr_type<T>::value>::type>
{
   static constexpr auto value = RegisterType::Fpr;
   static constexpr auto return_index = 1;
};

template<typename T>
struct register_type<T, typename std::enable_if<is_gpr32_type<T>::value>::type>
{
   static constexpr auto value = RegisterType::Gpr32;
   static constexpr auto return_index = 3;
};

template<typename T>
struct register_type<T, typename std::enable_if<is_gpr64_type<T>::value>::type>
{
   static constexpr auto value = RegisterType::Gpr64;
   static constexpr auto return_index = 3;
};

// Prepends a type T to a tuple
template<typename T, typename Ts>
struct tuple_prepend;

template<typename T, typename... Ts>
struct tuple_prepend<T, std::tuple<Ts...>>
{
   using type = std::tuple<T, Ts...>;
};

template<typename T>
struct tuple_prepend<T, void>
{
   using type = std::tuple<T>;
};

// Calculate the index for a given RegisterType
template<RegisterType type, std::size_t GprIndex, std::size_t FprIndex>
struct register_index;

template<std::size_t GprIndex, std::size_t FprIndex>
struct register_index<RegisterType::Gpr32, GprIndex, FprIndex>
{
   static constexpr auto value = GprIndex;
   static constexpr auto gpr_next = value + 1;
   static constexpr auto fpr_next = FprIndex;
};

template<std::size_t GprIndex, std::size_t FprIndex>
struct register_index<RegisterType::Gpr64, GprIndex, FprIndex>
{
   static constexpr auto value = ((GprIndex % 2) == 0) ? (GprIndex + 1) : GprIndex;
   static constexpr auto gpr_next = value + 2;
   static constexpr auto fpr_next = FprIndex;
};

template<std::size_t GprIndex, std::size_t FprIndex>
struct register_index<RegisterType::Fpr, GprIndex, FprIndex>
{
   static constexpr auto value = FprIndex;
   static constexpr auto gpr_next = GprIndex;
   static constexpr auto fpr_next = value + 1;
};

// An empty type to store function parameter info
template<typename ValueType, RegisterType Type, std::size_t Index>
struct param_info_t
{
   using type = ValueType;
   static constexpr auto reg_index = Index;
   static constexpr auto reg_type = Type;
};

// Calculates a std::tuple<param_info_t...> type for a list of types
template<std::size_t GprIndex, std::size_t FprIndex, typename... Ts>
struct get_param_infos_impl;

template<std::size_t GprIndex, std::size_t FprIndex, typename Head, typename... Tail>
struct get_param_infos_impl<GprIndex, FprIndex, Head, Tail...>
{
   using head_register_type = register_type<Head>;
   using head_arg_index = register_index<head_register_type::value, GprIndex, FprIndex>;
   using type = typename tuple_prepend<
      param_info_t<Head, head_register_type::value, head_arg_index::value>,
      typename get_param_infos_impl<head_arg_index::gpr_next, head_arg_index::fpr_next, Tail...>::type
   >::type;
};

template<std::size_t GprIndex, std::size_t FprIndex>
struct get_param_infos_impl<GprIndex, FprIndex>
{
   using type = std::tuple<>;
};

// Stores information about a function
template<typename T>
struct function_traits;

template<typename ReturnType, typename... ArgTypes>
struct function_traits<ReturnType(ArgTypes...)>
{
   static constexpr auto is_member_function = false;
   static constexpr auto num_args = sizeof...(ArgTypes);
   static constexpr auto has_return_value = !std::is_void<ReturnType>::value;

   using return_info = param_info_t<ReturnType, register_type<ReturnType>::value, register_type<ReturnType>::return_index>;
   using param_info = typename get_param_infos_impl<3, 1, ArgTypes...>::type;
};

template<typename ObjectType, typename ReturnType, typename... ArgTypes>
struct function_traits<ReturnType(ObjectType::*)(ArgTypes...)>
{
   static constexpr auto is_member_function = true;
   static constexpr auto num_args = sizeof...(ArgTypes);
   static constexpr auto has_return_value = !std::is_void<ReturnType>::value;

   using return_info = param_info_t<ReturnType, register_type<ReturnType>::value, register_type<ReturnType>::return_index>;
   using param_info = typename get_param_infos_impl<4, 1, ArgTypes...>::type;
   using object_info = param_info_t<virt_ptr<ObjectType>, RegisterType::Gpr32, 3>;
};

template<typename ReturnType, typename... ArgTypes>
struct function_traits<ReturnType(*)(ArgTypes...)> : function_traits<ReturnType(ArgTypes...)>
{
};

template <typename T>
struct function_traits<T&> : function_traits<T> { };

template <typename T>
struct function_traits<const T&> : function_traits<T> { };

template <typename T>
struct function_traits<volatile T&> : function_traits<T> { };

template <typename T>
struct function_traits<const volatile T&> : function_traits<T> { };

template <typename T>
struct function_traits<T&&> : function_traits<T> { };

template <typename T>
struct function_traits<const T&&> : function_traits<T> { };

template <typename T>
struct function_traits<volatile T&&> : function_traits<T> { };

template <typename T>
struct function_traits<const volatile T&&> : function_traits<T> { };

} // namespace cafe::detail
