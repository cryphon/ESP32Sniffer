#include "net_transport.h"

#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "defs.h"

static int sock = -1;
static struct sockaddr_in dest_addr;

void net_transport_init(void)
{
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if(sock < 0)
    {
        printf("failed to create socket\n");
        return;
    }

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PC_PORT);
    inet_pton(AF_INET, PC_IP, &dest_addr.sin_addr);
}


bool net_transport_send(const void* buf, size_t len)
{
    if (sock < 0) return false;

    int res = sendto(sock, buf, len, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    if (res < 0)
    {
        printf("[WARNING] sendto failed: errno %d\n", errno);
        return false;
    }
    return true;
}
