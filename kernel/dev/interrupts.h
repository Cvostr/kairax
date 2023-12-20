#ifndef _INTERRUPTS_H
#define _INTERRUPTS_H

int register_irq_handler(int irq, void* handler, void* data);

//todo : опознавание устройства
int unregister_irq_handler(int irq);

#endif