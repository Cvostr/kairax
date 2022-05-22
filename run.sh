qemu-system-x86_64 -m 2G -cdrom os.iso -d int \
			-drive id=disk,file=drive.img,if=none,format=raw \
 			-device ahci,id=ahci \
			-device nvme,serial=deadbeef \
 			-device ide-hd,drive=disk,bus=ahci.0 \
			-no-reboot \
			-no-shutdown      
