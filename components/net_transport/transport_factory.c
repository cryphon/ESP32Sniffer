#include "transport_factory.h"
#include "transport_socket.h"

const transport_strategy_t* transport_get_active(void)
{
#if CONFIG_TRANSPORT_USB
    //return transport_usb_get();
#else
    return transport_socket_get();
#endif
}
