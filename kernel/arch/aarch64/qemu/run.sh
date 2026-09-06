qemu-system-aarch64 \
    -d int \
    -M virt \
    -cpu cortex-a57 \
    -smp 2 \
    -m 2G \
    -kernel kernel.elf