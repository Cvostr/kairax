#include "netdb.h"
#include "string.h"
#include "stdlib.h"

#include "sys/socket.h"
#include <netinet/in.h> 
#include "arpa/inet.h"
#include "arpa/nameserv.h"
#include "unistd.h"

#include "stdio.h"

#define NSSERV  "8.8.8.8"

#define HOSTENTBUFSZ    1024
char hostent_buffer[HOSTENTBUFSZ];

char* hostent_alloc(size_t* m, size_t len)
{
    size_t temp = *m;
    *m += len;
    return hostent_buffer + temp;
}

struct hostent *gethostbyname(const char *name)
{
    return gethostbyname2(name, AF_INET);
}

const char* hstrerror(int e) {
    switch (e) {
        case 0: 
            return "OK";
        case NO_DATA: 
            return "No data of requested type.";
        case TRY_AGAIN: 
            return "Temporary failure.";
        case HOST_NOT_FOUND:
        default: 
            return "Unknown host.";
    }
}

struct hostent *gethostbyname2(const char *name, int af)
{
    int i;
    int sockfd = -1;
    struct sockaddr_in     dnssrv_addr; 

    int QTYPE = 0;
    int h_len = 0;
    
    switch (af)
    {
    case AF_INET:
        QTYPE = TYPE_A;
        h_len = 4;
        break;
    case AF_INET6:
        QTYPE = TYPE_AAAA;
        h_len = 16;
        break;
    default:
        h_errno = NETDB_INTERNAL;
        return NULL;
    }

    dnssrv_addr.sin_family = AF_INET; 
    dnssrv_addr.sin_port = htons(53); 
    dnssrv_addr.sin_addr.s_addr = inet_addr(NSSERV); 

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) 
    { 
        h_errno = NETDB_INTERNAL;
        return NULL;    
    } 

    char nsreq[256];
    memset(nsreq, 0, sizeof(nsreq));
    DNS_HEADER* nsreq_header = (DNS_HEADER*) nsreq;
    // Формируем DNS заголовок
    nsreq_header->id = 0xAA19;
    nsreq_header->qr = 0;  // Это вопрос
    nsreq_header->opcode = OP_QUERY;
    nsreq_header->rd = 1;  // Разрешена рекурсия
    nsreq_header->qdcount = htons(1); // Всего 1 запрос
    // Формируем вопрос
    char* dst = nsreq + sizeof(DNS_HEADER);
    size_t question_sz = ns_form_question(dst, name, QTYPE, CLASS_IN);

    size_t total_sz = sizeof(DNS_HEADER) + question_sz;

    sendto(sockfd, (const char *) nsreq, total_sz, 
        0, (const struct sockaddr *) &dnssrv_addr,  
            sizeof(dnssrv_addr)); 
           
    char nsresp[256];
    int resp_addr_len = sizeof(dnssrv_addr);
    size_t ns_resp_len = recvfrom(sockfd, (char *)nsresp, sizeof(nsresp),  
                0, (struct sockaddr *) &dnssrv_addr, 
                &resp_addr_len); 

    close(sockfd);
    
    DNS_HEADER* nsresp_header = (DNS_HEADER*) nsresp;
    
    if (nsresp_header->id != nsreq_header->id)
    {
        h_errno = TRY_AGAIN;
        return NULL;
    }

    if (nsresp_header->qr != 1)
    {
        h_errno = TRY_AGAIN;
        return NULL;
    }

    // Маппинг кода ошибок
    int rcode = nsresp_header->rcode; 
    switch (rcode)
    {
    case RCODE_NOERROR:
        break;
    case RCODE_NXDOMAIN:
        h_errno = HOST_NOT_FOUND;
        return NULL;
    case RCODE_SERVFAIL:
        h_errno = TRY_AGAIN;
        return NULL;
    case RCODE_FORMERR:
    case RCODE_REFUSED:
    case RCODE_NOTIMPL:
        h_errno = NO_RECOVERY;
        return NULL;
    default:
        h_errno = NO_ADDRESS;
        return NULL;
    }

    uint16_t resp_questions = ntohs(nsresp_header->qdcount);
    uint16_t resp_answers = ntohs(nsresp_header->ancount);

    size_t hostent_buff_offset = sizeof(struct hostent);
    // Зануляем буфер
    struct hostent *result = (struct hostent *) hostent_buffer;
    memset(result, 0, HOSTENTBUFSZ);
    // Заполнить начальную информацию
    result->h_addrtype = af;
    result->h_length = h_len;

    // Выделить память под список ответов
    size_t addrlist_len = sizeof(char*) * (resp_answers + 1);
    result->h_addr_list = (char**) hostent_alloc(&hostent_buff_offset, addrlist_len);
    memset(result->h_addr_list, 0, addrlist_len);

    //printf("DNS qu %i answ %i\n", resp_questions, resp_answers);

    char tempname[256];
    char* resp_current = nsresp + sizeof(DNS_HEADER);

    // Пропустим отзеркленную секцию вопросов
    for (i = 0; i < resp_questions; i ++)
    {
        size_t len = ns_read_query(resp_current, tempname, NULL, NULL);
        resp_current += len;
        //printf("DNS query %s\n", tempname);
    }

    // Обработаем ответы
    for (i = 0; i < resp_answers; i ++)
    {
        DNS_ANSWER* answ = (DNS_ANSWER*) resp_current;
        uint16_t rdlen = ntohs(answ->rdlength);
        
        // переход к следующему ответу
        resp_current += sizeof(DNS_ANSWER) + rdlen;
        // Записать адрес
        result->h_addr_list[i] = hostent_alloc(&hostent_buff_offset, result->h_length);
        memcpy(result->h_addr_list[i], answ->rddata, result->h_length);
    }

    return result;
}