"C:\Program Files\qemu\qemu-system-x86_64w" -monitor stdio -m 1G -d int -smp 4 ^
-drive id=disk,file=drive.img,if=none,format=raw ^
-drive id=bootdisk,file=kairax.iso,if=none ^
-device ahci,id=ahci ^
-device nvme,serial=deadbeef ^
-device ide-hd,drive=disk,bus=ahci.0,bootindex=4 ^
-device ide-hd,drive=bootdisk,bus=ahci.1,bootindex=0 ^
-device intel-hda -device hda-duplex ^
-no-reboot -no-shutdown      
