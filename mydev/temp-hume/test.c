#include "si7006.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define PRINT_ERR(msg) do { perror(msg); exit(1); } while (0)

int main(int argc, const char* argv[])
{
    int fd;
    int serial, firmware, tmp, hum;
    float tmp_f, hum_f;
    if ((fd = open("/dev/si7006", O_RDWR)) == -1)
        PRINT_ERR("open error");
        
    while (1) {
        if (ioctl(fd, GET_TMP, &tmp) == -1)
            PRINT_ERR("ioctl error");
        if (ioctl(fd, GET_HUM, &hum) == -1)
            PRINT_ERR("ioctl error");
        hum_f = 125.0 * hum / 65536 - 6;
        tmp_f = 175.72 * tmp / 65536 - 46.85;

        printf("tmp = %.2f,hum = %.2f\r", tmp_f, hum_f);
        fflush(stdout);
        usleep(500);
    }
    close(fd);
    return 0;
}