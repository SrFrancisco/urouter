#include "arp.h"
#include "link.h"
#include "message.h"
#include "socket.h"
#include <arpa/inet.h>
#include <cassert>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <ostream>
#include <unistd.h>
#include <unordered_map>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        std::cerr << "Usage ./switch [iface]+" << std::endl;
        return 1;
    }
 
    SocketManager manager;
    for(long i = 1; i < argc; ++i)
    {
        std::cout << "Starting socket in " << argv[i] << std::endl;
        manager.addConnection(std::string(argv[i]));
    }

    char *recv_buffer = (char *)malloc(MAX_BUFFER);

    //TODO: Have a tiemout system to invalidade entries
    std::unordered_map<MAC_addr, std::string> mac_table{};
    while(true)
    {
        manager.L2recvApplyAll(recv_buffer, MAX_BUFFER, 
            [&](struct ether_header header, std::string iface ,ssize_t total_len) 
            {
                auto dest_mac = MAC_addr::from_c_struct(header.ether_dhost);
                auto src_mac = MAC_addr::from_c_struct(header.ether_shost);

                std::cout << "<== MAC: Who has " << dest_mac.toString() 
                          << " tell " << src_mac.toString() << std::endl;

                // register source
                mac_table[src_mac] = iface;

                if(!mac_table.contains(dest_mac))
                {
                    // bcast to all interfaces
                    manager.broadcast(recv_buffer, total_len);
                }
                else
                {
                    // send to known iface
                    manager.sendTo(mac_table.at(dest_mac), recv_buffer, total_len);
                }

                std::cout << " --- START MAC TABLE --- " << std::endl;
                for(const auto& [m, f] : mac_table)
                    std::cout << "- MAC: " << m.toString() << " IFACE: " << f << std::endl;
                std::cout << " --- END MAC TABLE --- " << std::endl;
            }
        );
    }

    return 0;
}