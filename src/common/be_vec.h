#pragma once
#include "common/be_val.h"

template<typename Type>
struct be_vec2
{
   be_val<Type> x;
   be_val<Type> y;
};

template<typename Type>
struct be_vec3
{
   be_val<Type> x;
   be_val<Type> y;
   be_val<Type> z;
};
