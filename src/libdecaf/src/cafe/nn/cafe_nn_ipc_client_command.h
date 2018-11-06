#pragma once
#include "cafe_nn_ipc_bufferallocator.h"

#include "nn/nn_result.h"
#include "nn/ios/nn_ios_error.h"
#include "nn/ipc/nn_ipc_format.h"
#include "nn/ipc/nn_ipc_managedbuffer.h"

#include <array>
#include <libcpu/be2_struct.h>

namespace nn::ipc
{

namespace detail
{

template<typename... Ts>
struct ManagedBufferCount;

template<>
struct ManagedBufferCount<>
{
   static constexpr auto Input = 0;
   static constexpr auto Output = 0;
};

template<typename T, typename... Ts>
struct ManagedBufferCount<T, Ts...>
{
   static constexpr auto Input = 0 + ManagedBufferCount<Ts...>::Input;
   static constexpr auto Output = 0 + ManagedBufferCount<Ts...>::Output;
};

template<typename T, typename... Ts>
struct ManagedBufferCount<InBuffer<T>, Ts...>
{
   static constexpr auto Input = 1 + ManagedBufferCount<Ts...>::Input;
   static constexpr auto Output = 0 + ManagedBufferCount<Ts...>::Output;
};

template<typename T, typename... Ts>
struct ManagedBufferCount<InOutBuffer<T>, Ts...>
{
   static constexpr auto Input = 1 + ManagedBufferCount<Ts...>::Input;
   static constexpr auto Output = 1 + ManagedBufferCount<Ts...>::Output;
};

template<typename T, typename... Ts>
struct ManagedBufferCount<OutBuffer<T>, Ts...>
{
   static constexpr auto Input = 0 + ManagedBufferCount<Ts...>::Input;
   static constexpr auto Output = 1 + ManagedBufferCount<Ts...>::Output;
};

struct ManagedBufferInfo
{
   virt_ptr<void> ipcBuffer = nullptr;

   virt_ptr<void> userBuffer;
   uint32_t userBufferSize;

   virt_ptr<void> unalignedBeforeBuffer;
   uint32_t unalignedBeforeBufferSize;

   virt_ptr<void> alignedBuffer;
   uint32_t alignedBufferSize;

   virt_ptr<void> unalignedAfterBuffer;
   uint32_t unalignedAfterBufferSize;

