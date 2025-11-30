#include "link.h"

int main()
{
    // test server
    char buf[MAX_BUFFER] = {};
    sprintf(buf, "%s", "TEST SEND");

    std::cout << "Starting raw socket" << std::endl;
    RawSocket socket("lo");
    std::cout << "Sending Packet" << std::endl;
    socket.blockingSend(buf, MAX_BUFFER);
    
    char buf2[MAX_BUFFER] = {};
    std::cout << "Waiting for Packet" << std::endl;
    socket.blockingRecv(buf2, MAX_BUFFER);
    std::cout << "done" << std::endl;
    std::cout << std::string(buf2) << std::endl;
    
    return 0;
}