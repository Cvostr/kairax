__thread int errno;

int* __errno_location() {
    return &errno;
}