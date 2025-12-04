#ifndef _INPUT_H
#define _INPUT_H

#include "stdint.h"

#define MOUSE_EVENT_BUTTON_UP       1
#define MOUSE_EVENT_BUTTON_DOWN     2
#define MOUSE_EVENT_MOVE            3
#define MOUSE_EVENT_SCROLL          4

#define MOUSE_BUTTON_LEFT           1
#define MOUSE_BUTTON_MIDDLE         2
#define MOUSE_BUTTON_RIGHT          3

struct mouse_event {
    uint8_t type;

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

#endif