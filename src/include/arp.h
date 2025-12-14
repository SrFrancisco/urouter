#pragma once

/** This file implements the ARP protocol and an ARP table */

#include "socket.h"
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
  char* send_buf;

public:
  ARP_Table() = default;

  std::optional<ARP_Table_Entry> query(uint32_t ip);
  void insert_or_update(ARP_Table_Entry entry);
  void print();
};

class ARP_Service {
private:
  MAC_addr _own_addr;
  uint32_t _own_ip;
  ARP_Table _arp_table;
  RawSocket &_socket;
  MAC_addr _BROADCAST;

  // buffer for preparing packets
  char* buffer;
  size_t buffer_len;

public:
  ARP_Service(RawSocket &sock, char* buf, size_t buf_len);

  void broadcast_arp_requests(uint32_t target_ip);
  void process_arp_packet(char *buf, size_t buf_size, size_t stride);
};