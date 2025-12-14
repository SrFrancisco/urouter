#include "socket.h"
#include "message.h"
#include <net/ethernet.h>
#include <type_traits>

RawSocket::~RawSocket() { _cleanup(); }

RawSocket::RawSocket(std::string network_iface) : iface_mame(network_iface) {
  // Initialize dest struct
  // see packet(7) man
  
  static_assert(   std::is_trivially_default_constructible_v<struct sockaddr_ll> 
                && std::is_trivially_destructible_v<struct sockaddr_ll>, 
                     "struct needs to have implicit lifetime");
  static_assert(   std::is_trivially_default_constructible_v<struct sockaddr> 
                && std::is_trivially_destructible_v<struct sockaddr>, 
                     "struct needs to have implicit lifetime");

  memset(&socket_addr, 0, sizeof(struct sockaddr_ll));
  // To send packets: specify sll_family, sll_addr, sll_halen, sll_ifindex, and
  // sll_protocol
  socket_addr.sll_family = AF_PACKET;
  socket_addr.sll_protocol =
      htons(ETH_P_ALL); // get all packets (incl. IP and ARP)
  socket_addr.sll_ifindex = if_nametoindex(network_iface.c_str());
  assert(socket_addr.sll_ifindex != 0); // could not find interface

  socketfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  assert(socketfd != -1);

  int rc = bind(socketfd, (struct sockaddr *)&socket_addr, sizeof(socket_addr));
  if (rc != 0) {
    std::cerr << "Could not bind socket!" << std::endl;
    close(socketfd);
    exit(1);
  }
}

void RawSocket::blockingSend(void *buf, size_t buf_size) {
  std::cout << "==> Sending msg" << std::endl;
  assert(buf != nullptr);
  assert(socketfd > 0);
  int rc = send(socketfd, buf, buf_size, 0);
  if (rc == -1) {
    std::cerr << "Err in send with errno " << errno << std::endl;
    _cleanup();
  }
  return;
}

void RawSocket::blockingRecv(void *buf, size_t buf_size) {
  int rc = ::recv(socketfd, buf, buf_size, 0);
  if (rc == -1) {
    std::cerr << "Err in recv with errno " << errno << std::endl;
    _cleanup();
  }
}

size_t generate_ether_header(char *buffer, size_t buf_size, uint16_t protocol,
                             MAC_addr &src, MAC_addr &dest) {
  
  static_assert(   std::is_trivially_default_constructible_v<struct ether_header> 
                && std::is_trivially_destructible_v<struct ether_header>, 
                     "struct needs to have implicit lifetime");
                  
  assert(buf_size >= sizeof(struct ether_header));
  ASSERT_ALIGNMENT(buffer, struct ether_header);

  memset(buffer, 0, sizeof(struct ether_header));
  struct ether_header *header = std::launder(reinterpret_cast<struct ether_header *>(buffer));

  header->ether_type = htons(protocol);
  std::copy(std::begin(src.addr), std::end(src.addr), header->ether_shost);
  std::copy(std::begin(dest.addr), std::end(dest.addr), header->ether_dhost);

  return sizeof(struct ether_header);
}