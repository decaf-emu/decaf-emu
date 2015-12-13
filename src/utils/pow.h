#pragma once

template<typename Type>
inline Type
Log2(Type x)
{
   Type y = 0;

   while (x > 1) {
      x >>= 1;
      y++;
   }

   return y;
}