   bool output;
};

struct ClientCommandData
{
   virt_ptr<BufferAllocator> allocator;
   virt_ptr<void> requestBuffer;
   virt_ptr<void> responseBuffer;
   virt_ptr<::ios::IoctlVec> vecsBuffer;
   ManagedBufferInfo *ioBuffers;
   int numVecIn;
   int numVecOut;
};

template<typename Type>
struct IpcSerialiser
{
   static void
   write(ClientCommandData &data,
         size_t &offset,
         int &inputVecIdx,
         int &outputVecIdx,
         const Type &value)
   {
      auto ptr = virt_cast<Type *>(virt_cast<virt_addr>(data.requestBuffer) + offset);
      *ptr = value;
      offset += sizeof(Type);
   }
};

template<>
struct IpcSerialiser<ManagedBuffer>
{
   static void
   write(ClientCommandData &data,
         size_t &offset,
         int &inputVecIdx,
         int &outputVecIdx,
         const ManagedBuffer &userBuffer)
   {
      // The user buffer pointer is not guaranteed to be aligned so we must
      // split the buffer by separately reading / writing the unaligned data at
      // the start and end of the user buffer.
      auto &ioBuffer = data.ioBuffers[(inputVecIdx + outputVecIdx) / 2];
      ioBuffer.ipcBuffer = data.allocator->allocate(256);
      ioBuffer.userBuffer = userBuffer.ptr;
      ioBuffer.userBufferSize = userBuffer.size;
      ioBuffer.output = userBuffer.output;

      auto midPoint = virt_cast<virt_addr>(ioBuffer.ipcBuffer) + 64;

      auto unalignedStart = virt_cast<virt_addr>(userBuffer.ptr);
      auto unalignedEnd = unalignedStart + userBuffer.size;

      auto alignedStart = align_up(unalignedStart, 64);
      auto alignedEnd = align_down(unalignedEnd, 64);

      ioBuffer.alignedBuffer = virt_cast<void *>(alignedStart);
      ioBuffer.alignedBufferSize =
         static_cast<uint32_t>(alignedEnd - alignedStart);

      ioBuffer.unalignedBeforeBufferSize =
         static_cast<uint32_t>(alignedStart - unalignedStart);
      ioBuffer.unalignedBeforeBuffer =
         virt_cast<void *>(virt_cast<virt_addr>(ioBuffer.ipcBuffer) + 64
                           - ioBuffer.unalignedBeforeBufferSize);

      ioBuffer.unalignedAfterBufferSize =
         static_cast<uint32_t>(unalignedEnd - alignedEnd);
      ioBuffer.unalignedAfterBuffer =
         virt_cast<void *>(virt_cast<virt_addr>(ioBuffer.ipcBuffer) + 64);

      if (userBuffer.input) {
         // Copy the unaligned buffer input
         std::memcpy(ioBuffer.unalignedBeforeBuffer.get(),
                     virt_cast<void *>(unalignedStart).get(),
                     ioBuffer.unalignedBeforeBufferSize);

         std::memcpy(
            ioBuffer.unalignedAfterBuffer.get(),
            virt_cast<void *>(unalignedEnd
                              - ioBuffer.unalignedAfterBufferSize).get(),
            ioBuffer.unalignedAfterBufferSize);
      }

      // Calculate our ioctlv vecs indices
      auto alignedBufferIndex = uint8_t { 0 };
      auto unalignedBufferIndex = uint8_t { 0 };
      auto bufferIndexOffset = uint8_t { 0 };

      if (userBuffer.input) {
         alignedBufferIndex = static_cast<uint8_t>(inputVecIdx++);
         unalignedBufferIndex = static_cast<uint8_t>(inputVecIdx++);
         bufferIndexOffset = 1 + data.numVecOut;
      } else {
         alignedBufferIndex = static_cast<uint8_t>(outputVecIdx++);
         unalignedBufferIndex = static_cast<uint8_t>(outputVecIdx++);
         bufferIndexOffset = 1;
      }

      // Update our ioctlv vecs buffer
      auto &alignedBufferVec =
         data.vecsBuffer[bufferIndexOffset + alignedBufferIndex];
      auto &unalignedBufferVec =
         data.vecsBuffer[bufferIndexOffset + unalignedBufferIndex];

      alignedBufferVec.vaddr = virt_cast<virt_addr>(ioBuffer.alignedBuffer);
      alignedBufferVec.len = ioBuffer.alignedBufferSize;

      if (ioBuffer.unalignedBeforeBufferSize + ioBuffer.unalignedAfterBufferSize) {
         unalignedBufferVec.vaddr =
            virt_cast<virt_addr>(ioBuffer.unalignedBeforeBuffer);
         unalignedBufferVec.len =
            ioBuffer.unalignedBeforeBufferSize
            + ioBuffer.unalignedAfterBufferSize;
      } else {
         unalignedBufferVec.vaddr = virt_addr { 0u };
         unalignedBufferVec.len = 0u;
      }

      // Serialise the buffer info to the request
      auto managedBuffer =
         virt_cast<ManagedBufferParameter *>(
            virt_cast<virt_addr>(data.requestBuffer) + offset);
      managedBuffer->alignedBufferSize = ioBuffer.alignedBufferSize;
      managedBuffer->unalignedBeforeBufferSize =
         static_cast<uint8_t>(ioBuffer.unalignedBeforeBufferSize);
      managedBuffer->unalignedAfterBufferSize =
         static_cast<uint8_t>(ioBuffer.unalignedAfterBufferSize);
      managedBuffer->alignedBufferIndex = alignedBufferIndex;
      managedBuffer->unalignedBufferIndex = unalignedBufferIndex;
      offset += 8;
   }
};

template<typename Type>
struct IpcSerialiser<::nn::ipc::InBuffer<Type>>
{
   static void write(ClientCommandData &data, size_t &offset, int &inputVecIdx,
                     int &outputVecIdx, const ManagedBuffer &userBuffer)
   {
      IpcSerialiser<ManagedBuffer>::write(data, offset, inputVecIdx,
                                          outputVecIdx, userBuffer);
   }
};

template<typename Type>
struct IpcSerialiser<::nn::ipc::InOutBuffer<Type>>
{
   static void write(ClientCommandData &data, size_t &offset, int &inputVecIdx,
                     int &outputVecIdx, const ManagedBuffer &userBuffer)
   {
      IpcSerialiser<ManagedBuffer>::write(data, offset, inputVecIdx,
                                          outputVecIdx, userBuffer);
   }
};

template<typename Type>
struct IpcSerialiser<::nn::ipc::OutBuffer<Type>>
{
   static void write(ClientCommandData &data, size_t &offset, int &inputVecIdx,
                     int &outputVecIdx, const ManagedBuffer &userBuffer)
   {
      IpcSerialiser<ManagedBuffer>::write(data, offset, inputVecIdx,
                                          outputVecIdx, userBuffer);
   }
};

template<typename Type>
struct IpcDeserialiser
{
   static void read(ClientCommandData &data, size_t &offset, Type &value)
   {
      auto ptr =
         virt_cast<Type *>(
            virt_cast<virt_addr>(data.responseBuffer) + offset);
      value = *ptr;
      offset += sizeof(Type);
   }
};

template<int, int, typename... Types>
struct ClientCommandHelper;

template<int ServiceId, int CommandId,
         typename... ParameterTypes,
         typename... ResponseTypes>
struct ClientCommandHelper<ServiceId, CommandId,
                           std::tuple<ParameterTypes...>,
                           std::tuple<ResponseTypes...>>
{
   static constexpr auto NumInputBuffers =
      ManagedBufferCount<ParameterTypes...>::Input;

   static constexpr auto NumOutputBuffers =
      ManagedBufferCount<ParameterTypes...>::Output;

   static constexpr auto NumManagedBuffers = NumInputBuffers + NumOutputBuffers;

public:
   ClientCommandHelper(virt_ptr<BufferAllocator> allocator)
   {
      // Allocate buffers
      mData.allocator = allocator;
      mData.vecsBuffer = virt_cast<::ios::IoctlVec *>(allocator->allocate(128));
      mData.requestBuffer = allocator->allocate(128);
      mData.responseBuffer = allocator->allocate(128);
      mData.ioBuffers = mManagedBufferInfo.data();
      mData.numVecIn = 1 + 2 * NumOutputBuffers;
      mData.numVecOut = 1 + 2 * NumInputBuffers;

      std::memset(mData.vecsBuffer.get(), 0, 128);
      std::memset(mData.requestBuffer.get(), 0, 128);
      std::memset(mData.responseBuffer.get(), 0, 128);

      // Setup request header
      auto request = virt_cast<RequestHeader *>(mData.requestBuffer);
      request->unk0x00 = 1u;
      request->command = static_cast<uint32_t>(CommandId);
      request->unk0x08 = 0u;
      request->service = static_cast<uint32_t>(ServiceId);

      // Setup vecs buffer
      mData.vecsBuffer[0].vaddr =
         virt_cast<virt_addr>(mData.responseBuffer);
      mData.vecsBuffer[0].len = 128u;

      mData.vecsBuffer[mData.numVecIn].vaddr =
         virt_cast<virt_addr>(mData.requestBuffer);
      mData.vecsBuffer[mData.numVecIn].len = 128u;
   }

   ~ClientCommandHelper()
   {
      if (mData.vecsBuffer) {
         mData.allocator->deallocate(mData.vecsBuffer);
      }

      if (mData.requestBuffer) {
         mData.allocator->deallocate(mData.requestBuffer);
      }

      if (mData.responseBuffer) {
         mData.allocator->deallocate(mData.responseBuffer);
      }

      for (auto &ioBuffer : mManagedBufferInfo) {
         if (ioBuffer.ipcBuffer) {
            mData.allocator->deallocate(ioBuffer.ipcBuffer);
         }
      }
   }

public:
   const ClientCommandData &getCommandData()
   {
      return mData;
   }

   void setParameters(ParameterTypes... params)
   {
      auto offset = sizeof(RequestHeader);
      auto inputVecIdx = 0;
      auto outputVecIdx = 0;
      (IpcSerialiser<ParameterTypes>::write(mData, offset, inputVecIdx,
                                            outputVecIdx, params), ...);
   }

   Result readResponse(ResponseTypes &... responses)
   {
      auto header = virt_cast<ResponseHeader *>(mData.responseBuffer);
      auto result = Result { static_cast<uint32_t>(static_cast<int32_t>(header->result)) };

      // Read unaligned output buffer data
      for (auto &ioBuffer : mManagedBufferInfo) {
         if (!ioBuffer.output) {
            continue;
         }

         std::memcpy(ioBuffer.userBuffer.get(),
                     ioBuffer.unalignedBeforeBuffer.get(),
                     ioBuffer.unalignedBeforeBufferSize);

         auto userAfterBufferAddr =
            virt_cast<virt_addr>(ioBuffer.userBuffer)
            + ioBuffer.userBufferSize - ioBuffer.unalignedAfterBufferSize;

         std::memcpy(virt_cast<void *>(userAfterBufferAddr).get(),
                     ioBuffer.unalignedAfterBuffer.get(),
                     ioBuffer.unalignedAfterBufferSize);
      }

      // Read response values
      auto offset = sizeof(ResponseHeader);
      (IpcDeserialiser<ResponseTypes>::read(mData, offset, responses),  ...);

      return result;
   }

private:
   ClientCommandData mData;
   std::array<ManagedBufferInfo, NumManagedBuffers> mManagedBufferInfo;
};

} // namespace detail

template<typename CommandType>
class ClientCommand;

template<typename CommandType>
class ClientCommand :
   public detail::ClientCommandHelper<CommandType::service,
                                      CommandType::command,
                                      typename CommandType::parameters,
                                      typename CommandType::response>
{
public:
   ClientCommand(virt_ptr<BufferAllocator> allocator) :
      detail::ClientCommandHelper<CommandType::service,
                                  CommandType::command,
                                  typename CommandType::parameters,
                                  typename CommandType::response>(allocator)
   {
   }
};

} // namespace nn::ipc
