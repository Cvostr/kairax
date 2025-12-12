#include "sys/mman.h"
#include "unistd.h"
#include "stdlib.h"
#include "fcntl.h"
#include "stdio.h"
#include "errno.h"

#define SHM_NAME "/my_shared_memory"
#define SHM_SIZE 4096 // 4KB

void _writer()
{
 int shm_fd;
    void *ptr;
    const char *message = "Hello from shared memory!";

    // Create and open the shared memory object
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    printf("shm_fd = %i\n", shm_fd);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    // Set the size of the shared memory object
    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        printf("errno %i\n", errno);
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    // Map the shared memory object into the process's address space
    ptr = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Write data to shared memory
    sprintf(ptr, "%s", message);
    printf("Writer wrote: \"%s\" to shared memory.\n", (char *)ptr);

    // Close the shared memory file descriptor (mapping remains)
    close(shm_fd);

    // Unmap the shared memory
    if (munmap(ptr, SHM_SIZE) == -1) {
        perror("munmap");
        exit(EXIT_FAILURE);
    }

    // Unlink the shared memory object (removes it from the system)
    // This is typically done by one of the processes when shared memory is no longer needed.
    // shm_unlink(SHM_NAME); // Uncomment if you want the writer to unlink it

    return 0;
}

void reader()
{
    int shm_fd;
    void *ptr;
    sleep(1);

    // Open the existing shared memory object
    shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    // Map the shared memory object into the process's address space
    ptr = mmap(0, SHM_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Read data from shared memory
    printf("Reader read: \"%s\" from shared memory.\n", (char *)ptr);

    // Close the shared memory file descriptor
    close(shm_fd);

    // Unmap the shared memory
    if (munmap(ptr, SHM_SIZE) == -1) {
        perror("munmap");
        exit(EXIT_FAILURE);
    }

    // Unlink the shared memory object (removes it from the system)
    // This is typically done by one of the processes when shared memory is no longer needed.
    shm_unlink(SHM_NAME); // Reader unlinks it in this example

    return 0;
}

int main(int argc, char** argv) 
{
    pid_t r = fork();
    if (r == 0) {
        reader();
    } else if (r > 0) {
        _writer();
        int status;
        waitpid(r, &status, 0);
    } else {
        perror("fork()");
    }
}