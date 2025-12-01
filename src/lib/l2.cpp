#include "l2.h"
#include <cstddef>

size_t L2::generate_ether_header(char* buffer, size_t buf_size, uint16_t protocol,
        MAC_addr &src, MAC_addr &dest)
{
    assert(buf_size >= sizeof(struct ether_header));

    memset(buffer, 0, sizeof(struct ether_header));
    struct ether_header *header = reinterpret_cast<struct ether_header*>(buffer);
    
    header->ether_type = htons(protocol);
    std::copy(std::begin(src.addr), std::end(src.addr), 
        header->ether_shost);
    std::copy(std::begin(dest.addr), std::end(dest.addr), 
        header->ether_dhost);

    return sizeof(struct ether_header);

}