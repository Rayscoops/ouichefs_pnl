#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "requettes.h"
#include <linux/ioctl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SIZE  1000


int main(int arv, char **argv)
{
    int fd = open("/dev/ouichefs_ioctl", O_RDWR);
    long ret_val = ioctl(fd, CHANGE_VERSION, "2");
    return 0;
}