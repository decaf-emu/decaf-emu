#pragma once
#include "ios/net/ios_net_socket.h"
#include "nsysnet_enum.h"

#include <cstdint>
#include <libcpu/be2_struct.h>

namespace cafe::nsysnet
{

using ios::net::SocketAddr;
using ios::net::SocketFdSet;
using ios::net::SocketTimeval;

struct SocketHostEnt
{
   be2_virt_ptr<char> h_name;
   be2_virt_ptr<virt_ptr<char>> h_aliases;
   be2_val<int32_t> h_addrtype;
   be2_val<int32_t> h_length;
   be2_virt_ptr<virt_ptr<uint32_t>> h_addr_list;
};
CHECK_OFFSET(SocketHostEnt, 0x00, h_name);
CHECK_OFFSET(SocketHostEnt, 0x04, h_aliases);
CHECK_OFFSET(SocketHostEnt, 0x08, h_addrtype);
CHECK_OFFSET(SocketHostEnt, 0x0C, h_length);
CHECK_OFFSET(SocketHostEnt, 0x10, h_addr_list);
CHECK_SIZE(SocketHostEnt, 0x14);

using ResolverAlloc = virt_func_ptr<virt_ptr<void>(uint32_t)>;
using ResolverFree = virt_func_ptr<void(virt_ptr<void>)>;

int32_t
socket_lib_init();

int32_t
socket_lib_finish();

int32_t
set_resolver_allocator(ResolverAlloc allocFn,
                       ResolverFree freeFn);

int32_t
accept(int32_t sockfd,
       virt_ptr<SocketAddr> addr,
       virt_ptr<int32_t> addrlen);

int32_t
bind(int32_t sockfd,
     virt_ptr<SocketAddr> addr,
     int32_t addrlen);

int32_t
connect(int32_t sockfd,
        virt_ptr<SocketAddr> addr,
        int32_t addrlen);

virt_ptr<GetHostError>
get_h_errno();

virt_ptr<SocketHostEnt>
gethostbyname(virt_ptr<const char> name);

int32_t
getpeername(int32_t sockfd,
            virt_ptr<SocketAddr> addr,
            virt_ptr<uint32_t> addrlen);

int32_t
listen(int32_t sockfd,
       int32_t backlog);

int32_t
recv(int32_t sockfd,
     virt_ptr<void> buffer,
     int32_t len,
     int32_t flags);

int32_t
setsockopt(int32_t sockfd,
           int32_t level,
           int32_t optname,
           virt_ptr<void> optval,
           int32_t optlen);

int32_t
select(int32_t nfds,
       virt_ptr<SocketFdSet> readfds,
       virt_ptr<SocketFdSet> writefds,
       virt_ptr<SocketFdSet> exceptfds,
       virt_ptr<SocketTimeval> timeout);

int32_t
socket(int32_t family,
       int32_t type,
       int32_t proto);

int32_t
socketclose(int32_t sockfd);

} // namespace cafe::nsysnet
