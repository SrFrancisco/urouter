#pragma once

#include <assert.h>
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iterator>
//#include <linux/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
//#include <linux/if_ether.h>
#include <netinet/in.h>
#include <new>
#include <string>
#include <unistd.h>
#include <memory>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <utility>
#include "l2.h"
#include <netinet/ip.h> // see: https://stackoverflow.com/questions/42840636/difference-between-struct-ip-and-struct-iphdr

static constexpr unsigned long MAX_BUFFER = 1 << 13;

struct RawSocket {
    struct sockaddr_ll socket_addr;
    int socketfd;
    MAC_addr mac;
    std::string iface_mame;

    RawSocket(std::string network_iface);

    ~RawSocket();

    void blockingRecv(void *buf, size_t buf_size);
    
    /**
     * WARN! This function will not verify the structure of the payload. Make
     *       sure it is a correct L2 payload with Ethernet Header + all above
     */
    void blockingSend(void* buf, size_t buf_size);

private:
    inline void _cleanup()
    {
        assert(socketfd != -1);
        close(socketfd);
        socketfd = -1; // just in case
    }
};
