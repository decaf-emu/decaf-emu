#include "nsysnet.h"
#include "nsysnet_enum.h"
#include "nsysnet_socket_lib.h"

#include "cafe/cafe_ppc_interface_invoke_guest.h"
#include "cafe/cafe_stackobject.h"
#include "cafe/libraries/coreinit/coreinit_ghs.h"
#include "cafe/libraries/coreinit/coreinit_ios.h"
#include "cafe/libraries/coreinit/coreinit_memdefaultheap.h"
#include "cafe/libraries/coreinit/coreinit_mutex.h"
#include "cafe/libraries/coreinit/coreinit_ipcbufpool.h"
#include "ios/net/ios_net_socket.h"
#include "ios/ios_error.h"

#include <charconv>
#include <common/strutils.h>
#include <optional>
#include <string_view>

namespace cafe::nsysnet
{

using namespace coreinit;

using ios::net::SocketAddr;
using ios::net::SocketAddrIn;
using ios::net::SocketCommand;
using ios::net::SocketDnsQueryType;
using ios::net::SocketError;
using ios::net::SocketFamily;

using ios::net::SocketAcceptRequest;
using ios::net::SocketBindRequest;
using ios::net::SocketCloseRequest;
using ios::net::SocketConnectRequest;
using ios::net::SocketDnsQueryRequest;
using ios::net::SocketListenRequest;
using ios::net::SocketRecvRequest;
using ios::net::SocketSetSockOptRequest;
using ios::net::SocketSelectRequest;
using ios::net::SocketSocketRequest;

using ios::net::SocketAcceptResponse;
using ios::net::SocketSelectResponse;

// We do not get this from ios::net because that uses phys_ptr, and here we
// want virt_ptr.
struct SocketDnsQueryResponse
{
   be2_val<uint32_t> unk0x00;
   be2_val<uint32_t> unk0x04;
   be2_val<uint32_t> sendTime;
   be2_val<uint32_t> expireTime;
   be2_val<uint16_t> tries;
   be2_val<uint16_t> lport;
   be2_val<uint16_t> id;
   UNKNOWN(0x2);
   be2_val<uint32_t> unk0x18;
   be2_val<uint32_t> replies;
   be2_val<uint32_t> ipaddrs;
   be2_array<uint32_t, 10> ipaddrList;
   be2_array<virt_ptr<uint32_t>, 10> hostentIpaddrList;
   be2_val<uint32_t> err;
   be2_val<uint32_t> rcode;
   be2_array<char, 256> dnsNames;
   be2_array<char, 129> unk0x17C;
   UNKNOWN(0x27C - 0x1FD);
   be2_val<uint32_t> authsIp;
   be2_array<virt_ptr<char>, 2> aliases;
   UNKNOWN(0x290 - 0x288);
   be2_struct<SocketHostEnt> hostent;
   be2_val<SocketDnsQueryType> queryType;
   be2_array<uint8_t, 2> unk0x2A5;
   UNKNOWN(0x2B0 - 0x2A7);
   be2_val<uint32_t> dnsReq;
   be2_val<uint32_t> next;

   //! Used to adjust pointers in hostent
   be2_val<virt_addr> selfPointerOffset;
};
CHECK_OFFSET(SocketDnsQueryResponse, 0x00, unk0x00);
CHECK_OFFSET(SocketDnsQueryResponse, 0x04, unk0x04);
CHECK_OFFSET(SocketDnsQueryResponse, 0x08, sendTime);
CHECK_OFFSET(SocketDnsQueryResponse, 0x0C, expireTime);
CHECK_OFFSET(SocketDnsQueryResponse, 0x10, tries);
CHECK_OFFSET(SocketDnsQueryResponse, 0x12, lport);
CHECK_OFFSET(SocketDnsQueryResponse, 0x14, id);
CHECK_OFFSET(SocketDnsQueryResponse, 0x18, unk0x18);
CHECK_OFFSET(SocketDnsQueryResponse, 0x1C, replies);
CHECK_OFFSET(SocketDnsQueryResponse, 0x20, ipaddrs);
CHECK_OFFSET(SocketDnsQueryResponse, 0x24, ipaddrList);
CHECK_OFFSET(SocketDnsQueryResponse, 0x4C, hostentIpaddrList);
CHECK_OFFSET(SocketDnsQueryResponse, 0x74, err);
CHECK_OFFSET(SocketDnsQueryResponse, 0x78, rcode);
CHECK_OFFSET(SocketDnsQueryResponse, 0x7C, dnsNames);
CHECK_OFFSET(SocketDnsQueryResponse, 0x17C, unk0x17C);
CHECK_OFFSET(SocketDnsQueryResponse, 0x27C, authsIp);
CHECK_OFFSET(SocketDnsQueryResponse, 0x280, aliases);
CHECK_OFFSET(SocketDnsQueryResponse, 0x290, hostent);
CHECK_OFFSET(SocketDnsQueryResponse, 0x2A4, queryType);
CHECK_OFFSET(SocketDnsQueryResponse, 0x2A5, unk0x2A5);
CHECK_OFFSET(SocketDnsQueryResponse, 0x2B0, dnsReq);
CHECK_OFFSET(SocketDnsQueryResponse, 0x2B4, next);
CHECK_OFFSET(SocketDnsQueryResponse, 0x2B8, selfPointerOffset);
CHECK_SIZE(SocketDnsQueryResponse, 0x2BC);

struct SocketLibData
{
   static constexpr uint32_t MessageCount = 0x20;
   static constexpr uint32_t MessageSize = 0x100;

