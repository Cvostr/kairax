#include "stdio.h"
#include "process.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "sys/wait.h"
#include "sys/stat.h"
#include "errno.h"
#include "spawn.h"
#include "threads.h"
#include "fcntl.h"

char* testdir = "testdir";
char* testfile = "testdir/testfile.txt";
#define TESTDIR_PERM 0666
#define TESTFILE_PERM 0667

#define WRITE_PATTERN_ROUNDS 128
#define PATTERN_SIZE 256
char pattern[PATTERN_SIZE];

int main(int argc, char** argv)
{
    struct stat file_stat;
    int fd, rc;

    printf("Test 1: Dir creation\n");
    fd = mkdir(testdir, TESTDIR_PERM);
    if (fd == -1) {
        printf("Failed mkdir() for %s\n", testdir);
        return 1;
    }
    rc = stat(testdir, &file_stat);
    if (rc == -1) {
        printf("Failed stat() for %s\n", testdir);
        return 2;
    }
    int perm = file_stat.st_mode & 0777;
    if (perm != TESTDIR_PERM) {
        printf("Invalid perm for %s, expected %i, got %i\n", testdir, TESTDIR_PERM, perm);
        return 3;
    }
    close(fd);

    printf("Test 2: File creation\n");
    fd = creat(testfile, TESTFILE_PERM);
    if (fd == -1) {
        printf("Failed creat() for %s\n", testfile);
        return 4;
    }
    rc = stat(testfile, &file_stat);
    if (rc == -1) {
        printf("Failed stat() for %s, errno = %i\n", testfile, errno);
        return 5;
    }
    perm = file_stat.st_mode & 0777;
    if (perm != TESTFILE_PERM) {
        printf("Invalid perm for %s, expected %i, got %i\n", testfile, TESTFILE_PERM, perm);
        return 6;
    }

    close(fd);

    printf("Test 3: File write\n");
    fd = open(testfile, O_RDWR, 0);
    if (fd == -1) {
        printf("Failed open() for %s\n", testfile);
        return 7;
    }

    for (int j = 0; j < PATTERN_SIZE; j ++) {
        pattern[j] = j;
    }
    int total_len = WRITE_PATTERN_ROUNDS * PATTERN_SIZE;
    for (int i = 0; i < WRITE_PATTERN_ROUNDS; i ++) {
        rc = write(fd, pattern, PATTERN_SIZE);
    }
    int pos = lseek(fd, 0, SEEK_CUR);
    if (pos != total_len) {
        printf("Incorrect pos, expected %i, got %i\n", total_len, pos);
        return 8;
    }

    printf("Test 4: File read\n");
    lseek(fd, 0, SEEK_SET);
    char testbuf[PATTERN_SIZE];
    for (int i = 0; i < WRITE_PATTERN_ROUNDS; i ++) {

        rc = read(fd, testbuf, PATTERN_SIZE);
        if (rc == -1) {
            printf("Failed read() for %s, errno = %i\n", testfile, errno);
            return 9;
        }
        if (rc != PATTERN_SIZE) {
            printf("Failed read() for %s: incorrect result, expected %i, got %i\n", testfile, PATTERN_SIZE, rc);
            return 10;
        }
        
        for (int j = 0; j < 256; j ++) {
            if (pattern[j] != testbuf[j]) {
                printf("Incorect data in file at pos %i\n", i * WRITE_PATTERN_ROUNDS + j);
                return 11;
            }
        }
    }

    printf("Test 5: File remove\n");
    rc = remove(testfile);
    if (rc == -1) {
        printf("Failed remove() for %s, errno = %i\n", testfile, errno);
        return 12;
    }

    return 0;
}