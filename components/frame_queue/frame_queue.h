#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <stdint.h>

#define FRAME_MAX_LEN 1472
#define FRAME_QUEUE_LEN 64

typedef struct {
    uint16_t len;
    uint8_t  data[FRAME_MAX_LEN];
} captured_frame_t;

extern QueueHandle_t s_frame_queue;

/* call once, before frame_capture_handler or any consumer touches the queue */
void frame_queue_init(void);
