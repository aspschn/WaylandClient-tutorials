#ifndef _BLUSHER_POINTER_EVENT_H
#define _BLUSHER_POINTER_EVENT_H

#include <stdint.h>

typedef struct bl_pointer_event {
    uint32_t button;
} bl_pointer_event;

bl_pointer_event* bl_pointer_event_new();

void bl_pointer_event_free(bl_pointer_event *event);

#endif /* _BLUSHER_POINTER_EVENT_H */
