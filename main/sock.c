#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "defs.h"

static int sock = -1;
static struct sockaddr_in dest_addr;

void sock_init(void)
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


