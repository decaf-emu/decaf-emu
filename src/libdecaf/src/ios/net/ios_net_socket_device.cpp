#include "ios_net_log.h"
#include "ios_net_socket_async_task.h"
#include "ios_net_socket_device.h"

#include "ios/kernel/ios_kernel_hardware.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"

#include "ios/ios_error.h"
#include "ios/ios_network_thread.h"

#include <atomic>
#include <ares.h>
#include <common/platform.h>
#include <common/strutils.h>
#include <libcpu/cpu_formatters.h>
#include <functional>
#include <mutex>
#include <uv.h>
#include <vector>

using namespace ios::kernel;
using ios::kernel::internal::setInterruptAhbAll;
using ios::internal::networkUvLoop;

namespace ios::net::internal
{

static constexpr int SO_AF_UNSPEC = 0;
static constexpr int SO_AF_INET = 2;

static constexpr int SO_SOCK_STREAM = 1;
static constexpr int SO_SOCK_DGRAM = 2;

static constexpr int SO_IPPROTO_IP = 0;
static constexpr int SO_IPPROTO_TCP = 6;
static constexpr int SO_IPPROTO_UDP = 17;

static void *
resourceRequestToHandleData(phys_ptr<ResourceRequest> resourceRequest)
{
   return reinterpret_cast<void *>(
      static_cast<uintptr_t>(
         static_cast<uint32_t>(
            phys_cast<phys_addr>(resourceRequest))));
}

static phys_ptr<ResourceRequest>
handleDataToResourceRequest(void *data)
{
   return phys_cast<ResourceRequest *>(
      static_cast<phys_addr>(
         static_cast<uint32_t>(
            reinterpret_cast<uintptr_t>(data))));
}

std::optional<Error>
SocketDevice::accept(phys_ptr<ResourceRequest> resourceRequest,
                     SocketHandle fd,
                     phys_ptr<SocketAddrIn> sockAddr,
                     int32_t sockAddrLen)
{
   return makeError(ErrorCategory::Socket, SocketError::Inval);
}

std::optional<Error>
SocketDevice::bind(phys_ptr<ResourceRequest> resourceRequest,
                   SocketHandle fd,
                   phys_ptr<SocketAddrIn> sockAddr,
                   int32_t sockAddrLen)
{
   auto socket = getSocket(fd);
   if (!socket) {
      return makeError(ErrorCategory::Socket, SocketError::BadFd);
   }

   auto addr = sockaddr_in { 0 };
   addr.sin_addr.s_addr = sockAddr->sin_addr.s_addr_;
   addr.sin_family = sockAddr->sin_family;
   addr.sin_port = htons(sockAddr->sin_port);

   auto result = 0;
   if (socket->type == Socket::Tcp) {
      result = uv_tcp_bind(reinterpret_cast<uv_tcp_t *>(socket->handle.get()),
                           reinterpret_cast<sockaddr *>(&addr), 0);
   } else if (socket->type == Socket::Udp) {
      result = uv_udp_bind(reinterpret_cast<uv_udp_t *>(socket->handle.get()),
                           reinterpret_cast<sockaddr *>(&addr), 0);
   }

   if (result != 0) {
      return makeError(ErrorCategory::Socket, SocketError::GenericError);
   }

   return Error::OK;
}

std::optional<Error>
SocketDevice::createSocket(phys_ptr<ResourceRequest> resourceRequest,
                           int32_t family,
                           int32_t type,
                           int32_t proto)
{
   auto fd = SocketHandle { -1 };
   for (auto i = 0u; i < mSockets.size(); ++i) {
      if (!mSockets[i].type) {
         fd = i + 1;
         break;
      }
   }

   if (fd == -1) {
      return makeError(ErrorCategory::Socket, SocketError::MFile);
   }

   if (family != SO_AF_INET) {
      return makeError(ErrorCategory::Socket, SocketError::AfNoSupport);
   }

   if (type == SO_SOCK_STREAM) {
      if (proto != SO_IPPROTO_IP && proto != SO_IPPROTO_TCP) {
         return makeError(ErrorCategory::Socket, SocketError::ProtoNoSupport);
      }

      auto handle = std::make_unique<uv_tcp_t>();
      if (uv_tcp_init(networkUvLoop(), handle.get()) != 0) {
         return makeError(ErrorCategory::Socket, SocketError::GenericError);
      }

      auto &socket = mSockets[fd - 1];
      socket.device = this;
      socket.type = Socket::Tcp;
      handle->data = &socket;
      socket.handle.reset(reinterpret_cast<uv_handle_t *>(handle.release()));
   } else if (type == SO_SOCK_DGRAM) {
      if (proto != SO_IPPROTO_IP && proto != SO_IPPROTO_UDP) {
         return makeError(ErrorCategory::Socket, SocketError::ProtoNoSupport);
      }

      auto handle = std::make_unique<uv_udp_t>();
      if (uv_udp_init(networkUvLoop(), handle.get()) != 0) {
         return makeError(ErrorCategory::Socket, SocketError::GenericError);
      }

      auto &socket = mSockets[fd - 1];
      socket.device = this;
      socket.type = Socket::Udp;
      handle->data = &socket;
      socket.handle.reset(reinterpret_cast<uv_handle_t *>(handle.release()));
   } else {
      return makeError(ErrorCategory::Socket, SocketError::ProtoNoSupport);
   }

   return static_cast<Error>(fd);
}

std::optional<Error>
SocketDevice::closeSocket(phys_ptr<ResourceRequest> resourceRequest,
                          SocketHandle fd)
{
   auto socket = getSocket(fd);
   if (!socket) {
      return makeError(ErrorCategory::Socket, SocketError::BadFd);
   }

   // TODO: Do we need to make use of the close callback?
   uv_close(socket->handle.get(), nullptr);
   socket->handle.reset();
   return Error::OK;
}

void
uvReadAllocCallback(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
   buf->base = new char[suggested_size];
   buf->len = static_cast<decltype(buf->len)>(suggested_size);
}

static void
uvReadCallback(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
   auto socket = reinterpret_cast<SocketDevice::Socket *>(stream->data);

   if (nread >= 0) {
      socket->readBuffer.insert(socket->readBuffer.end(), buf->base, buf->base + buf->len);
   } else if (nread < 0) {
      socket->except = makeError(ErrorCategory::Socket, SocketError::GenericError);
   }

   delete buf->base;
   socket->device->checkPendingSelects();
}

static void
uvConnectCallback(uv_connect_t *req, int status)
{
   auto socket = reinterpret_cast<SocketDevice::Socket *>(req->data);

   if (status) {
      if (socket->connectRequest) {
         completeSocketTask(socket->connectRequest,
                            makeError(ErrorCategory::Socket, SocketError::ConnRefused));
      }

      socket->connect.reset();
      return;
   }

   // Immediately start reading data so we can fill our incoming read buffer
   // in order to be able to emulate selects on read fds
   uv_read_start(reinterpret_cast<uv_stream_t *>(socket->handle.get()),
                  uvReadAllocCallback, uvReadCallback);

   if (socket->connectRequest) {
      completeSocketTask(socket->connectRequest, Error::OK);
   }

   socket->connected = true;
   socket->device->checkPendingSelects();
}

std::optional<Error>
SocketDevice::connect(phys_ptr<ResourceRequest> resourceRequest,
                      SocketHandle fd,
                      phys_ptr<SocketAddrIn> sockAddr,
                      int32_t sockAddrLen)
{
   auto socket = getSocket(fd);
   if (!socket) {
      return makeError(ErrorCategory::Socket, SocketError::BadFd);
   }

   if (socket->connect) {
      return makeError(ErrorCategory::Socket, SocketError::IsConn);
   }

   if (socket->type != Socket::Tcp) {
      return makeError(ErrorCategory::Socket, SocketError::Prototype);
   }

   auto addr = sockaddr_in { 0 };
   addr.sin_addr.s_addr = sockAddr->sin_addr.s_addr_;
   addr.sin_family = sockAddr->sin_family;
   addr.sin_port = htons(sockAddr->sin_port);

   socket->connect = std::make_unique<uv_connect_t>();
   socket->connect->data = socket;

   auto error = uv_tcp_connect(socket->connect.get(),
                               reinterpret_cast<uv_tcp_t *>(socket->handle.get()),
                               reinterpret_cast<const sockaddr *>(&addr),
                               &uvConnectCallback);
   if (error) {
      socket->connect.reset();
      return makeError(ErrorCategory::Socket, SocketError::ConnAborted);
   }

   if (socket->nonBlocking) {
      socket->connectRequest = nullptr;
      return makeError(ErrorCategory::Socket, SocketError::InProgress);
   }

   socket->connectRequest = resourceRequest;
   return {};
}

static void
getHostByNameCallback(void *arg, int status, int timeouts, struct hostent *hostent)
{
   auto resourceRequest = handleDataToResourceRequest(arg);
   auto response = phys_cast<SocketDnsQueryResponse *>(resourceRequest->requestData.args.ioctlv.vecs[1].paddr);

   response->hostent.h_name = phys_addrof(response->dnsNames);
   string_copy(response->hostent.h_name.get(),
               hostent->h_name,
               response->dnsNames.size());

   response->hostent.h_addrtype = hostent->h_addrtype;
   response->hostent.h_length = hostent->h_length;

   // From testing on my Wii U it seems no aliases are returned.
   response->aliases[0] = 0u;

   response->ipaddrs = 0u;
   for (auto i = 0u; hostent->h_addr_list[i] && i < response->ipaddrList.size(); ++i) {
      response->ipaddrList[i] = *reinterpret_cast<uint32_t *>(hostent->h_addr_list[i]);
      response->ipaddrs++;
   }

   response->selfPointerOffset = static_cast<uint32_t>(phys_cast<phys_addr>(response));
   completeSocketTask(resourceRequest, Error::OK);
}

std::optional<Error>
SocketDevice::dnsQuery(phys_ptr<ResourceRequest> resourceRequest,
                       phys_ptr<SocketDnsQueryRequest> request,
                       phys_ptr<SocketDnsQueryResponse> response)
{
   if (request->queryType == SocketDnsQueryType::GetHostByName) {
      ares_gethostbyname(ios::internal::networkAresChannel(),
                         phys_addrof(request->name).get(),
                         AF_INET,
                         getHostByNameCallback,
                         resourceRequestToHandleData(resourceRequest));
      return {};
   } else {
      return makeError(ErrorCategory::Socket, SocketError::Inval);
   }
}

std::optional<Error>
SocketDevice::getpeername(phys_ptr<kernel::ResourceRequest> resourceRequest,
                          phys_ptr<const SocketGetPeerNameRequest> request,
                          phys_ptr<SocketGetPeerNameResponse> response)
{
   auto socket = getSocket(request->fd);
   if (!socket) {
      return makeError(ErrorCategory::Socket, SocketError::BadFd);
   }

   if (!socket->connected) {
      return makeError(ErrorCategory::Socket, SocketError::NotConn);
   }

   auto addr = sockaddr_in { };
   auto addrlen = static_cast<int>(sizeof(sockaddr_in));
   auto error = uv_tcp_getpeername(reinterpret_cast<uv_tcp_t *>(socket->handle.get()),
                                   reinterpret_cast<struct sockaddr *>(&addr), &addrlen);
   if (error) {
      return makeError(ErrorCategory::Socket, SocketError::NotConn);
   }

   response->addr.sin_family = addr.sin_family;
   response->addr.sin_port = ntohs(addr.sin_port);
   response->addr.sin_addr.s_addr_ = addr.sin_addr.s_addr;
   response->addrlen = static_cast<int32_t>(sizeof(SocketAddrIn));
   return Error::OK;
}

std::optional<Error>
SocketDevice::setsockopt(phys_ptr<kernel::ResourceRequest> resourceRequest,
                         phys_ptr<const SocketSetSockOptRequest> request,
                         phys_ptr<void> optval,
                         uint32_t optlen)
{
   auto socket = getSocket(request->fd);
   if (!socket) {
      return makeError(ErrorCategory::Socket, SocketError::BadFd);
   }

   if (request->optname == 4118) {
      if (!optval || optlen != sizeof(uint32_t)) {
         return makeError(ErrorCategory::Socket, SocketError::Inval);
      }

      socket->nonBlocking = !!*phys_cast<uint32_t *>(optval);
      return Error::OK;
   }

   netLog->warn("Unimplemented setsockopt for optname {}", request->optname);
   return Error::OK;
}

std::optional<Error>
SocketDevice::checkSelect(phys_ptr<kernel::ResourceRequest> resourceRequest)
{
   auto request = phys_cast<const SocketSelectRequest *>(resourceRequest->requestData.args.ioctl.inputBuffer);
   auto response = phys_cast<SocketSelectResponse *>(resourceRequest->requestData.args.ioctl.outputBuffer);
   auto readyReadFds = uint32_t { 0 };
   auto readyWriteFds = uint32_t { 0 };
   auto readyExceptFds = uint32_t { 0 };
   auto numFds = 0;

   for (auto i = 0; i < request->nfds; ++i) {
      if ((request->readfds >> i) & 1) {
         auto socket = getSocket(i);
         if (!socket) {
            return makeError(ErrorCategory::Socket, SocketError::BadFd);
         }

         // Check if socket is ready to read
         if (!socket->readBuffer.empty()) {
            readyReadFds |= 1u << i;
            ++numFds;
         }
      }
   }

   for (auto i = 0; i < request->nfds; ++i) {
      if ((request->writefds >> i) & 1) {
         auto socket = getSocket(i);
         if (!socket) {
            return makeError(ErrorCategory::Socket, SocketError::BadFd);
         }

         // Our sockets are always ready to write once they have connected
         if (socket->connected) {
            readyWriteFds |= 1u << i;
            ++numFds;
         }
      }
   }

   for (auto i = 0; i < request->nfds; ++i) {
      if ((request->exceptfds >> i) & 1) {
         auto socket = getSocket(i);
         if (!socket) {
            return makeError(ErrorCategory::Socket, SocketError::BadFd);
         }

         // Check if socket has errored
         if (socket->except != Error::OK) {
            response->exceptfds |= 1u << i;
            ++numFds;
         }
      }
   }

   if (numFds) {
      response->readfds = readyReadFds;
      response->writefds = readyWriteFds;
      response->exceptfds = readyExceptFds;
      return static_cast<Error>(numFds);
   }

   return {};
}

void
SocketDevice::checkPendingSelects()
{
   auto now = std::chrono::system_clock::now();
   auto itr = mPendingSelects.begin();

   while (itr != mPendingSelects.end()) {
      auto result = checkSelect(itr->resourceRequest);
      if (!result.has_value() && now >= itr->expireTime) {
         result = makeError(ErrorCategory::Socket, SocketError::TimedOut);
      }

      if (result.has_value()) {
         completeSocketTask(itr->resourceRequest, result);
         itr = mPendingSelects.erase(itr);
      } else {
         ++itr;
      }
   }
}

std::optional<Error>
SocketDevice::select(phys_ptr<ResourceRequest> resourceRequest,
                     phys_ptr<const SocketSelectRequest> request,
                     phys_ptr<SocketSelectResponse> response)
{
   auto result = checkSelect(resourceRequest);
   if (result.has_value()) {
      return result;
   }

   if (request->hasTimeout) {
      auto expireTime = std::chrono::system_clock::now() +
         std::chrono::seconds(request->timeout.tv_sec) +
         std::chrono::microseconds(request->timeout.tv_usec);
      mPendingSelects.push_back({ expireTime, resourceRequest });
      return {};
   }

   return static_cast<Error>(0);
}

#if 0
static void
uvListenConnectionCallback(uv_stream_t *server, int status)
{
   auto socket = reinterpret_cast<SocketDevice::Socket *>(server->data);
}
#endif

std::optional<Error>
SocketDevice::listen(phys_ptr<ResourceRequest> resourceRequest,
                     SocketHandle fd,
                     int32_t backlog)
{
   auto socket = getSocket(fd);
   if (!socket) {
      return makeError(ErrorCategory::Socket, SocketError::BadFd);
   }

   return makeError(ErrorCategory::Socket, SocketError::Inval);
#if 0
   auto error = uv_listen(reinterpret_cast<uv_stream_t *>(socket->handle.get()),
                          backlog, uvListenConnectionCallback);
   if (error != 0) {
   }

   return {};
#endif
}

SocketDevice::Socket *
SocketDevice::getSocket(SocketHandle fd)
{
   if (fd <= 0) {
      return nullptr;
   }

   if (fd > mSockets.size()) {
      return nullptr;
   }

   if (!mSockets[fd - 1].type) {
      return nullptr;
   }

   return &mSockets[fd - 1];
}

} // namespace ios::net::internal
