#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <libcpu/be2_struct.h>

TEST_CASE("be_* address of returns virt_ptr")
{
   // "virt_addrof be_val<uint32_t>" returns "virt_ptr<uint32_t>"
   REQUIRE((std::is_same<virt_ptr<uint32_t>, decltype(virt_addrof(std::declval<be2_val<uint32_t>>()))>::value));

   // "virt_addrof be_struct<SomeStructure>" returns "virt_ptr<SomeStructure>"
   struct SomeStructure { };
   REQUIRE((std::is_same<virt_ptr<SomeStructure>, decltype(virt_addrof(std::declval<be2_struct<SomeStructure>>()))>::value));

   // "virt_addrof be_array<char, 100>" returns "vitr_ptr<char>"
   REQUIRE((std::is_same<virt_ptr<char>, decltype(virt_addrof(std::declval<be2_array<char, 100>>()))>::value));
}

TEST_CASE("virt_ptr dereference")
{
   // Dereferencing "virt_ptr<uint32_t>" returns "be2_val<uint32_t> &"
   REQUIRE((std::is_same<be2_val<uint32_t> &, decltype(*std::declval<virt_ptr<uint32_t>>())>::value));

   // Dereferencing "virt_ptr<const uint32_t>" returns "const be_val<const uint32_t> &"
   REQUIRE((std::is_same<const be2_val<const uint32_t> &, decltype(*std::declval<virt_ptr<const uint32_t>>())>::value));
}

TEST_CASE("virt_ptr cast")
{
   // virt_ptr<T> can be cast to virt_ptr<void>
   REQUIRE((std::is_constructible<virt_ptr<void>, virt_ptr<uint32_t>>::value));

   // virt_ptr<X> can not be cast to virt_ptr<Y>
   REQUIRE((!std::is_constructible<virt_ptr<uint32_t>, virt_ptr<void>>::value));
   REQUIRE((!std::is_constructible<virt_ptr<uint64_t>, virt_ptr<uint32_t>>::value));

   // Assign virt_ptr<uint32_t> to be2_ptr<uint32_t>
   REQUIRE((std::is_assignable<be2_ptr<uint32_t>, virt_ptr<uint32_t>>::value));

   // Can construct virt_ptr<const void> from virt_ptr<void>
   REQUIRE((std::is_constructible<virt_ptr<const void>, virt_ptr<void>>::value));
}

TEST_CASE("be_val values")
{
   // Assign uint32_t to be2_val<uint32_t>
   REQUIRE((std::is_assignable<be2_val<uint32_t>, uint32_t>::value));

   // Array access of be_array<uint32_t, N> returns a be_val<uint32_t> &
   REQUIRE((std::is_same<be2_val<uint32_t> &, decltype(std::declval<be2_array<uint32_t, 100>>()[1])>::value));
}
