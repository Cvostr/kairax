#include "eth.h"

   uint16_t switch_endian16(uint16_t nb) {
       return (nb>>8) | (nb<<8);
   }
   
   uint32_t switch_endian32(uint32_t nb) {
       return ((nb>>24)&0xff)      |
              ((nb<<8)&0xff0000)   |
              ((nb>>8)&0xff00)     |
              ((nb<<24)&0xff000000);
   }

void eth_handle_frame(unsigned char* data, size_t len)
{
    struct ethernet_frame* frame = (struct ethernet_frame*) data;
    printk("src: ");
    for (int i = 0; i < 6; i ++) {
        printk("%i ", frame->src[i]);
    }    
    printk("\ndest: ");
    for (int i = 0; i < 6; i ++) {
        printk("%i ", frame->dest[i]);
    }    
    printk("\ntype: %i\n", switch_endian16(frame->type));

}