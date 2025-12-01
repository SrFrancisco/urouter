#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <ios>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <string>

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

class L2 {
public:
  size_t generate_ether_header(char *buffer, size_t buf_size, uint16_t protocol,
                               MAC_addr &src, MAC_addr &dest);
  
};