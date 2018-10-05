#pragma once
#include "nn_olv_initializeparam.h"

#include "cafe/libraries/nn_result.h"
#include <libcpu/be2_struct.h>

namespace cafe::nn::olv
{

struct MainAppParam
{
};
UNKNOWN_SIZE(MainAppParam);

nn::Result
Initialize(virt_ptr<InitializeParam> initParam);

nn::Result
Initialize(virt_ptr<MainAppParam> mainAppParam,
           virt_ptr<InitializeParam> initParam);

nn::Result
Finalize();

bool
IsInitialized();

}  // namespace cafe::nn::olv
