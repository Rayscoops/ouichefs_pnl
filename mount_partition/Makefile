
all:
	insmod ../ouichefs.ko
	mount -oloop -t ouichefs ../mkfs/test.img partition_ouichefs/ 
	
clean:
	umount partition_ouichefs/
	rmmod ouichefs.ko
	

.PHONY: all 
