#pragma once

#include "message.h"
#include <assert.h>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <string>
// #include <linux/ip.h>
#include <memory>
#include <new>
#include <sys/socket.h>
#include <sys/types.h>
// #include <linux/if_ether.h>
#include <array>
#include <linux/if_packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h> // see: https://stackoverflow.com/questions/42840636/difference-between-struct-ip-and-struct-iphdr
#include <string>
#include <sys/socket.h>
#include <type_traits>
#include <unistd.h>
#include <unordered_map>

// based on the code in
// https://ventspace.wordpress.com/2022/04/11/quick-snippet-c-type-trait-templates-for-lambda-details/
template <typename Tuple> struct first_arg;
template <typename First, typename... Rest>
struct first_arg<std::tuple<First, Rest...>> {
  using type = First;
};
template <typename T> struct fun_type {
  using type = void;
};
template <typename Ret, typename Class, typename... Args>
struct fun_type<Ret (Class::*)(Args...) const> {
  using arg_types = std::tuple<Args...>;
  using first_arg = typename first_arg<arg_types>::type;
};
template <typename F> using function_info = fun_type<decltype(&F::operator())>;

template <typename T> std::true_type IsL2TImpl(const L2Message<T> *);
std::false_type IsL2TImpl(const void *);
template <typename T>
using IsL2T = decltype(IsL2TImpl(std::declval<std::remove_cvref_t<T> *>()));

static constexpr unsigned long MAX_BUFFER = 1 << 13;

enum ETH_OP_TYPE : uint16_t {
  IPv4 = 0x0800,
  ARP = 0x0806,
};

struct MAC_addr {
  std::array<uint8_t, 6> addr = {};

  MAC_addr() = default;
  static inline MAC_addr from_c_struct(uint8_t data[6]) {
    MAC_addr mac;
    memcpy(mac.addr.data(), data, 6);

    return mac;
  }

  std::string toString() const {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(2) << (int)addr[0] << ":"
        << std::setw(2) << (int)addr[1] << ":" << std::setw(2) << (int)addr[2]
        << ":" << std::setw(2) << (int)addr[3] << ":" << std::setw(2)
        << (int)addr[4] << ":" << std::setw(2) << (int)addr[5];
    return oss.str();
  }

};

bool inline operator==(const MAC_addr& lhs, const MAC_addr& rhs) {
  return lhs.addr == rhs.addr;
}

template<> struct std::hash<MAC_addr> {
  std::size_t operator()(MAC_addr const& s) const noexcept {
      uint64_t value = 0;
      std::memcpy(&value, s.addr.data(), 6);
      return std::hash<uint64_t>{}(value);
  }
};

size_t generate_ether_header(char *buffer, size_t buf_size, uint16_t protocol,
                             MAC_addr &src, MAC_addr &dest);

struct RawSocket {
  struct sockaddr_ll socket_addr;
  int socketfd;
  MAC_addr mac;
  std::string iface_mame;

  RawSocket(std::string network_iface);

  ~RawSocket();

  void blockingRecv(void *buf, size_t buf_size);
  ssize_t recv(void* buf, size_t buf_size);

  /**
   * WARN! This function will not verify the structure of the payload. Make
   *       sure it is a correct L2 payload with Ethernet Header + all above
   */
  void blockingSend(void *buf, size_t buf_size);

  template <typename Callback, typename... Callbacks>
    requires std::invocable<Callback,
                            typename function_info<Callback>::first_arg> &&
             IsL2T<typename function_info<Callback>::first_arg>::value
  void apply(void *buffer, size_t len, Callback function,
             Callbacks... callbacks) {
    struct ether_header *header = static_cast<struct ether_header *>(buffer);
    uint16_t type = ntohs(header->ether_type);
    using MsgType = // we have ref types
        std::remove_reference_t<typename function_info<Callback>::first_arg>;
    if (MsgType::ETHER_TYPE == type) {

      // this requires the struct to have an implicit lifetime
      // https://en.cppreference.com/w/cpp/language/classes.html#Implicit-lifetime_class
      // https://stackoverflow.com/a/78198111
      static_assert(    std::is_trivially_default_constructible_v<MsgType> 
                     && std::is_trivially_destructible_v<MsgType>, 
                     "struct needs to have implicit lifetime");

      auto &message = *std::launder(reinterpret_cast<MsgType *>(
          static_cast<uint8_t *>(buffer) + sizeof(ether_header)));
      function(message);

    } else {
      apply(buffer, len, callbacks...);
    }
  }

  inline void apply(void *buffer, [[maybe_unused]] size_t len) {
    std::cout << "No action for message type "
              << ntohs(static_cast<struct ether_header *>(buffer)->ether_type)
              << std::endl;
  }

  template <typename... Args>
  void blockingRecvApply(void *buf, size_t buf_size, Args... callbacks) {
    blockingRecv(buf, buf_size);
    apply(buf, buf_size, callbacks...);
  }

private:
  inline void _cleanup() {
    assert(socketfd != -1);
    close(socketfd);
    socketfd = -1; // just in case
  }
};


class SocketManager {
private:
  std::unordered_map<std::string,RawSocket> _sockets{};
public:
  SocketManager() = default;
  void addConnection(std::string network_iface);
  
  template <typename... Args>
  void recvApplyAll(void *buf, size_t buf_size, Args... callbacks) {
    ssize_t rc;

    //TODO: may want to use select/poll or have one thread for each iface
    for(auto& [ifname,sock] : _sockets)
    {
      rc = sock.recv(buf, buf_size);
      if(rc != -1)
        sock.apply(buf, buf_size, callbacks...);
    }  
  }

  void L2recvApplyAll(void* buf, size_t buf_size,
                      std::function<void(struct ether_header header, std::string iface, ssize_t total_size)> callback) {
    ssize_t rc;
    for(auto& [ifname,sock] : _sockets)
    {
      rc = sock.recv(buf, buf_size);
      if(rc != -1) {
        struct ether_header header = *static_cast<struct ether_header *>(buf);
        callback(header, ifname, rc);
      }
    } 

  }

  void broadcast(void* buf, size_t buf_size);
  void sendTo(std::string iface, void* buf, size_t buf_size);
};
