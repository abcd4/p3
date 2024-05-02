main: checkdisk.c raid4disk.c main.c ramdisk.c block_store.h
	gcc -o main -I. -DHW_FS main.c checkdisk.c raid4disk.c ramdisk.c

grade: checkdisk.c raid4disk.c grade.c ramdisk.c block_store.h
	gcc -o grade -I. -DHW_FS grade.c checkdisk.c raid4disk.c ramdisk.c

clean:
	rm main
