#include "link.h"

RawSocket::~RawSocket() { _cleanup(); }

RawSocket::RawSocket(std::string network_iface)
{
    // Initialize dest struct
    // see packet(7) man
    memset(&socket_addr, 0, sizeof(struct sockaddr_ll));
    // To send packets: specify sll_family, sll_addr, sll_halen, sll_ifindex, and sll_protocol
    socket_addr.sll_family = AF_PACKET;
    socket_addr.sll_protocol = htons(ETH_P_ALL); // get all packets (incl. IP and ARP)
    socket_addr.sll_ifindex = if_nametoindex(network_iface.c_str());
    assert(socket_addr.sll_ifindex != 0); // could not find interface

    socketfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    assert(socketfd != -1);

    int rc = bind(socketfd, 
                (struct sockaddr *) &socket_addr, 
                    sizeof(socket_addr));
    if(rc != 0)
    {
        std::cerr << "Could not bind socket!" << std::endl;
        close(socketfd);
        exit(1);
    }
}

void RawSocket::blockingSend(void* buf, size_t buf_size)
{
    assert(buf != nullptr);
    assert(socketfd > 0);
    int rc = send(socketfd, buf, buf_size,0);
    if(rc == -1)
    {
        std::cerr << "Err in send with errno " << errno << std::endl;
        _cleanup();
    }
    return;
}

void RawSocket::blockingRecv(void *buf, size_t buf_size) 
{
    int rc = ::recv(socketfd, buf, buf_size,0);
    if(rc == -1)
    {
        std::cerr << "Err in recv with errno " << errno << std::endl;
        _cleanup();
    }
}