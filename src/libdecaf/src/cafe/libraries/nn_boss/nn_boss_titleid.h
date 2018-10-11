#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::nn::boss
{

class TitleID
{
public:
   TitleID();
   TitleID(virt_ptr<TitleID> other);
   TitleID(uint64_t id);

   bool
   IsValid();

   uint64_t
   GetValue();

   uint32_t
   GetTitleID();

   uint32_t
   GetTitleCode();

   uint32_t
   GetUniqueId();

   bool
   operator ==(virt_ptr<TitleID> other);

   bool
   operator !=(virt_ptr<TitleID> other);

private:
   be2_val<uint32_t> mLower;
   be2_val<uint32_t> mUpper;

protected:
   CHECK_MEMBER_OFFSET_BEG
      CHECK_OFFSET(TitleID, 0x00, mLower);
      CHECK_OFFSET(TitleID, 0x04, mUpper);
   CHECK_MEMBER_OFFSET_END
};
CHECK_SIZE(TitleID, 8);

} // namespace cafe::nn::boss
