qemu-system-aarch64 \
    -d int \
    -D qemu.log \
    -M virt \
    -cpu cortex-a57 \
    -smp 2 \
    -m 2G \
    -kernel kernel.elf \
    -fw_cfg name=opt/kairax.kernel,file=kernel.elf