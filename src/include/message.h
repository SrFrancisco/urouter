#pragma once

#include <cstddef>
#include <cstdint>

#define ASSERT_ALIGNMENT(BUF,TYPE) \
  (assert(reinterpret_cast<uintptr_t>(BUF) % alignof(TYPE) == 0))

template <typename T>
struct L2Message {
  using type = T;
  static constexpr uint16_t ETHER_TYPE = 0x0000;

  inline size_t len() { return sizeof(T); }

  L2Message() = default;
};

struct arp_request : L2Message<arp_request> {
  static constexpr uint16_t ETHER_TYPE = 0x0806;

  uint16_t hrd;   /** Hardware Type */
  uint16_t pro;   /** Protocol Type */
  uint8_t hln;    /** Hardware Length */
  uint8_t pln;    /** Protocol Length */
  uint16_t op;    /** Operation */
  uint8_t sha[6]; /** Sender Hardware Address */
  uint32_t spa;   /** Sender protocol address */
  uint8_t tha[6]; /** Target hardware address */
  uint32_t tpa;   /** Target protocol address */

} __attribute__((__packed__));

struct ipv4_request : L2Message<ipv4_request> {
  static constexpr uint16_t ETHER_TYPE = 0x0800;
  //TODO
};
