#pragma once
#include "cafe/libraries/nn_ec/nn_ec_rootobject.h"
#include "cafe/libraries/nn_ec/nn_ec_noncopyable.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_ec
{

struct Query : RootObject
{
   be2_virt_ptr<void> impl;
};

virt_ptr<Query>
Query_Constructor(virt_ptr<Query> self);

void
Query_Destructor(virt_ptr<Query> self,
                 ghs::DestructorFlags flags);

void
Query_Clear(virt_ptr<Query> self);

} // namespace cafe::nn_ec
