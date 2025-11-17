#ifndef _MOUSE_H
#define _MOUSE_H

#include "kairax/types.h"

#define KEY_BUFFER_SIZE 20
#define KEY_BUFFERS_MAX 10

#define MOUSE_EVENT_BUTTON_UP       1
#define MOUSE_EVENT_BUTTON_DOWN     2
#define MOUSE_EVENT_MOVE            3
#define MOUSE_EVENT_SCROLL          4

#define MOUSE_BUTTON_LEFT           1
#define MOUSE_BUTTON_MIDDLE         2
#define MOUSE_BUTTON_RIGHT          3

struct mouse_event {
    uint8_t event_type;

    union {
        struct {
            uint32_t id;
        } btn;

        struct {
            int32_t rel_x;
            int32_t rel_y;
        } move;

        struct {
            int32_t value;
        } scroll;

    } u_event;
};

struct mouse_buffer {
    struct mouse_event events[KEY_BUFFER_SIZE];
    unsigned char start;
    unsigned char end;
};

void mouse_add_event(struct mouse_event* event);
void mouse_init();

#endif