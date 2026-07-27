#include "frame_queue.h"

QueueHandle_t s_frame_queue;

void frame_queue_init(void)
{
    s_frame_queue = xQueueCreate(FRAME_QUEUE_LEN, sizeof(captured_frame_t));
}
