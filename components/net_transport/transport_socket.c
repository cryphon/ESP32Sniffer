#include "transport_socket.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include <string.h>

static const char* TAG = "transport_socket";

static int s_sock = -1;

#define TARGET_IP       CONFIG_TRANSPORT_SOCKET_TARGET_IP
#define TARGET_PORT     CONFIG_TRANSPORT_SOCKET_TARGET_PORT

static esp_err_t socket_connect(void)
{
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr(TARGET_IP);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(TARGET_PORT);

    s_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP); /* UDP: no handshake to redo per flush cycle */
    if(s_sock < 0)
    {
        ESP_LOGE(TAG, "socket() failed: errno %d", errno);
        return ESP_FAIL;
    }

    if(connect(s_sock,(struct sockaddr*)&dest_addr, sizeof(dest_addr)) != 0)
    {
        ESP_LOGE(TAG, "connect() failed: errno %d", errno);
        close(s_sock);
        s_sock = -1;
        return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t socket_init(void)
{
    if (s_sock >= 0) {
        close(s_sock);
        s_sock = -1;
    }
    return socket_connect();
}
static esp_err_t socket_send(const uint8_t* buf, size_t len)
{
    if (s_sock < 0) {
        if (socket_connect() != ESP_OK) {
            return ESP_FAIL;
        }
    }
    int res = send(s_sock, buf, len, 0);
    if (res < 0) {
        if (errno == ENOMEM) {
            ESP_LOGW(TAG, "send() transient ENOMEM, dropping frame, socket kept alive");
            return ESP_FAIL;   // drop this frame, but don't close the socket
        }
        ESP_LOGW(TAG, "send() failed: errno %d, will reconnect next call", errno);
        close(s_sock);
        s_sock = -1;
        return ESP_FAIL;
    }
    return ESP_OK;
}

static void socket_deinit(void)
{
    if(s_sock >= 0)
    {
        close(s_sock);
        s_sock = -1;
    }
}

static const transport_strategy_t s_transport = {
    .name = "socket",
    .init = socket_init,
    .send = socket_send,
    .deinit = socket_deinit,
};

const transport_strategy_t* transport_socket_get(void)
{
    return &s_transport;
}


