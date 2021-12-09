#include "pointer-event.h"

#include <stdlib.h>

bl_pointer_event* bl_pointer_event_new()
{
    bl_pointer_event *event = malloc(sizeof(bl_pointer_event));

    return event;
}

void bl_pointer_event_free(bl_pointer_event *event)
{
    if (event == NULL) {
        return;
    }
    free(event);
}
