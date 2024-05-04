#ifndef _DEVICE_MAN_H
#define _DEVICE_MAN_H

#include "device.h"
#include "device_drivers.h"

int register_device(struct device* dev);

struct device* get_device(int index);

struct device* get_device_with_type(int type, int index);

void probe_device(struct device* dev);

void probe_devices();

int device_assign_shortname(struct device* dev, const char* name);

#endif