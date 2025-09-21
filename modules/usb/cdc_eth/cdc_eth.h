#ifndef _CDC_ETH_H
#define _CDC_ETH_H

int usb_cdc_hex2int(char hex);

int cdc_eth_init(void);
void cdc_eth_exit(void);

#endif