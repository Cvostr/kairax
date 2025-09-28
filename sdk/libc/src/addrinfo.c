#include "netdb.h"
#include "stdlib.h"
#include <string.h>

#include "sys/socket.h"
#include <netinet/in.h> 
#include "arpa/inet.h"
#include "arpa/nameserv.h"
#include "unistd.h"

#include "stdio.h"

#define NSSERV  "8.8.8.8"

void freeaddrinfo(struct addrinfo *res) 
{
    while (res) 
    {
        struct addrinfo *curr;
        curr = res;
        res = curr->ai_next;
        free(curr);
    }
}

const char* gai_strerror(int error) 
{
    switch (error) {
        case EAI_FAMILY: return "Family not supported";
        case EAI_SOCKTYPE: return "Socket type not supported";
        case EAI_NONAME: return "Unknown host";
        case EAI_SERVICE: return "Unknown service";
        case EAI_MEMORY: return "No memory";
        case EAI_AGAIN: return "Temporary failure";
    }
    return "Unknown DNS error";
}

int getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res)
{
    int i;
    int sockfd = -1;
    size_t question_sz = 0;
    uint8_t total_questions = 0;
    size_t total_req_sz = sizeof(DNS_HEADER);
    size_t total_struct_sz;
    struct sockaddr_in     dnssrv_addr; 

    // Выставление параметров из hints или по умолчанию 
    int ai_family = (hints != NULL) ? hints->ai_family : AF_UNSPEC; 
    int ai_socktype = (hints != NULL) ? hints->ai_socktype : 0;
    int ai_protocol = (hints != NULL) ? hints->ai_protocol : 0; 
    int ai_flags = (hints != NULL) ? hints->ai_flags : 0; 

    uint16_t sock_port = 0;

    // Проверка ai_family
    if (!(ai_family == AF_INET || ai_family == AF_INET6 || ai_family == AF_UNSPEC))
    {
        return EAI_FAMILY;
    }

    if (node == NULL)
    {
        return -EAI_NONAME;
    }

    if (service != NULL)
    {
        char* endptr;
        // Попробуем перевести строку в число
        sock_port = strtol(service, &endptr, 10);
        if (*endptr == '\0') 
        {
            // Это была числовая строка
            sock_port = htons(sock_port);
        } 
        else 
        {
            // TODO: поиск по названию?
            return -EAI_SERVICE;
        }
    }

    if (ai_family == AF_INET || ai_family == AF_UNSPEC)
    {
        // Если IPv4 или не указано, попробуем считать node как адрес IPv4
        struct in_addr inet_addr;
        if (inet_aton(node, &inet_addr) != 0)
        {
            // в node указали ip адрес 
            // считаем суммарный размер одной структуры addrinfo
            total_struct_sz = sizeof(struct addrinfo) + sizeof(struct sockaddr_in); 

            struct addrinfo* new = malloc(total_struct_sz);
            if (new == NULL)
            {
                return -EAI_MEMORY;
            }
            memset(new, 0, total_struct_sz);
            *res = new;

            // Заполнение структуры
            new->ai_family = AF_INET;
            new->ai_canonname = ((void*) new) + sizeof(struct addrinfo);
            new->ai_addrlen = sizeof(struct sockaddr_in);
            new->ai_addr = ((void*)new) + sizeof(struct addrinfo);

            struct sockaddr_in *ipaddr = (struct sockaddr_in *) new->ai_addr;
            ipaddr->sin_family = AF_INET;
            ipaddr->sin_port = sock_port;
            memcpy(&ipaddr->sin_addr, &inet_addr, 4);
            return 0;
        }
    }

    // заполнение структуры с адресом
    dnssrv_addr.sin_family = AF_INET; 
    dnssrv_addr.sin_port = htons(53); 
    dnssrv_addr.sin_addr.s_addr = inet_addr(NSSERV); 

    // Формирование запроса
    char nsreq[256];
    memset(nsreq, 0, sizeof(nsreq));
    DNS_HEADER* nsreq_header = (DNS_HEADER*) nsreq;
    // Формируем DNS заголовок
    nsreq_header->id = 0xAA19;
    nsreq_header->qr = 0;  // Это вопрос
    nsreq_header->opcode = OP_QUERY;
    nsreq_header->rd = 1;  // Разрешена рекурсия
    // Формируем вопросы
    char* dst = nsreq + sizeof(DNS_HEADER);

    if (ai_family == AF_INET || ai_family == AF_UNSPEC)
    {
        question_sz = ns_form_question(dst, node, TYPE_A, CLASS_IN);
        total_req_sz += question_sz;
        dst += question_sz;
        total_questions ++;
    }

    if (ai_family == AF_INET6 || ai_family == AF_UNSPEC)
    {
        question_sz = ns_form_question(dst, node, TYPE_AAAA, CLASS_IN);
        total_req_sz += question_sz;
        dst += question_sz;
        total_questions ++;
    }

    // Добавляем в заголовок количество вопросов
    nsreq_header->qdcount = htons(total_questions);

    // Буфер ответа
    char nsresp[256];
    ssize_t ns_response_len = 0;

    if (ai_socktype == SOCK_DGRAM || ai_socktype == 0)
    {
        if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
        { 
            return EAI_SYSTEM;    
        } 

        sendto(sockfd, (const char *) nsreq, total_req_sz, 
            0, (const struct sockaddr *) &dnssrv_addr,  
                sizeof(dnssrv_addr)); 

        int resp_addr_len = sizeof(dnssrv_addr);
        ns_response_len = recvfrom(sockfd, (char *)nsresp, sizeof(nsresp),  
                0, (struct sockaddr *) &dnssrv_addr, 
                &resp_addr_len); 

        if (ns_response_len < 0)
        {
            return EAI_SYSTEM;
        }

        close(sockfd);
    }
    else
    {
        return EAI_SOCKTYPE;
    }

    DNS_HEADER* nsresp_header = (DNS_HEADER*) nsresp;
    
    if ((nsresp_header->id != nsreq_header->id) || (nsresp_header->qr != 1))
    {
        return EAI_SYSTEM;
    }

    if (nsresp_header->rcode == RCODE_SERVFAIL)
    {
        return EAI_AGAIN;
    }
    if (nsresp_header->rcode == RCODE_NXDOMAIN)
    {
        return EAI_NONAME;
    }
    if (nsresp_header->rcode != 0)
    {
        return EAI_FAIL;
    }

    // Считаем колво запросов и ответов
    uint16_t resp_questions = ntohs(nsresp_header->qdcount);
    uint16_t resp_answers = ntohs(nsresp_header->ancount);

    char tempname[256];
    char* resp_current = nsresp + sizeof(DNS_HEADER);

    // Пропустим отзеркленную секцию вопросов
    for (i = 0; i < resp_questions; i ++)
    {
        size_t len = ns_read_query(resp_current, tempname, NULL, NULL);
        resp_current += len;
    }

    struct addrinfo* root = NULL;
    struct addrinfo* current = NULL;

    // Обработаем ответы
    for (i = 0; i < resp_answers; i ++)
    {
        DNS_ANSWER* answ = (DNS_ANSWER*) resp_current;
        uint16_t rdlen = ntohs(answ->rdlength);
        
        // переход к следующему ответу
        resp_current += sizeof(DNS_ANSWER) + rdlen;

        // Считать имя
        uint16_t nameoffset = answ->name & 0x3FFF;
        ns_read_name(nsresp + nameoffset, tempname);
        size_t namelen = strlen(tempname);

        int resp_ai_family = 0;
        int sock_struct_size = 0;
        switch (ntohs(answ->type))
        {
            case TYPE_A:
                resp_ai_family = AF_INET;
                sock_struct_size = sizeof(struct sockaddr_in);
                break;
            case TYPE_AAAA:
                resp_ai_family = AF_INET6;
                sock_struct_size = sizeof(struct sockaddr_in6);
                break;
        }

        // считаем суммарный размер одной структуры addrinfo
        total_struct_sz = sizeof(struct addrinfo) + (namelen + 1) + sock_struct_size; 

        struct addrinfo* new = malloc(total_struct_sz);
        if (new == NULL)
        {
            return -EAI_MEMORY;
        }
        memset(new, 0, total_struct_sz);
        if (root == NULL) {
            root = new;
        }
        if (current != NULL) {
            current->ai_next = new;
        }
        current = new;

        // Заполнение структуры
        new->ai_family = resp_ai_family;
        new->ai_canonname = ((void*) new) + sizeof(struct addrinfo);
        strcpy(new->ai_canonname, tempname);
        new->ai_addrlen = sock_struct_size;
        new->ai_addr = ((void*)new) + sizeof(struct addrinfo) + (namelen + 1);

        switch (ntohs(answ->type))
        {
            case TYPE_A:
                struct sockaddr_in *ipaddr = (struct sockaddr_in *) new->ai_addr;
                ipaddr->sin_family = AF_INET;
                ipaddr->sin_port = sock_port;
                memcpy(&ipaddr->sin_addr, answ->rddata, 4);
                break;
            case TYPE_AAAA:
                struct sockaddr_in6 *ip6addr = (struct sockaddr_in6 *) new->ai_addr;
                ip6addr->sin6_family = AF_INET6;
                ip6addr->sin6_port = sock_port;
                memcpy(&ip6addr->sin6_addr, answ->rddata, 16);
                break;
        }
    }

    *res = root;

    return 0;
}
