#include "arpa/nameserv.h"
#include "netinet/in.h"
#include "string.h"

size_t ns_read_query(char* in, char* name, uint16_t* qtype, uint16_t* qclass)
{
    size_t resp = ns_read_name(in, name);
    resp += 4;
    return resp;
}

size_t ns_form_question(char* out, const char* name, int type, int class)
{
    size_t encoded_name_len = ns_encode_name(out, name);
    uint16_t* next = (uint16_t*) (out + encoded_name_len);
    // QTYPE
    next[0] = htons(type);
    // QCLASS
    next[1] = htons(class);

    return encoded_name_len + 4;
}

size_t ns_encode_name(char* dst, const char* name)
{
    char* nptr = (char*) name;
    char* dot;
    size_t seg_len = 0;
    size_t result = 0;

    while (*nptr)
    {
        dot = strchr(nptr, '.');

        if (dot != NULL)
        {
            seg_len = dot - nptr;
        } 
        else
        {
            seg_len = strlen(nptr);
        }

        // Записываем длину сегмента
        dst[0] = seg_len;

        // Копируем сегмент адреса
        memcpy(dst + 1, nptr, seg_len);

        // Сдвигаем указатели
        dst += (seg_len + 1);
        result += (seg_len + 1);
        
        nptr += seg_len;

        if (*nptr == '\0')
        {
            break;
        }
        else
        {
            // в nptr указатель на символ '.', сдвигаемся на 1
            nptr += 1;
        }
    }

    // Не забываем про 0 в конце
    dst[0] = '\0';
    result ++;

    return result;
}

size_t ns_read_name(char* in, char* name)
{
    char* p = in;
    size_t result = 0;
    
    while (*p != 0)
    {
        uint8_t len = *p;
        p++;
        memcpy(name, p, len);
        p += len;
        result += (len + 1);

        name += len;
        if (*p == 0)
        {
            name[0] = '\0';
        }
        else
        {
            name[0] = '.';
            name += 1;   
        }
    }

    return result + 1;
}