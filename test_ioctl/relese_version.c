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


int main(int arv, char ** argv)
{
    int fd = open ("../partition/partition_ouichefs/toto", O_RDWR);
    long ret_val = ioctl(fd, RLEASE_VERSION, "3");
    return 0;
}