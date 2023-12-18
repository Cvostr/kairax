#ifndef _DEVICE_MAN_H
#define _DEVICE_MAN_H

#include "device.h"
#include "device_drivers.h"

int register_device(struct device* dev);

struct device* get_device(int index);

void probe_device(struct device* dev);

void probe_devices();

#endif