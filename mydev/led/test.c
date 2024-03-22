#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "led.h"

#define PRINT_ERR(msg) do { perror(msg); exit(1); } while (0)

void led_blink(int fd, int which)
{
    ioctl(fd, LED_ON, &which);
    sleep(1);
    ioctl(fd, LED_OFF, &which);
    sleep(1);
}

int main(int argc, const char* argv[])
{
    int fd;
    if ((fd = open("/dev/myled", O_RDWR)) == -1)
        PRINT_ERR("open error");

    while (1) {
        for (int i = CLED1; i <= ELED6; i++)
            led_blink(fd, i);
    }
    close(fd);
    return 0;
}