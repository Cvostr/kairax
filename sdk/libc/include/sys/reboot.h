#ifndef _SYS_REBOOT_H
#define _SYS_REBOOT_H

#ifdef __cplusplus
extern "C" {
#endif


#define REBOOT_CMD_POWER_OFF    0x30
#define REBOOT_CMD_REBOOT       0x40 

int reboot (int flag);

#ifdef __cplusplus
}
#endif


#endif