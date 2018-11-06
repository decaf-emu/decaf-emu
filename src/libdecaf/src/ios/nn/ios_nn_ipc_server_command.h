#pragma once
#include "ios_nn_ipc_server.h"
#include "nn/ipc/nn_ipc_format.h"
#include "nn/ipc/nn_ipc_managedbuffer.h"

namespace nn::ipc
{

namespace detail
{

template<typename Type>
struct IpcDeserialiser
{
   static void
   read(const CommandHandlerArgs &args, size_t &offset, Type &value)
   {
      auto ptr =
         phys_cast<Type *>(phys_cast<phys_addr>(args.requestBuffer) + offset);
      value = *ptr;
      offset += sizeof(Type);
   }
};

template<>
struct IpcDeserialiser<ManagedBuffer>
{
   static void
   read(const CommandHandlerArgs &args,
        size_t &offset,
        ManagedBuffer &value)
   {
      auto managedBuffer =
         phys_cast<ManagedBufferParameter *>(
            phys_cast<phys_addr>(args.requestBuffer) + offset);

      auto alignedBufferIndex = 1 + managedBuffer->alignedBufferIndex;
      auto unalignedBufferIndex = 1 + managedBuffer->unalignedBufferIndex;

      value.alignedBuffer =
         phys_cast<void *>(args.vecs[alignedBufferIndex].paddr);
      value.alignedBufferSize = managedBuffer->alignedBufferSize;

      value.unalignedBeforeBuffer =
         phys_cast<void *>(args.vecs[unalignedBufferIndex].paddr);
      value.unalignedBeforeBufferSize = managedBuffer->unalignedBeforeBufferSize;

      value.unalignedAfterBuffer =
         phys_cast<void *>(args.vecs[unalignedBufferIndex].paddr
                           + managedBuffer->unalignedBeforeBufferSize);
      value.unalignedAfterBufferSize = managedBuffer->unalignedAfterBufferSize;

      offset += sizeof(ManagedBufferParameter);
   }
};

template<typename Type>
struct IpcDeserialiser<ipc::InBuffer<Type>>
{
   static void read(const CommandHandlerArgs &args, size_t &offset,
                    ipc::InBuffer<Type> &value)
   {
      IpcDeserialiser<ManagedBuffer>::read(args, offset, value);
   }
};

template<typename Type>
struct IpcDeserialiser<ipc::InOutBuffer<Type>>
{
   static void read(const CommandHandlerArgs &args, size_t &offset,
                    ipc::InOutBuffer<Type> &value)
   {
      IpcDeserialiser<ManagedBuffer>::read(args, offset, value);
   }
};

template<typename Type>
struct IpcDeserialiser<ipc::OutBuffer<Type>>
{
   static void read(const CommandHandlerArgs &args, size_t &offset,
                    ipc::OutBuffer<Type> &value)
   {
      IpcDeserialiser<ManagedBuffer>::read(args, offset, value);
   }
};

template<typename Type>
struct IpcSerialiser
{
   static void write(const CommandHandlerArgs &args, size_t &offset,
                     const Type &value)
   {
      auto ptr =
         phys_cast<Type *>(phys_cast<phys_addr>(args.responseBuffer) + offset);
      *ptr = value;
      offset += sizeof(Type);
   }
};

template<int, int, typename... Types>
struct ServerCommandHelper;

template<int ServiceId, int CommandId,
         typename... ParameterTypes,
         typename... ResponseTypes>
struct ServerCommandHelper<ServiceId, CommandId,
                           std::tuple<ParameterTypes...>,
                           std::tuple<ResponseTypes...>>
{
   ServerCommandHelper(const CommandHandlerArgs &args) :
      mArgs(args)
   {
   }

   void ReadRequest(ParameterTypes &... parameters)
   {
      auto offset = size_t { 0 };
      (IpcDeserialiser<ParameterTypes>::read(mArgs, offset, parameters), ...);
   }

   void WriteResponse(const ResponseTypes &... responses)
   {
      auto offset = size_t { 0 };
      (IpcSerialiser<ResponseTypes>::write(mArgs, offset, responses), ...);
   }

   const CommandHandlerArgs &mArgs;
};

} // namespace detail

template<typename CommandType>
struct ServerCommand;

template<typename CommandType>
struct ServerCommand :
   detail::ServerCommandHelper<CommandType::service,
                               CommandType::command,
                               typename CommandType::parameters,
                               typename CommandType::response>
{
   ServerCommand(const CommandHandlerArgs &args) :
      detail::ServerCommandHelper<CommandType::service,
                                  CommandType::command,
                                  typename CommandType::parameters,
                                  typename CommandType::response>(args)
   {
   }
};

} // namespace nn::ipc
