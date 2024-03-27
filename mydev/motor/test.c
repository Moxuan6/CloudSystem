#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define PRINT_ERR(msg) do { perror(msg); exit(1); } while (0)

int main(int argc,const char * argv[])
{
    int fd;
    if((fd = open("/dev/motor",O_RDWR))==-1)
        PRINT_ERR("open error");
    sleep(3);
    close(fd);
    return 0;
}