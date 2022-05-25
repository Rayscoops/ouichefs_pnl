obj-m += ouichefs.o
ouichefs-objs := fs.o super.o inode.o file.o dir.o

.ONESHELL:

KERNELDIR ?= /home/riham/Bureau/pnl/linux-5.10.17
CHECK_PATCH=./checkpatch.pl --no-tree

all:
	make -C $(KERNELDIR) M=$(PWD) modules 

debug:
	make -C $(KERNELDIR) M=$(PWD) ccflags-y+="-DDEBUG -g" modules


check:
	for f in *.c *.h ; do
		$(CHECK_PATCH) -f $$f
	done

clean:
	make -C $(KERNELDIR) M=$(PWD) clean
	rm -rf *~

.PHONY: all clean
