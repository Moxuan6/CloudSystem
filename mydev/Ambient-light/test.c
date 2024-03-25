#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define PRINT_ERR(msg) do { perror(msg); exit(1); } while (0)

int main(int argc,const char * argv[])
{
    int fd;
    unsigned short als_data; // 光照传感器是16位ADC

    // 将ALS ADC数据转换为Lux单位, ALS Gain 4 0.0049
    float lux, resolution =  0.0049;

    if((fd = open("/dev/ap3216",O_RDWR))==-1)
        PRINT_ERR("open error");

    while(1){
        read(fd, &als_data, sizeof(als_data));
        printf("ALS data: %d\n", als_data);
        lux = als_data * resolution;
        printf("ALS data: %f lux\n", lux); // 打印环境光数据
        sleep(1); // 每秒读取一次
    }

    close(fd);
    return 0;
}