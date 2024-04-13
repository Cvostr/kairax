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
char* testfile2 = "testdir/testfile2.txt";
#define TESTDIR_PERM 0666
#define TESTFILE_PERM 0667

#define WRITE_PATTERN_ROUNDS 128
#define PATTERN_SIZE 256
char pattern[PATTERN_SIZE];

int main(int argc, char** argv)
{
    struct stat file_stat;
    int fd, rc;
    int old_links;

    rc = stat(".", &file_stat);
    old_links = file_stat.st_nlink;

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
    if (file_stat.st_nlink != 2) {
        printf("Invalid hard link count for %s, expected %i, got %i\n", testdir, 2, file_stat.st_nlink);
        return 4;
    }
    rc = stat(".", &file_stat);
    if (file_stat.st_nlink != (old_links + 1)) {
        printf("Invalid hard link count for %s, expected %i, got %i\n", testdir, (old_links + 1), file_stat.st_nlink);
        return 5;
    }
    close(fd);

    printf("Test 2: File creation\n");
    fd = creat(testfile, TESTFILE_PERM);
    if (fd == -1) {
        printf("Failed creat() for %s\n", testfile);
        return 5;
    }
    rc = stat(testfile, &file_stat);
    if (rc == -1) {
        printf("Failed stat() for %s, errno = %i\n", testfile, errno);
        return 6;
    }
    perm = file_stat.st_mode & 0777;
    if (perm != TESTFILE_PERM) {
        printf("Invalid perm for %s, expected %i, got %i\n", testfile, TESTFILE_PERM, perm);
        return 7;
    }

    close(fd);

    printf("Test 3: File write\n");
    fd = open(testfile, O_RDWR, 0);
    if (fd == -1) {
        printf("Failed open() for %s\n", testfile);
        return 8;
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
        return 9;
    }
    stat(testfile, &file_stat);
    if (total_len != file_stat.st_size) {
        printf("Incorrect file size, expected %i, got %i\n", total_len, file_stat.st_size);
        return 10;
    }

    printf("Test 4: File read\n");
    lseek(fd, 0, SEEK_SET);
    char testbuf[PATTERN_SIZE];
    for (int i = 0; i < WRITE_PATTERN_ROUNDS; i ++) {

        rc = read(fd, testbuf, PATTERN_SIZE);
        if (rc == -1) {
            printf("Failed read() for %s, errno = %i\n", testfile, errno);
            return 11;
        }
        if (rc != PATTERN_SIZE) {
            printf("Failed read() for %s: incorrect result, expected %i, got %i\n", testfile, PATTERN_SIZE, rc);
            return 12;
        }
        
        for (int j = 0; j < 256; j ++) {
            if (pattern[j] != testbuf[j]) {
                printf("Incorect data in file at pos %i\n", i * WRITE_PATTERN_ROUNDS + j);
                return 13;
            }
        }
    }

    printf("Test 5: File rename\n");
    rc = rename(testfile, testfile2);
    if (rc == -1) {
        printf("Failed rename() from %s to %s, errno = %i\n", testfile, testfile2, errno);
        return 14;
    }
    rc = stat(testfile, &file_stat);
    if (rc != -1) {
        printf("Successful stat() of %s, Something wrong\n", testfile);
        return 15;
    }
    if (errno != ENOENT) {
        printf("Incorrect errno(), expected %i, got %i\n", ENOENT, errno);
        return 16;
    }
    rc = stat(testfile2, &file_stat);
    if (rc == -1) {
        printf("Failed stat() for %s, errno = %i\n", testfile2, errno);
        return 17;
    }

    printf("Test 6: File remove\n");
    rc = remove(testfile2);
    if (rc == -1) {
        printf("Failed remove() for %s, errno = %i\n", testfile2, errno);
        return 18;
    }
    rc = stat(testfile2, &file_stat);
    if (rc != -1) {
        printf("Successful stat() of %s, Something wrong\n", testfile2);
        return 19;
    }
    if (errno != ENOENT) {
        printf("Incorrect errno, expected %i, got %i\n", ENOENT, errno);
        return 20;
    }
    printf("\tClosing fd %i\n", fd);
    close(fd);

    return 0;
}