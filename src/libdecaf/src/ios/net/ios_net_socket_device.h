#pragma once
#include "ios_net_enum.h"
#include "ios_net_socket_request.h"
#include "ios_net_socket_response.h"
#include "ios_net_socket_types.h"

#include "ios/ios_enum.h"

#include <array>
#include <chrono>
#include <memory>
#include <optional>
#include <vector>
#include <uv.h>

namespace ios::kernel
{
struct ResourceRequest;
};

namespace ios::net::internal
{

/**
 * \ingroup ios_net
 * @{
 */

class SocketDevice
{
public:
   struct PendingWrite
   {
      phys_ptr<kernel::ResourceRequest> resourceRequest;
      std::unique_ptr<uv_write_t> handle;
      uint32_t sendBytes;
   };

   struct Socket
   {
      enum Type
      {
         Unused,
         Tcp,
         Udp,
      };

      SocketDevice *device = nullptr;
      bool nonBlocking = false;
      Type type = Unused;
      std::unique_ptr<uv_handle_t> handle;
      std::unique_ptr<uv_connect_t> connect;
      phys_ptr<kernel::ResourceRequest> connectRequest = nullptr;
      bool connected = false;
      Error except = Error::OK;
      std::vector<char> readBuffer;
      std::vector<phys_ptr<kernel::ResourceRequest>> pendingReads;
      std::vector<PendingWrite> pendingWrites;
   };

   struct PendingSelect
   {
      ~PendingSelect() {
         uv_timer_stop(&timer);
      }

      SocketDevice *device;
      phys_ptr<kernel::ResourceRequest> resourceRequest;
      uv_timer_t timer;
   };

public:
   std::optional<Error>
   accept(phys_ptr<kernel::ResourceRequest> resourceRequest,
          SocketHandle fd,
          phys_ptr<SocketAddrIn> sockAddr,
          int32_t sockAddrLen);

   std::optional<Error>
   bind(phys_ptr<kernel::ResourceRequest> resourceRequest,
        SocketHandle fd,
        phys_ptr<SocketAddrIn> sockAddr,
        int32_t sockAddrLen);

   std::optional<Error>
   createSocket(phys_ptr<kernel::ResourceRequest> resourceRequest,
                int32_t family,
                int32_t type,
                int32_t proto);

   std::optional<Error>
   closeSocket(phys_ptr<kernel::ResourceRequest> resourceRequest,
               SocketHandle fd);

   std::optional<Error>
   connect(phys_ptr<kernel::ResourceRequest> resourceRequest,
           SocketHandle fd,
           phys_ptr<SocketAddrIn> sockAddr,
           int32_t sockAddrLen);

   std::optional<Error>
   dnsQuery(phys_ptr<kernel::ResourceRequest> resourceRequest,
            phys_ptr<SocketDnsQueryRequest> request,
            phys_ptr<SocketDnsQueryResponse> response);

   std::optional<Error>
   getpeername(phys_ptr<kernel::ResourceRequest> resourceRequest,
               phys_ptr<const SocketGetPeerNameRequest> request,
               phys_ptr<SocketGetPeerNameResponse> response);

   std::optional<Error>
   recv(phys_ptr<kernel::ResourceRequest> resourceRequest,
        phys_ptr<const SocketRecvRequest> request,
        phys_ptr<char> alignedBeforeBuffer,
        uint32_t alignedBeforeLength,
        phys_ptr<char> alignedBuffer,
        uint32_t alignedLength,
        phys_ptr<char> alignedAfterBuffer,
        uint32_t alignedAfterLength);

   std::optional<Error>
   send(phys_ptr<kernel::ResourceRequest> resourceRequest,
        phys_ptr<const SocketSendRequest> request,
        phys_ptr<char> alignedBeforeBuffer,
        uint32_t alignedBeforeLength,
        phys_ptr<char> alignedBuffer,
        uint32_t alignedLength,
        phys_ptr<char> alignedAfterBuffer,
        uint32_t alignedAfterLength);

   std::optional<Error>
   setsockopt(phys_ptr<kernel::ResourceRequest> resourceRequest,
              phys_ptr<const SocketSetSockOptRequest> request,
              phys_ptr<void> optval,
              uint32_t optlen);

   std::optional<Error>
   select(phys_ptr<kernel::ResourceRequest> resourceRequest,
          phys_ptr<const SocketSelectRequest> request,
          phys_ptr<SocketSelectResponse> response);

   std::optional<Error>
   listen(phys_ptr<kernel::ResourceRequest> resourceRequest,
          SocketHandle fd,
          int32_t backlog);

   void checkPendingSelects();
   void checkPendingReads(Socket *socket);

protected:
   Socket *getSocket(int sockfd);
   std::optional<Error> checkSelect(phys_ptr<kernel::ResourceRequest> resourceRequest);
   std::optional<Error> checkRecv(phys_ptr<kernel::ResourceRequest> resourceRequest);

   static void uvExpirePendingSelectCallback(uv_timer_t *timer);

private:
   std::array<Socket, 64> mSockets;
   std::vector<std::unique_ptr<PendingSelect>> mPendingSelects;
};

/** @} */

} // namespace ios::net::internal
