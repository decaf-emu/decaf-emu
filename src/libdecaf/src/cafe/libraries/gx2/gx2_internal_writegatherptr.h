#pragma once
#include <cstring>
#include <libcpu/be2_struct.h>

namespace cafe::gx2::internal
{

/**
 * A very simple class to emulate a write gather pointer.
 */
template<typename Type>
class be2_write_gather_ptr
{
public:
   be2_val<Type> &
   operator *()
   {
      return *(mPointer++);
   }

   void
   write(Type value)
   {
      *(mPointer++) = value;
   }

   void
   write(virt_ptr<Type> buffer,
         uint32_t count)
   {
      std::memcpy(mPointer.get(), buffer.get(), count);
      mPointer += count;
   }

   virt_ptr<Type>
   get()
   {
      return mPointer;
   }

   be2_write_gather_ptr &
   operator =(virt_ptr<Type> ptr)
   {
      mPointer = ptr;
      return *this;
   }

private:
   be2_virt_ptr<Type> mPointer;
};

} // namespace cafe::gx2::internal
