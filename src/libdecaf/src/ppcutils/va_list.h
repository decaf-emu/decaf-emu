#pragma once
#include <cstdint>
#include <common/be_array.h>
#include <common/be_ptr.h>
#include <common/be_val.h>
#include <common/bitutils.h>
#include <common/structsize.h>
#include <libcpu/cpu.h>

namespace ppctypes
{

/**
 * Annotation type for a function to show it takes variable arguments
 */
struct VarArgs
{
};

struct va_list
{
   static const unsigned NumSavedRegs = 8;

   class iterator
   {
   public:
      iterator(va_list *list, unsigned gpr, unsigned fpr) :
         mList(list),
         mGpr(gpr),
         mFpr(fpr)
      {
      }

      template<typename Type>
      typename std::enable_if<sizeof(Type) == 4 && !std::is_floating_point<Type>::value && !std::is_pointer<Type>::value, Type>::type
      next()
      {
         return bit_cast<Type>(nextGpr32());
      }

      template<typename Type>
      typename std::enable_if<sizeof(Type) == 8 && !std::is_floating_point<Type>::value && !std::is_pointer<Type>::value, Type>::type
      next()
      {
         return bit_cast<Type>(nextGpr64());
      }

      template<typename Type>
      typename std::enable_if<std::is_pointer<Type>::value, Type>::type
      next()
      {
         return mem::translate<typename std::remove_pointer<Type>::type>(nextGpr32());
      }

      template<typename Type>
      typename std::enable_if<std::is_floating_point<Type>::value, Type>::type
      next()
      {
         return static_cast<Type>(nextFpr());
      }

   protected:
      uint32_t nextGpr32()
      {
         auto value = uint32_t { 0 };

         if (mGpr < NumSavedRegs) {
            value = mList->reg_save_area[mGpr];
         } else {
            value = mList->overflow_arg_area[mGpr - NumSavedRegs];
         }

         mGpr++;
         return value;
      }

      uint64_t nextGpr64()
      {
         // Align gpr to 64 bit
         if (mGpr % 2) {
            mGpr++;
         }

         auto value = static_cast<uint64_t>(nextGpr32()) << 32;
         value |= nextGpr32();
         return value;
      }

      double nextFpr()
      {
         auto value = double { 0.0 };
         auto fpr_save_area = reinterpret_cast<be_val<double> *>(&mList->reg_save_area[8]);

         if (mFpr < NumSavedRegs) {
            value = fpr_save_area[mFpr];
         } else {
            decaf_abort("How the fuck do we handle va_list with FPR overflow");
         }

         mFpr++;
         return value;
      }

   private:
      va_list *mList;
      unsigned mGpr;
      unsigned mFpr;
   };

   iterator begin()
   {
      return iterator(this, firstGpr, firstFpr);
   }

   //! Index of first GPR
   uint8_t firstGpr;

   //! Index of first FPR
   uint8_t firstFpr;

   uint8_t __padding[2];

   //! Pointer to register values r10+
   be_ptr<be_val<uint32_t>> overflow_arg_area;

   //! Pointer to register values r3...r10 followed by f1...f8 (if saved)
   be_ptr<be_val<uint32_t>> reg_save_area;
};
CHECK_OFFSET(va_list, 0x00, firstGpr);
CHECK_OFFSET(va_list, 0x01, firstFpr);
CHECK_OFFSET(va_list, 0x04, overflow_arg_area);
CHECK_OFFSET(va_list, 0x08, reg_save_area);
CHECK_SIZE(va_list, 0x0C);

/**
 * Structure to help us allocate a va_list on the stack
 */
struct stack_va_list
{
   //! Actual va_list structure
   va_list list;

   //! Padding to 8 byte align
   PADDING(0x4);

   //! Save area for r3...r10
   be_array<uint32_t, 8> gpr_save_area;

   //! Save area for f1...f8
   be_array<double, 8> fpr_save_area;
};
CHECK_OFFSET(stack_va_list, 0x00, list);
CHECK_OFFSET(stack_va_list, 0x10, gpr_save_area);
CHECK_OFFSET(stack_va_list, 0x30, fpr_save_area);
CHECK_SIZE(stack_va_list, 0x70);

static inline va_list *
make_va_list(uint8_t gpr, uint8_t fpr)
{
   auto core = cpu::this_core::state();
   auto overflow = core->gpr[1] + 8 + 8;  // +8 for kcstub() adjustment

   // Allocate space on stack, 8 bytes for weird PPC ABI storage
   core->gpr[1] -= 8 + sizeof(stack_va_list);

   // Setup the va_list structure
   auto stack_list = mem::translate<stack_va_list>(core->gpr[1] + 8);
   stack_list->list.firstGpr = gpr;
   stack_list->list.firstFpr = fpr;
   stack_list->list.reg_save_area = &stack_list->gpr_save_area[0];
   stack_list->list.overflow_arg_area = mem::translate<be_val<uint32_t>>(overflow);

   // Save r3...r10
   for (auto i = 3; i <= 10; ++i) {
      stack_list->gpr_save_area[i - 3] = core->gpr[i];
   }

   // Optionally save f1...f8
   auto saveFloat = !!(core->cr.value & (1 << (31 - 6)));

   if (saveFloat) {
      for (auto i = 1; i <= 8; ++i) {
         stack_list->fpr_save_area[i - 1] = core->fpr[i].value;
      }
   }

   return &stack_list->list;
}

static inline void
free_va_list(va_list *list)
{
   auto core = cpu::this_core::state();
   core->gpr[1] += 8 + sizeof(stack_va_list);
}

} // namespace ppctypes
