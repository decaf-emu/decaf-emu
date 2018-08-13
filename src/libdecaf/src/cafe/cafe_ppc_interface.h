#pragma once
#include <cstdint>
#include <common/type_traits.h>
#include <libcpu/be2_struct.h>
#include <type_traits>

namespace cafe
{

/**
 * Type used to indicate function takes variable arguments.
 */
struct var_args
{
   // Set to the index of the first gpr register to use.
   uint32_t gpr;

   // Set to the index of the first fpr register to use.
   uint32_t fpr;
};

namespace detail
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
   VarArgs,
};

/**
 * A type T is stored in an FPR register if it is:
 * - A floating point type
 */
template<typename T>
using is_fpr_type = is_true<std::is_floating_point<T>::value>;

/**
 * Check if a type looks like a common/bitfield.h defined bitfield.
 */
template<class T>
struct void_t { typedef void type; };

template<class T, class U = void>
struct is_bitfield_type : std::false_type { };

template<class T>
struct is_bitfield_type<T, typename void_t<typename T::BitfieldType>::type> : std::true_type { };

/**
 * A type T is stored in a single GPR register if it is:
 * - sizeof(T) <= 4
 * - Not a floating point
 * - Not a var_args type
 * - If it is a cpu func pointer
 * - If it is a cpu pointer
 * - If a uint32_t can be constructed from T.
 *
 * TODO: Remove std::is_pointer when CafeOS no longer uses raw pointers.
 */
template<typename T>
using is_gpr32_type = is_true<
   sizeof(T) <= 4 &&
   !std::is_floating_point<T>::value &&
   !std::is_same<var_args, T>::value &&
   (std::is_constructible<typename safe_underlying_type<T>::type, uint32_t>::value ||
    cpu::is_cpu_pointer<T>::value ||
    cpu::is_cpu_func_pointer<T>::value ||
    is_bitfield_type<T>::value)>;

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

template<typename T>
using is_var_args_type = is_true<
   std::is_same<var_args, T>::value>;

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

template<typename T>
struct register_type<T, typename std::enable_if<is_var_args_type<T>::value>::type>
{
   static constexpr auto value = RegisterType::VarArgs;
   static constexpr auto return_index = 0;
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

template<std::size_t GprIndex, std::size_t FprIndex>
struct register_index<RegisterType::VarArgs, GprIndex, FprIndex>
{
   static constexpr auto value = GprIndex | (FprIndex << 8);
   static constexpr auto gpr_next = -1;
   static constexpr auto fpr_next = -1;
};

// An empty type to store function parameter info
template<typename ValueType, RegisterType Type, auto Index>
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
   using head_register_type = register_type<std::remove_cv_t<Head>>;
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

   using return_type = register_type<std::remove_cv_t<ReturnType>>;
   using return_info = param_info_t<ReturnType, return_type::value, return_type::return_index>;
   using param_info = typename get_param_infos_impl<3, 1, ArgTypes...>::type;
};

template<typename ObjectType, typename ReturnType, typename... ArgTypes>
struct function_traits<ReturnType(ObjectType::*)(ArgTypes...)>
{
   static constexpr auto is_member_function = true;
   static constexpr auto num_args = sizeof...(ArgTypes);
   static constexpr auto has_return_value = !std::is_void<ReturnType>::value;

   using return_type = register_type<std::remove_cv_t<ReturnType>>;
   using return_info = param_info_t<ReturnType, return_type::value, return_type::return_index>;
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

} // namespace detail

} // namespace cafe
