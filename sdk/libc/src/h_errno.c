__thread int h_errno;

int* __h_errno_location() {
    return &h_errno;
}