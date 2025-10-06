#include "stdio.h"
#include "process.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "sys/wait.h"
#include "errno.h"
#include "spawn.h"
#include "threads.h"
#include "kairax.h"

void thr1() {
    for (int i = 0; i < 3; i ++) {
        fprintf(stdout, " THR \n");
        sleep(1);
    }

    thread_exit(125);
}

#define THREADS 5 
mtx_t mutex = {0};
char buff[4000];

void thr2(int* val) {
    int va = (int) val;
    char st[5];
    sprintf(st, "THR%i", va);
    mtx_lock(&mutex);
    for (int i = 0; i < 100; i ++) {
        strcat(buff, st);
        usleep(1000);
    }
    strcat(buff, " - ");

    mtx_unlock(&mutex);

    thread_exit(125);
}

int main(int argc, char** argv) {

    printf("ARGS %i\n", argc);
    for(int i = 0; i < argc; i ++) {
        printf("%i : %s \n", i, argv[i]);
    }

    char* testmem;
    for (int i = 0; i < 10; i ++) {
        testmem = malloc(120);
        memset(testmem, 0, 120);
        sprintf(testmem, "String in malloced memory %i %c", 12, 'a');
        printf("%s 0x%x 0x%p\n", testmem, testmem, testmem);
    }

    char bf[100];
    __dtostr(1.5, bf, 100, 10, 6, 0);

    char* a = malloc(10);
    printf("Mallocated to %p\n", a);
    char b[10];
    int nv;
    int xv;
    int neg;
    int oct;
    float fval, fval2;
    sscanf("123abab 456asas 7891011 FFFF -234 12 123.456 -768.333", "%s %s %i %x %i %o %f %f", a, b, &nv, &xv, &neg, &oct, &fval, &fval2);
    printf("1: %s 2: %s 3 NUM: %i 4: %i 5: %i 6: %i 7: %f 8: %f\n", a, b, nv, xv, neg, oct, fval, fval2);

    sscanf("   0xFFF, 0xDE - test 77", "   0x%x, 0x%x - test %o", &nv, &xv, &oct);
    printf("1: %i 2: %x 3: %o 4: %f 5: %G\n", nv, xv, oct, 123.123456, 232.90921);

    printf("PID : %i\n", getpid());

    int status = 0;

    char* args[3];
    args[0] = "ls.a";
    args[1] = "-a";
    args[2] = NULL;
    pid_t pid = 0;
    posix_spawn(&pid, "/ls.a", args);
    printf("CREATED PROCESS %i, errno = %i\n", pid, errno);
    pid_t rc = waitpid(pid, &status, 0);
    printf("PROCESS FINISHED WITH CODE %i, rc = %i, errno = %i\n", status, rc, errno);

    pid_t tpi = create_thread(thr1, NULL);
    printf("CREATED THREAD %i, errno = %i\n", tpi, errno);

    rc = waitpid(tpi, &status, 0);
    printf("THREAD FINISHED WITH CODE %i, rc = %i, errno = %i\n", status, rc, errno);

    struct meminfo pre, post;
    rc = KxGetMeminfo(&pre);

    pid_t pids[THREADS];
    for (int i = 0; i < THREADS; i ++) {
        pids[i] = create_thread(thr2, i);
    }

    for (int i = 0; i < THREADS; i ++) {
        rc = waitpid(pids[i], &status, 0);
    }

    rc = KxGetMeminfo(&post);

    printf("RESULT IS %s\n", buff);
    printf("pre_mem = %i, post_mem = %i\n", pre.mem_used, post.mem_used);

    FILE* tsf = fopen("bugaga.txt", "r");
    if (tsf == NULL) {
        printf("ERR: tsf null\n");
        return 1;
    }
    char* buffer = malloc(20);
    int readed = 0;
    while ((readed = fread(buffer, 1, 20, tsf)) > 0) {
        printf("FTELL IS %i\n", ftell(tsf));
        for (int i = 0; i < readed; i ++) {
            putchar(buffer[i]);
        }
    }
    fclose(tsf);
    tsf = fopen("bugaga.txt", "w");
    int written = fwrite("abab", 1, 4, tsf);
    fclose(tsf);
    printf("Written: %i bytes\n", written);

    int num1, num2;
    scanf("%i %i", &num1, &num2);
    printf("NUM1: %i, NUM2: %i\n", num1, num2);

    char sym = 'A';
    for(int iterations = 0; iterations < 5; iterations ++) {

        sleep(3);
        sym++;
        printf("From prog: %c\n", sym);
    }

    // Не терминируем для проверки fflush из atexit
    printf("Goodbye!");

    return 0;
}