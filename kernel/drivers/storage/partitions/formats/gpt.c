#include "gpt.h"

int check_gpt_header_signature(gpt_header_t* header){
    for(int i = 0; i < 8; i ++){
        if(header->signature[i] != GPT_SIGNATURE[i])
            return 0;
    }
    return 1;
}