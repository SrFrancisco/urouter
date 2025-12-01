#pragma once

/** This file implements the ARP protocol and an ARP table */

#include "l2.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <link.h>
#include <net/ethernet.h>
#include <net/if_arp.h>
#include <optional>
#include <unordered_map>

using ARP_Table_Entry = std::pair<uint32_t, MAC_addr>;
class ARP_Table {
private:
  std::unordered_map<uint32_t, MAC_addr> _table = {};

public:
  ARP_Table() = default;

  std::optional<ARP_Table_Entry> query(uint32_t ip);
  void insert_or_update(ARP_Table_Entry entry);
  void print();
};

struct arp_request {
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

class ARP_Service : L2 {
private:
  MAC_addr _own_addr;
  uint32_t _own_ip;
  ARP_Table _arp_table;
  RawSocket &_socket;
  MAC_addr _BROADCAST;

public:
  ARP_Service(RawSocket &sock);

  void broadcast_arp_requests(uint32_t target_ip);

  void process_arp_packet(char *buf, size_t buf_size, size_t stride);
};