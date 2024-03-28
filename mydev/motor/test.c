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


    if (write(fd, "1", 1) < 0)  // 启动电机
        PRINT_ERR("write error");

    sleep(3);

    if (write(fd, "0", 1) < 0)  // 关闭电机
        PRINT_ERR("write error");
    close(fd);
    return 0;
}