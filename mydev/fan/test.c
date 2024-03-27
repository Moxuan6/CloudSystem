#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main(void)
{
    int fd;
    int duty_cycle;

    // 打开设备文件
    fd = open("/dev/fan", O_WRONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    // 设置占空比
    printf("Enter duty cycle (0-3): ");
    scanf("%d", &duty_cycle);

    // 将占空比写入设备文件
    char buf[10];
    sprintf(buf, "%d", duty_cycle);
    if (write(fd, buf, sizeof(buf)) < 0) {
        perror("write");
        close(fd);
        return 1;
    }

    // 关闭设备文件
    close(fd);

    return 0;
}