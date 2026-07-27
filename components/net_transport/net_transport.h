#pragma once
#include <stddef.h>
#include <stdbool.h>

void net_transport_init();
bool net_transport_send(const void* buf, size_t len);
