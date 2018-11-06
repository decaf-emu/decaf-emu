#include "ios_acp_nn_saveservice.h"

#include "nn/acp/nn_acp_result.h"

using namespace nn::acp;
using namespace nn::ipc;

namespace ios::acp::internal
{

nn::Result
SaveService::commandHandler(uint32_t unk1,
                            CommandId command,
                            CommandHandlerArgs &args)
{
   return ResultSuccess;
}

} // namespace ios::acp::internal