   SocketLibData()
   {
      OSInitMutex(virt_addrof(lock));
   }

   be2_struct<OSMutex> lock;
   be2_val<IOSHandle> handle = IOSHandle { -1 };
   be2_virt_ptr<IPCBufPool> messagePool;
   be2_array<uint8_t, MessageCount * MessageSize> messageBuffer;
   be2_val<uint32_t> messageCount;
   be2_val<ResolverAlloc> userResolverAlloc;
   be2_val<ResolverFree> userResolverFree;
   be2_val<GetHostError> getHostError;

   be2_struct<SocketHostEnt> getHostByNameHostEnt;
   be2_array<char, 32> getHostByNameName;
   be2_val<uint32_t> getHostByNameIpAddr;
   be2_array<virt_ptr<uint32_t>, 2> getHostByNameAddrList;
   be2_struct<SocketDnsQueryResponse> getHostByNameQuery;
};

static virt_ptr<SocketLibData>
sSocketLibData;

namespace internal
{

static bool
isInitialised();

static virt_ptr<void>
allocateIpcBuffer(uint32_t size);

static void
freeIpcBuffer(virt_ptr<void> buf);

static int32_t
decodeIosError(IOSError err);

static int32_t
performDnsQuery(virt_ptr<const char> name,
                SocketDnsQueryType queryType,
                uint32_t a3, uint32_t a4,
                virt_ptr<SocketDnsQueryResponse> outResponse,
                bool isAsync);

} // namespace internal

int32_t
socket_lib_init()
{
   auto error = 0;
   OSLockMutex(virt_addrof(sSocketLibData->lock));

   if (sSocketLibData->handle < 0) {
      auto iosError = IOS_Open(make_stack_string("/dev/socket"),
                               IOSOpenMode::None);

      if (iosError < 0) {
         sSocketLibData->handle = -1;
         error = SocketError::NoLibRm;
         goto out;
      }

      sSocketLibData->handle = static_cast<IOSHandle>(iosError);
   }

   if (!sSocketLibData->messagePool) {
      sSocketLibData->messagePool =
         IPCBufPoolCreate(virt_addrof(sSocketLibData->messageBuffer),
                          static_cast<uint32_t>(sSocketLibData->messageBuffer.size()),
                          SocketLibData::MessageSize,
                          virt_addrof(sSocketLibData->messageCount),
                          1);

      if (!sSocketLibData->messagePool) {
         error = SocketError::NoMem;
         IOS_Close(sSocketLibData->handle);
         sSocketLibData->handle = -1;
         goto out;
      }
   }

out:
   OSUnlockMutex(virt_addrof(sSocketLibData->lock));
   gh_set_errno(error);
   return error == 0 ? 0 : -1;
}


int32_t
socket_lib_finish()
{
   auto error = 0;
   OSLockMutex(virt_addrof(sSocketLibData->lock));

   if (sSocketLibData->handle >= 0) {
      IOS_Close(sSocketLibData->handle);
      sSocketLibData->handle = -1;
   } else {
      error = SocketError::NoLibRm;
   }

   OSUnlockMutex(virt_addrof(sSocketLibData->lock));
   gh_set_errno(error);
   return error == 0 ? 0 : -1;
}


int32_t
set_resolver_allocator(ResolverAlloc allocFn,
                       ResolverFree freeFn)
{
   if (!allocFn || !freeFn) {
      return -1;
   }

   sSocketLibData->userResolverAlloc = allocFn;
   sSocketLibData->userResolverFree = freeFn;
   return 0;
}


int32_t
accept(int32_t sockfd,
       virt_ptr<SocketAddr> addr,
       virt_ptr<int32_t> addrlen)
{
   if (!internal::isInitialised()) {
      gh_set_errno(SocketError::NotInitialised);
      return -1;
   }

   if (addr && (!addrlen || *addrlen != sizeof(SocketAddrIn))) {
      gh_set_errno(SocketError::Inval);
      return -1;
   }

   auto buf = internal::allocateIpcBuffer(sizeof(SocketAcceptRequest));
   if (!buf) {
      gh_set_errno(SocketError::NoMem);
      return -1;
   }

   auto request = virt_cast<SocketAcceptRequest *>(buf);
   request->fd = sockfd;

   if (addrlen) {
      request->addrlen = *addrlen;
   } else {
      request->addrlen = static_cast<int32_t>(sizeof(SocketAddrIn));
   }

   auto error = IOS_Ioctl(sSocketLibData->handle,
                          SocketCommand::Accept,
                          request,
                          sizeof(SocketAcceptRequest),
                          request,
                          sizeof(SocketAcceptResponse));
   if (error >= IOSError::OK) {
      auto response = virt_cast<SocketAcceptRequest *>(buf);
      if (addr) {
         std::memcpy(addr.get(), std::addressof(response->addr),
                     response->addrlen);
         *addrlen = response->addrlen;
      }
   }

   auto result = internal::decodeIosError(error);

   internal::freeIpcBuffer(buf);
   return result;
}


int32_t
bind(int32_t sockfd,
     virt_ptr<SocketAddr> addr,
     int32_t addrlen)
{
   if (!internal::isInitialised()) {
      gh_set_errno(SocketError::NotInitialised);
      return -1;
   }

   if (!addr || addr->sa_family != SocketFamily::Inet || addrlen != sizeof(SocketAddrIn)) {
      gh_set_errno(SocketError::Inval);
      return -1;
   }

   auto buf = internal::allocateIpcBuffer(sizeof(SocketBindRequest));
   if (!buf) {
      gh_set_errno(SocketError::NoMem);
      return -1;
   }

   auto request = virt_cast<SocketBindRequest *>(buf);
   request->fd = sockfd;
   request->addr = *virt_cast<SocketAddrIn *>(addr);
   request->addrlen = addrlen;

   auto error = IOS_Ioctl(sSocketLibData->handle,
                                    SocketCommand::Bind,
                                    request,
                                    sizeof(SocketBindRequest),
                                    NULL,
                                    0);

   auto result = internal::decodeIosError(error);

   internal::freeIpcBuffer(buf);
   return result;
}


int32_t
connect(int32_t sockfd,
        virt_ptr<SocketAddr> addr,
        int32_t addrlen)
{
   if (!internal::isInitialised()) {
      gh_set_errno(SocketError::NotInitialised);
      return -1;
   }

   if (!addr || addr->sa_family != SocketFamily::Inet || addrlen != sizeof(SocketAddrIn)) {
      gh_set_errno(SocketError::Inval);
      return -1;
   }

   // TODO: if set_multicast_state(TRUE)

   auto buf = internal::allocateIpcBuffer(sizeof(SocketConnectRequest));
   if (!buf) {
      gh_set_errno(SocketError::NoMem);
      return -1;
   }

   auto request = virt_cast<SocketConnectRequest *>(buf);
   request->fd = sockfd;
   request->addr = *virt_cast<SocketAddrIn *>(addr);
   request->addrlen = addrlen;

   auto error = IOS_Ioctl(sSocketLibData->handle,
                          SocketCommand::Connect,
                          request,
                          sizeof(SocketConnectRequest),
                          NULL,
                          0);

   auto result = internal::decodeIosError(error);

   internal::freeIpcBuffer(buf);
   return result;
}


/**
 * Parse IP address from a string.
 *
 * Similar to inet_aton but different:
 * - Requires 1-3 dots
 * - Only supports 0-255 for each number
 *
 * Which means unlike inet_aton:
 * - a       = invalid
 * - a.b     = a.0.0.b
 * - a.b.c   = a.0.b.c
 * - a.b.c.d = a.b.c.d
 */
static std::optional<uint32_t>
parseIpAddress(const char *host)
{
   if (!host) {
      return {};
   }

   auto len = strnlen(host, 256);
   if (!len || len == 256) {
      return {};
   }

   auto sv = std::string_view { host, len };

   // Count dots and ensure no invalid characters for an IP address
   auto numDots = 0;
   for (auto c : sv) {
      if (c == '.') {
         numDots++;
      } else if (c < '0' || c > '9') {
         return {};
      }
   }

   if (numDots < 1 || numDots > 3) {
      return {};
   }

   auto start = std::string_view::size_type { 0 };
   auto ip = std::array<uint8_t, 4> { 0, 0, 0, 0 };

   for (auto idx = 0; idx < numDots + 1; ++idx) {
      auto component = 0;
      auto end = std::min(sv.find_first_of('.', start), sv.size());
      auto result = std::from_chars(sv.data() + start, sv.data() + end, component);
      if (result.ec != std::errc() || component < 0 || component > 255) {
         return {};
      }

      if (idx == 0) {
         ip[idx] = static_cast<uint8_t>(component);
      } else {
         ip[idx + (3 - numDots)] = static_cast<uint8_t>(component);
      }

      start = end + 1;
   }

   auto output = uint32_t { 0 };
   std::memcpy(&output, ip.data(), 4);
   return output;
}

virt_ptr<GetHostError>
get_h_errno()
{
   return virt_addrof(sSocketLibData->getHostError);
}

virt_ptr<SocketHostEnt>
gethostbyname(virt_ptr<const char> name)
{
   if (!name || !name[0]) {
      sSocketLibData->getHostError = GetHostError::NoRecovery;
      return nullptr;
   }

   // If the host name is just an ip address we can return immediately.
   if (auto ipaddr = parseIpAddress(name.get()); ipaddr.has_value()) {
      auto result = virt_addrof(sSocketLibData->getHostByNameHostEnt);
      result->h_aliases = nullptr;
      result->h_length = 4;
      result->h_addrtype = 2;

      result->h_name = virt_addrof(sSocketLibData->getHostByNameName);
      string_copy(result->h_name.get(), name.get(), 32);

      result->h_addr_list = virt_addrof(sSocketLibData->getHostByNameAddrList);
      result->h_addr_list[0] = virt_addrof(sSocketLibData->getHostByNameIpAddr);
      result->h_addr_list[1] = nullptr;
      sSocketLibData->getHostByNameIpAddr = ipaddr.value();
      return result;
   }

   auto error = internal::performDnsQuery(
      name, SocketDnsQueryType::GetHostByName, 0u, 0u,
      virt_addrof(sSocketLibData->getHostByNameQuery), false);
   if (error < 0 || !sSocketLibData->getHostByNameQuery.ipaddrs) {
      sSocketLibData->getHostError = GetHostError::HostNotFound;
      return nullptr;
   }

   // Update ip address list
   sSocketLibData->getHostByNameQuery.hostent.h_addrtype = 2;
   sSocketLibData->getHostByNameQuery.hostent.h_length = 4;
   sSocketLibData->getHostByNameQuery.hostent.h_addr_list =
      virt_addrof(sSocketLibData->getHostByNameQuery.hostentIpaddrList);
   for (auto i = 0u; i < sSocketLibData->getHostByNameQuery.ipaddrs; ++i) {
      sSocketLibData->getHostByNameQuery.hostentIpaddrList[i] =
         virt_addrof(sSocketLibData->getHostByNameQuery.ipaddrList[i]);
   }

   sSocketLibData->getHostByNameQuery
      .hostentIpaddrList[sSocketLibData->getHostByNameQuery.ipaddrs] = nullptr;
   sSocketLibData->getHostError = GetHostError::OK;
   return virt_addrof(sSocketLibData->getHostByNameQuery.hostent);
}

int32_t
listen(int32_t sockfd,
       int32_t backlog)
{
   if (!internal::isInitialised()) {
      gh_set_errno(SocketError::NotInitialised);
      return -1;
   }

   if (backlog < 0) {
      gh_set_errno(SocketError::Inval);
      return -1;
   }

   auto buf = internal::allocateIpcBuffer(sizeof(SocketListenRequest));
   if (!buf) {
      gh_set_errno(SocketError::NoMem);
      return -1;
   }

   auto request = virt_cast<SocketListenRequest *>(buf);
   request->fd = sockfd;
   request->backlog = backlog;

   auto error = IOS_Ioctl(sSocketLibData->handle,
                          SocketCommand::Listen,
                          request,
                          sizeof(SocketListenRequest),
                          NULL,
                          0);

   auto result = internal::decodeIosError(error);

   internal::freeIpcBuffer(buf);
   return result;
}

static void
prepareUnalignedBuffer(virt_ptr<uint8_t> buffer,
                       int32_t len,
                       virt_ptr<uint8_t> alignedBeforeBuffer,
                       virt_ptr<uint8_t> alignedAfterBuffer,
                       virt_ptr<IOSVec> alignedBeforeVec,
                       virt_ptr<IOSVec> alignedMiddleVec,
                       virt_ptr<IOSVec> alignedAfterVec,
                       bool input)
{
   auto bufferAlignedStart = align_up(buffer, IOSVecAlign);
   auto bufferAlignedEnd = align_down(buffer + len, IOSVecAlign);
   auto bufferEnd = buffer + len;

   if (bufferAlignedStart != buffer) {
      alignedBeforeVec->vaddr = virt_cast<virt_addr>(alignedBeforeBuffer);
      alignedBeforeVec->len = static_cast<uint32_t>(bufferAlignedStart - buffer);

      if (input) {
         std::memcpy(alignedBeforeBuffer.get(),
                     buffer.get(),
                     alignedBeforeVec->len);
      }
   }

   alignedMiddleVec->vaddr = virt_cast<virt_addr>(bufferAlignedStart);
   alignedMiddleVec->len = static_cast<uint32_t>(bufferEnd - bufferAlignedStart);

   if (bufferAlignedEnd != bufferEnd) {
      alignedAfterVec->vaddr = virt_cast<virt_addr>(alignedAfterBuffer);
      alignedAfterVec->len = static_cast<uint32_t>(bufferEnd - bufferAlignedEnd);

      if (input) {
         std::memcpy(alignedAfterBuffer.get(),
                     bufferAlignedEnd.get(),
                     alignedAfterVec->len);
      }
   }
}

static void
parseUnalignedBuffer(virt_ptr<uint8_t> buffer,
                     int32_t len,
                     virt_ptr<IOSVec> alignedBeforeVec,
                     virt_ptr<IOSVec> alignedMiddleVec,
                     virt_ptr<IOSVec> alignedAfterVec)
{
   if (alignedBeforeVec->len) {
      std::memcpy(buffer.get(),
                  virt_cast<void *>(alignedBeforeVec->vaddr).get(),
                  alignedBeforeVec->len);
   }

   if (alignedAfterVec->len) {
      auto offset = alignedBeforeVec->len + alignedMiddleVec->len;
      std::memcpy(buffer.get() + offset,
                  virt_cast<void *>(alignedAfterVec->vaddr).get(),
                  alignedAfterVec->len);
   }
}

int32_t
recv(int32_t sockfd,
     virt_ptr<void> buffer,
     int32_t len,
     int32_t flags)
{
   if (!internal::isInitialised()) {
      gh_set_errno(SocketError::NotInitialised);
      return -1;
   }

   if (!buffer || len < 0) {
      gh_set_errno(SocketError::Inval);
      return -1;
   }

   auto buf = internal::allocateIpcBuffer(0x88);
   if (!buf) {
      gh_set_errno(SocketError::NoMem);
      return -1;
   }

   auto request = virt_cast<SocketRecvRequest *>(virt_cast<char *>(buf) + 0x80);
   request->fd = sockfd;
   request->flags = flags;

   auto vecs = virt_cast<IOSVec *>(buf);
   vecs[0].vaddr = virt_cast<virt_addr>(request);
   vecs[0].len = static_cast<uint32_t>(sizeof(SocketRecvRequest));

   vecs[1].vaddr = virt_addr { 0 };
   vecs[1].len = 0u;

   vecs[2].vaddr = virt_addr { 0 };
   vecs[2].len = 0u;

   vecs[3].vaddr = virt_addr{ 0 };
   vecs[3].len = 0u;

   if (align_check(buffer.get(), IOSVecAlign) && align_check(len, IOSVecAlign)) {
      vecs[1].vaddr = virt_cast<virt_addr>(buffer);
      vecs[1].len = static_cast<uint32_t>(len);
   } else {
      StackArray<uint8_t, IOSVecAlign * 3> alignedBuffers;
      auto alignedBeforeBuffer = align_up(virt_ptr<uint8_t> { alignedBuffers }, IOSVecAlign);
      auto alignedAfterBuffer = alignedBeforeBuffer + IOSVecAlign;

      prepareUnalignedBuffer(virt_cast<uint8_t *>(buffer), len,
                             alignedBeforeBuffer, alignedAfterBuffer,
                             virt_addrof(vecs[1]), virt_addrof(vecs[2]),
                             virt_addrof(vecs[3]),
                             false);
   }

   auto error = IOS_Ioctlv(sSocketLibData->handle,
                           SocketCommand::Recv,
                           1, 3, vecs);
   if (error >= IOSError::OK) {
      parseUnalignedBuffer(virt_cast<uint8_t *>(buffer), len,
                           virt_addrof(vecs[1]), virt_addrof(vecs[2]),
                           virt_addrof(vecs[3]));
   }

   auto result = internal::decodeIosError(error);

   internal::freeIpcBuffer(buf);
   return result;
}


int32_t
setsockopt(int32_t sockfd,
           int32_t level,
           int32_t optname,
           virt_ptr<void> optval,
           int32_t optlen)
{
   StackArray<uint8_t, 0x40, IOSVecAlign> optvalBuffer;

   if (!internal::isInitialised()) {
      gh_set_errno(SocketError::NotInitialised);
      return -1;
   }

   if (level != -1 && level != 0 && level != 6) {
      gh_set_errno(SocketError::Inval);
      return -1;
   }

   if (optlen < 0 || static_cast<unsigned>(optlen) > optvalBuffer.size()) {
      gh_set_errno(SocketError::Inval);
      return -1;
   }

   if (!optval && optlen > 0) {
      gh_set_errno(SocketError::Inval);
      return -1;
   }

   auto buf = internal::allocateIpcBuffer(0x24);
   if (!buf) {
      gh_set_errno(SocketError::NoMem);
      return -1;
   }

   // Yes they really do overlap IOSVec and SetOptRequest... :)
   auto request = virt_cast<SocketSetSockOptRequest *>(buf);
   request->fd = sockfd;
   request->level = level;
   request->optname = optname;

   auto vecs = virt_cast<IOSVec *>(buf);
   vecs[0].vaddr = virt_cast<virt_addr>(optvalBuffer);
   vecs[0].len = static_cast<uint32_t>(optlen);
   std::memcpy(optvalBuffer.get(), optval.get(), optlen);

   vecs[1].vaddr = virt_cast<virt_addr>(request);
   vecs[1].len = static_cast<uint32_t>(sizeof(SocketSetSockOptRequest));

   auto error = IOS_Ioctlv(sSocketLibData->handle,
                           SocketCommand::SetSockOpt,
                           2, 0, vecs);
   auto result = internal::decodeIosError(error);

   internal::freeIpcBuffer(buf);
   return result;
}


int32_t
select(int32_t nfds,
       virt_ptr<SocketFdSet> readfds,
       virt_ptr<SocketFdSet> writefds,
       virt_ptr<SocketFdSet> exceptfds,
       virt_ptr<SocketTimeval> timeout)
{
   if (!internal::isInitialised()) {
      gh_set_errno(SocketError::NotInitialised);
      return -1;
   }

   if (nfds < 0) {
      gh_set_errno(SocketError::NotInitialised);
      return -1;
   }

   if (timeout) {
      if (timeout->tv_sec < 0 || timeout->tv_usec < 0 ||
         timeout->tv_sec > 1000000 || timeout->tv_usec > 1000000) {
         gh_set_errno(SocketError::Inval);
         return -1;
      }
   }

   if ((!readfds || !*readfds) && (!writefds || !*writefds) && (!exceptfds || !*exceptfds)) {
      if (!timeout) {
         gh_set_errno(SocketError::Inval);
         return -1;
      } else if (timeout->tv_sec == 0 && timeout->tv_usec == 0) {
         // No fds and timeout is 0 = success
         return 0;
      }
   }

   auto buf = internal::allocateIpcBuffer(sizeof(SocketSelectRequest));
   if (!buf) {
      gh_set_errno(SocketError::NoMem);
      return -1;
   }

   auto response = virt_cast<SocketSelectResponse *>(buf);
   auto request = virt_cast<SocketSelectRequest *>(buf);
   request->nfds = nfds;
   request->readfds = 0u;
   request->writefds = 0u;
   request->exceptfds = 0u;
   request->hasTimeout = 0;

   if (readfds) {
      request->readfds = *readfds;
   }

   if (writefds) {
      request->writefds = *writefds;
   }

   if (exceptfds) {
      request->exceptfds = *exceptfds;
   }

   if (timeout) {
      request->timeout = *timeout;
      request->hasTimeout = 1;
   }

   auto result = 0;
   auto error = IOS_Ioctl(sSocketLibData->handle,
                          SocketCommand::Select,
                          request,
                          sizeof(SocketSelectRequest),
                          response,
                          sizeof(SocketSelectResponse));

   result = internal::decodeIosError(error);
   if (result >= 0) {
      if (readfds) {
         *readfds = response->readfds;
      }

      if (writefds) {
         *writefds = response->writefds;
      }

      if (exceptfds) {
         *exceptfds = response->exceptfds;
      }
   }

   internal::freeIpcBuffer(buf);
   return result;
}


int32_t
socket(int32_t family,
       int32_t type,
       int32_t proto)
{
   if (!internal::isInitialised()) {
      gh_set_errno(SocketError::NotInitialised);
      return -1;
   }

   auto buf = internal::allocateIpcBuffer(sizeof(SocketSocketRequest));
   if (!buf) {
      gh_set_errno(SocketError::NoMem);
      return -1;
   }

   auto request = virt_cast<SocketSocketRequest *>(buf);
   request->family = family;
   request->type = type;
   request->proto = proto;

   auto result = 0;
   auto error = IOS_Ioctl(sSocketLibData->handle,
                          SocketCommand::Socket,
                          request,
                          sizeof(SocketSocketRequest),
                          NULL,
                          0);

   if (ios::getErrorCategory(error) == ios::ErrorCategory::Socket
    && ios::getErrorCode(error) == SocketError::GenericError) {
      // Map generic socket error to no memory.
      gh_set_errno(SocketError::NoMem);
      result = -1;
   } else {
      result = internal::decodeIosError(error);
   }

   internal::freeIpcBuffer(buf);
   return result;
}


int32_t
socketclose(int32_t sockfd)
{
   if (!internal::isInitialised()) {
      gh_set_errno(SocketError::NotInitialised);
      return -1;
   }

   auto buf = internal::allocateIpcBuffer(sizeof(SocketCloseRequest));
   if (!buf) {
      gh_set_errno(SocketError::NoMem);
      return -1;
   }

   auto request = virt_cast<SocketCloseRequest *>(buf);
   request->fd = sockfd;

   auto error = IOS_Ioctl(sSocketLibData->handle,
                          SocketCommand::Close,
                          request,
                          sizeof(SocketCloseRequest),
                          NULL,
                          0);

   auto result = internal::decodeIosError(error);
   internal::freeIpcBuffer(buf);
   return result;
}

int32_t
socketlasterr()
{
   return gh_get_errno();
}

namespace internal
{

static bool
isInitialised()
{
   auto initialised = true;
   OSLockMutex(virt_addrof(sSocketLibData->lock));

   if (sSocketLibData->handle < 0 || !sSocketLibData->messagePool) {
      initialised = false;
   }

   OSUnlockMutex(virt_addrof(sSocketLibData->lock));
   return initialised;
}

static virt_ptr<void>
allocateIpcBuffer(uint32_t size)
{
   auto buf = IPCBufPoolAllocate(sSocketLibData->messagePool, size);
   if (buf) {
      std::memset(buf.get(), 0, size);
   }

   return buf;
}

static void
freeIpcBuffer(virt_ptr<void> buf)
{
   IPCBufPoolFree(sSocketLibData->messagePool, buf);
}

static int32_t
decodeIosError(IOSError err)
{
   if (err >= 0) {
      gh_set_errno(0);
      return err;
   }

   auto category = ios::getErrorCategory(err);
   auto code = ios::getErrorCode(err);
   auto error = SocketError::Unknown;

   switch (category) {
   case ios::ErrorCategory::Socket:
      error = static_cast<SocketError>(code);
      break;
   case ios::ErrorCategory::Kernel:
      if (code == ios::Error::Access) {
         error = SocketError::Inval;
      } else if (code == ios::Error::Intr) {
         error = SocketError::Aborted;
      } else if (code == ios::Error::QFull) {
         error = SocketError::Busy;
      } else {
         error = SocketError::Unknown;
      }
      break;
   default:
      error = SocketError::Unknown;
   }

   gh_set_errno(error);
   return -1;
}

int32_t
performDnsQuery(virt_ptr<const char> name,
                SocketDnsQueryType queryType,
                uint32_t a3,
                uint32_t a4,
                virt_ptr<SocketDnsQueryResponse> outResponse,
                bool isAsync)
{
   auto size = 1152u;
   auto buffer = virt_ptr<void> { nullptr };
   if (sSocketLibData->userResolverAlloc) {
      buffer = cafe::invoke(cpu::this_core::state(),
                            sSocketLibData->userResolverAlloc,
                            size + (IOSVecAlign - 1));
      buffer = align_up(buffer, IOSVecAlign);
   } else {
      buffer = MEMAllocFromDefaultHeapEx(size, IOSVecAlign);
   }

   if (!buffer) {
      gh_set_errno(SocketError::NoMem);
      return -1;
   }

   std::memset(buffer.get(), 0, size);

   auto request = virt_cast<SocketDnsQueryRequest *>(virt_cast<char *>(buffer) + 0x80);
   string_copy(virt_addrof(request->name).get(), name.get(), request->name.size());
   request->queryType = queryType;
   request->unk0x88 = a3;
   request->unk0x8C = a4;
   request->isAsync = isAsync ? uint8_t { 1 } : uint8_t { 0 };

   auto response = virt_cast<SocketDnsQueryResponse *>(virt_cast<char *>(buffer) + 0x180);

   auto vecs = virt_cast<IOSVec *>(buffer);
   vecs[0].vaddr = virt_cast<virt_addr>(request);
   vecs[0].len = static_cast<uint32_t>(sizeof(SocketDnsQueryRequest));

   vecs[1].vaddr = virt_cast<virt_addr>(response);
   vecs[1].len = static_cast<uint32_t>(sizeof(SocketDnsQueryResponse));

   auto error = IOS_Ioctlv(sSocketLibData->handle,
                           SocketCommand::DnsQuery,
                           1, 1, vecs);
   std::memcpy(outResponse.get(), response.get(), sizeof(SocketDnsQueryResponse));

   // Repair hostent pointers
   outResponse->hostent.h_aliases = virt_addrof(outResponse->aliases);

   for (auto i = 0u; i < outResponse->aliases.size(); ++i) {
      if (outResponse->aliases[i]) {
         outResponse->aliases[i] = virt_cast<char *>(
            virt_cast<virt_addr>(outResponse) +
            (virt_cast<virt_addr>(outResponse->aliases[i]) - outResponse->selfPointerOffset));
      }
   }

   outResponse->hostent.h_name = virt_cast<char *>(
      virt_cast<virt_addr>(outResponse) +
      (virt_cast<virt_addr>(outResponse->hostent.h_name) - outResponse->selfPointerOffset));

   if (sSocketLibData->userResolverFree) {
      cafe::invoke(cpu::this_core::state(),
                   sSocketLibData->userResolverFree,
                   buffer);
   } else {
      MEMFreeToDefaultHeap(buffer);
   }

   return error;
}

} // namespace internal

void
Library::registerSocketLibSymbols()
{
   RegisterFunctionExport(socket_lib_init);
   RegisterFunctionExport(socket_lib_finish);
   RegisterFunctionExport(accept);
   RegisterFunctionExport(bind);
   RegisterFunctionExport(connect);
   RegisterFunctionExport(get_h_errno);
   RegisterFunctionExport(gethostbyname);
   RegisterFunctionExport(listen);
   RegisterFunctionExport(recv);
   RegisterFunctionExport(set_resolver_allocator);
   RegisterFunctionExport(setsockopt);
   RegisterFunctionExport(select);
   RegisterFunctionExport(socket);
   RegisterFunctionExport(socketclose);
   RegisterFunctionExport(socketlasterr);

   RegisterDataInternal(sSocketLibData);
}

} // namespace cafe::nsysnet
