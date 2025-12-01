#include "arp.h"
#include "link.h"
#include <arpa/inet.h>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <net/ethernet.h>
#include <netinet/in.h>

volatile sig_atomic_t stop;
void st([[maybe_unused]] int signum) { stop = 1; }

// Router main loop
int main(int argc, char *argv[]) {

  if (argc < 2) {
    std::cerr << "Usage ./server dev default" << std::endl;
    return 1;
  }

  std::string def_gateway = "";
  if (argc == 3)
    def_gateway = std::string(argv[2]);

  RawSocket socket(argv[1]);
  ARP_Service arp(socket);

  // signal(SIGINT, st);

  char *buffer = (char *)malloc(MAX_BUFFER);

  // advertise
  if (def_gateway != "")
    arp.broadcast_arp_requests(inet_addr(def_gateway.c_str()));

  while (true) {
    socket.blockingRecv(buffer, MAX_BUFFER);

    struct ether_header *header =
        reinterpret_cast<struct ether_header *>(buffer);
    if (header->ether_type == htons(ARP)) {
      std::cout << "Received a ARP packet!" << std::endl;
      arp.process_arp_packet(buffer, MAX_BUFFER, sizeof(ether_header));
    } else if (header->ether_type == htons(IPv4))
      std::cerr << "IPv4 not yet implemented" << std::endl;
    else
      std::cerr << "Eth frame with type " << header->ether_type
                << " unrecognized! DROP!" << std::endl;
  }
  std::cout << "Stopping" << std::endl;
  return 0;
}