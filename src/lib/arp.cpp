#include "arp.h"
#include "message.h"
#include "socket.h"
#include <arpa/inet.h>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <link.h>
#include <netinet/in.h>
#include <optional>
#include <sys/ioctl.h>

#define ARP_REQUEST (htons(1))
#define ARP_ANSWER (htons(2))

std::optional<ARP_Table_Entry> ARP_Table::query(uint32_t ip) {
  if (auto it = _table.find(ip); it != _table.end())
    return {{it->first, it->second}};
  else
    return {};
}

void ARP_Table::insert_or_update(ARP_Table_Entry entry) {
  if (query(entry.first).has_value())
    _table[entry.first] = entry.second;
  else
    _table.insert(entry);

  // FIXME: Debug
  print();
}

void ARP_Table::print() {
  std::cout << " --- START ARP TABLE ---" << std::endl;
  for (auto &elem : _table) {
    struct in_addr ip_addr;
    ip_addr.s_addr = htonl(elem.first);
    std::string ip_str = inet_ntoa(ip_addr);
    std::cout << "- IP: " << ip_str << " MAC: " << elem.second.toString()
              << std::endl;
  }
  std::cout << " --- END ARP TABLE ---" << std::endl << std::endl;
}

ARP_Service::ARP_Service(RawSocket &sock, char* buf, size_t buf_len) : 
_socket(sock), _BROADCAST(), buffer(buf), buffer_len(buf_len) {
  std::memset(buffer, 0, buf_len);

  // set mac
  struct ifreq ifr;
  assert(sock.iface_mame.size() < IFNAMSIZ);
  memcpy(ifr.ifr_name, sock.iface_mame.c_str(), IFNAMSIZ);
  assert(ioctl(sock.socketfd, SIOCGIFHWADDR, &ifr) == 0);
  const uint8_t *u_mac = (uint8_t *)ifr.ifr_hwaddr.sa_data;
  printf("Local MAC Address : %02x:%02x:%02x:%02x:%02x:%02x\n", u_mac[0],
         u_mac[1], u_mac[2], u_mac[3], u_mac[4], u_mac[5]);
  _own_addr = MAC_addr();
  _own_addr.addr = {};
  memcpy(_own_addr.addr.data(), u_mac, 6);

  // set ip
  struct ifreq ifr_ip;
  assert(sock.iface_mame.size() < IFNAMSIZ);
  memcpy(ifr_ip.ifr_name, sock.iface_mame.c_str(), IFNAMSIZ);
  if (ioctl(sock.socketfd, SIOCGIFADDR, &ifr_ip) == 0) {
    auto addr = (struct sockaddr_in *)&ifr_ip.ifr_addr;
    auto address = inet_ntoa(addr->sin_addr);
    std::cout << "IP ADDR: " << address << std::endl;
    _own_ip = {inet_addr(address)};
  } else {
    // could not get the IP addr
    // TODO: This could be expected behaviour in I want to implement DHCP
    assert(false); // for now we assume if must have one
  }
}

void ARP_Service::broadcast_arp_requests(uint32_t target_ip_no) {
  // TODO: Impl circular buffer
  size_t stride =
      generate_ether_header(buffer, MAX_BUFFER, ARP, _own_addr, _BROADCAST);
  assert(MAX_BUFFER >= sizeof(arp_request) + stride);

  ASSERT_ALIGNMENT(buffer+stride, arp_request);
  struct arp_request *req =
      std::launder(reinterpret_cast<struct arp_request *>(buffer + stride));
  memset(req, 0, sizeof(struct arp_request));

  req->hrd = htons(1);
  req->pro = htons(IPv4);
  req->hln = 6;
  req->pln = 4;       // single byte, don't need reordering
  req->op = htons(1); // request
  req->spa = htonl(this->_own_ip);
  req->tpa = htonl(target_ip_no);

  std::copy(_own_addr.addr.begin(), _own_addr.addr.end(), req->sha);

  _socket.blockingSend(buffer, stride + sizeof(struct arp_request));

}


void ARP_Service::process_arp_packet(char *buf, size_t buf_size,
                                     size_t stride) {
  // we assume that the buf will be pointing to the struct
  assert(MAX_BUFFER >= stride + sizeof(arp_request));
  assert(stride < buf_size && buf_size <= MAX_BUFFER);
  
  ASSERT_ALIGNMENT(stride+buf, arp_request);
  struct arp_request *req =
      std::launder(reinterpret_cast<struct arp_request *>(stride + buf));

  {
    struct in_addr ip_addr;
    ip_addr.s_addr = htonl(req->spa);
    std::string ip_str = inet_ntoa(ip_addr);

    std::cout << "Sender IP:" << ip_str << std::endl;
  }

  {
    struct in_addr ip_addr;
    ip_addr.s_addr = htonl(req->tpa);
    std::string ip_str = inet_ntoa(ip_addr);

    std::cout << "Target IP:" << ip_str << std::endl;
  }

  // partially implement the algo from the RFC in `Packet Reception`
  if (req->pro == htons(IPv4)       // do I speak the protocol in ar$pro?
      && req->tpa == htonl(_own_ip) // am I the target protocol address?
      && req->op == htons(1)        // request
  ) {
    // the request is directed at the switch
    auto remote_mac = MAC_addr::from_c_struct(req->sha);
    auto remote_ip = req->spa;
    _arp_table.insert_or_update({remote_ip, remote_mac});

    size_t stride =
      generate_ether_header(buffer, MAX_BUFFER, ARP, _own_addr, remote_mac);
    assert(MAX_BUFFER >= sizeof(arp_request) + stride);

    ASSERT_ALIGNMENT(buffer+stride, arp_request);
    struct arp_request *resp =
        std::launder(reinterpret_cast<struct arp_request *>(buffer + stride));
    memcpy(resp, req, sizeof(struct arp_request));
    // We will use the same memory to send the reply, with the fields swapped
    uint8_t tmp[6];
    memcpy(tmp, resp->sha, 6);
    memcpy(resp->sha, _own_addr.addr.data(), 6);
    memcpy(resp->tha, tmp, 6);
    uint32_t tmp_ip = resp->spa;
    resp->spa = (resp->tpa);
    resp->tpa = (tmp_ip);
    resp->op = ARP_ANSWER;

    _socket.blockingSend(buffer, stride + sizeof(struct arp_request));

  } else if ((req->pro == htons(IPv4) && req->op == htons(1)) ||
             (req->tpa == htonl(_own_ip) && req->op == htons(2))) {
    auto remote_mac = MAC_addr::from_c_struct(req->sha);
    auto remote_ip = req->spa;
    _arp_table.insert_or_update({remote_ip, remote_mac});
  } else {
    assert(false); // should not happen
  }
}
