#ifndef _PIC_H
#define _PIC_H

#include "stdint.h"
#include "io.h"

#define PIC1_START_INTERRUPT 0x20
#define PIC2_START_INTERRUPT 0x28

#define INT_IRQ0 0x20
#define ICW4_8086 0x1

#define IRQ0  0x0 // timer
#define IRQ1  0x1 // keyboard
#define IRQ2  0x2 // cascade PIC
#define IRQ3  0x3 // COM2
#define IRQ4  0x4 // COM1
#define IRQ5  0x5 // LPT2
#define IRQ6  0x6 // Floppy disk
#define IRQ7  0x7 // LTP1
#define IRQ8  0x8 // CMOS real-time clock
#define IRQ9  0x9 // Free for peripherals (SCSI / NIC)
#define IRQ10 0xA // Free for peripherals (SCSI / NIC)
#define IRQ11 0xB // Free for peripherals (SCSI / NIC)
#define IRQ12 0xC // PS2 MOUSE
#define IRQ13 0xD // FPU (Floating Point Unit) / Coprocessor / inter-processor
#define IRQ14 0xE // Primary ATA hard disk
#define IRQ15 0xF // Secondary ATA hard disk

#define PIC_EOI 0x20 /* data end of interrupt */

#define PORT_PIC_MASTER_CMD      0x20
#define PORT_PIC_MASTER_DATA     0x21

#define PORT_PIC_SLAVE_CMD       0xA0
#define PORT_PIC_SLAVE_DATA      0xA1

#define PIC_EOI 0x20
//необходимо вызывать после каждого IRQ прерывания
void pic_eoi(uint8_t irq);
//Инициализация PIC
void init_pic(void);
//Разрешить прерывание IRQ
void pic_unmask(uint8_t irq);

void pic_mask(uint8_t irq);

#endif