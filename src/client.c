#include "../include/myhead.h"

#include "../mydev/temp-hume/si7006.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/errno.h>

#define PRINT_ERR(msg) do { perror(msg); exit(1); } while (0)

/*根据用户设置的阈值维护恒定环境线程*/
void *hold_envthread(void *argv);

/*获取环境数据线程*/
void *getenv_thpread(void *argv);

/*设置阈值线程*/
void *setlimit_thread(void *argv);

/*控制设备线程*/
void *ctrldev_thread(void *argv);

/*初始化网络*/
int client_network_init(int sockfd, struct sockaddr_in *addr, const char *ip, int port);

/*主函数*/
int main(int argc, char const *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "usage:%s <ip> <port>\n", argv[0]);
        exit(-1);
    }

    int sockfd = 0;
    struct sockaddr_in server_addr;
    sockfd = client_network_init(sockfd, &server_addr, argv[1], atoi(argv[2]));

    // 连接服务器
    if (-1 == connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr))) {
        perror("fail to connect");
        exit(-1);
    }

    // 连接上位机
    strcpy(msg.user.username, "admin");
    strcpy(msg.user.password, "123456");

    // 发送登录信息
    if (-1 == send(sockfd, &msg, sizeof(msg), 0)) {
        perror("fail to send");
        exit(-1);
    }

    // 接收登录信息
    if (-1 == recv(sockfd, &msg, sizeof(msg), 0)) {
        perror("fail to recv");
        exit(-1);
    }

    conttemp = 23.15;
    conthume = 45;
    contlux = 100;

    // 判断是否登录成功
    if(1 == msg.user.flags){
        printf("login success\n");
    } else {
        printf("login fail\n");
        return -1;
    }

    if( 1 == msg.user.flags){
        /*创建维护环境线程*/
		pthread_create(&tid,NULL,hold_envthread,NULL);

        /*等待用户下发指令，执行指令，返回结果*/
        while(1){
            memset(&msg,0,sizeof(msg));
            
            // 接收上位机的指令
            if (0 == recv(sockfd, &msg, sizeof(msg), 0)) {
                printf("Server closed the connection. Exiting.\n");
                exit(-1);
            }

            switch(msg.commd){
                case 1:
                    /*获取环境数据*/
                    puts("获取环境数据");
                    if (pthread_create(&tid, NULL, getenv_thpread, &sockfd) != 0) {
                        perror("fail to create thread");
                        exit(-1);
                    }
                    break;
                case 2:
                    /*设置阈值*/
                    puts("设置阈值");
                    if (pthread_create(&tid, NULL, setlimit_thread, &sockfd) != 0) {
                        perror("fail to create thread");
                        exit(-1);
                    }
                    break;
                case 3:
                    /*控制设备*/
                    puts("控制设备");
                    if (pthread_create(&tid, NULL, ctrldev_thread, &sockfd) != 0) {
                        perror("fail to create thread");
                        exit(-1);
                    }
                    break;
                default:
                    break;
            }
        }
    } else {
        printf("login fail\n");
    }

    close(sockfd);
    return 0;
}


/*初始化网络*/
int client_network_init(int sockfd, struct sockaddr_in *addr, const char *ip, int port)
{
    // 创建套接字
    if (-1 == (sockfd = socket(AF_INET, SOCK_STREAM, 0))) {
        perror("fail to socket");
        exit(-1);
    }

    // 填充网络信息结构体
    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    addr->sin_addr.s_addr = inet_addr(ip);

    return sockfd;
}

/*根据用户设置的阈值维护恒定环境线程*/
void *hold_envthread(void *arg)
{
    msg_t buf;
    // 获取环境数据
    int fd, tmp, hum;
    float tmp_f, hum_f;
    unsigned short als_data; // 光照传感器是16位ADC
    float lux, resolution = 0.0049;
    int led_num;


    // 读取参考文件数据赋值给参考变量
    FILE *fp = fopen("init.txt", "r");
    if (fp == NULL) {
        perror("fail to fopen");
        exit(-1);
    }

    // fscanf(文件指针, "格式化字符串", 参数列表), %hhu 无符号字符
    fscanf(fp, "tempup=%f\n", &settempup);
    fscanf(fp, "tempdown=%f\n", &settempdown);
    fscanf(fp, "humeup=%f\n", &sethumeup);
    fscanf(fp, "humedown=%f\n", &sethumedown);
    fscanf(fp, "luxup=%f\n", &setluxup);
    fscanf(fp, "luxdown=%f\n", &setluxdown);

    fclose(fp);

    // 打印参考变量数据
    printf("温度上限=%.2f 温度下限=%.2f 湿度上限=%.2f 湿度下限=%.2f 光照上限=%.2f 光照下限=%.2f\n",
           settempup, settempdown, sethumeup, sethumedown, setluxup, setluxdown);


    // 初始化设备状态为关闭
    int fan_status = 0; // 风扇状态，0表示关闭，1表示开启
    int buzzer_status = 0; // 蜂鸣器状态，0表示关闭，1表示开启
    int led_status = 0; // LED状态，0表示关闭，1表示开启

    while (1) {
        
        // 获取环境数据
        if ((fd = open("/dev/si7006", O_RDWR)) == -1)
            PRINT_ERR("open error");
        if (ioctl(fd, GET_TMP, &tmp) == -1)
            PRINT_ERR("ioctl error");
        if (ioctl(fd, GET_HUM, &hum) == -1)
            PRINT_ERR("ioctl error");

        close(fd);

        hum_f = 125.0 * hum / 65536 - 6;
        tmp_f = 175.72 * tmp / 65536 - 46.85;

        if ((fd = open("/dev/ap3216", O_RDWR)) == -1)
            PRINT_ERR("open error");

        read(fd, &als_data, sizeof(als_data));
        lux = als_data * resolution;

        close(fd);

        // 将获取到的环境数据赋值给缓存变量
        conttemp = tmp_f;
        conthume = hum_f;
        contlux = lux;

        if ((fd = open("/dev/myled", O_RDWR)) == -1) {
            perror("open error");
            exit(1);
        }

        // 根据环境数据与参考值比较，决定是否开启对应的设备
        // 判断温度是否在设定范围内
        if (conttemp >= settempdown && conttemp <= settempup) {
            if (fan_status != 0) {
                fan_status = 0;
                // 关闭风扇设备
                // buf.envdata.devstatus &= ~0x02;
                printf("温度在设定范围内，风扇关闭\n");
            }
        } else {
            if (fan_status != 1) {
                fan_status = 1;
                // 开启风扇设备
                // buf.envdata.devstatus |= 0x02;
                printf("温度超出设定范围，风扇开启\n");
            }
        }

        // 判断湿度是否在设定范围内
        if (conthume >= sethumedown && conthume <= sethumeup) {
            if(buzzer_status != 0){
                buzzer_status = 0;
                // 关闭蜂鸣器设备
                // buf.envdata.devstatus &= ~0x08;
                printf("湿度在设定范围内，蜂鸣器关闭\n");
            }
        } else {
            if(buzzer_status != 1){
                buzzer_status = 1;
                // 开启蜂鸣器设备
                // buf.envdata.devstatus |= 0x08;
                printf("湿度超出设定范围，蜂鸣器开启\n");
            }
        }

        // 判断光照是否在设定范围内
        if (contlux >= setluxdown && contlux <= setluxup) {
            if(led_status != 0){
                led_status = 0;
                // 关闭LED设备
                // buf.envdata.devstatus &= ~0x01;
                printf("光照在设定范围内，LED关闭\n");
                for (led_num = 1; led_num <= 6; led_num++) {
                    ioctl(fd, LED_OFF, &led_num);
                }
            }
        } else {
            if(led_status != 1){
                led_status = 1;
                // 开启LED设备
                // buf.envdata.devstatus |= 0x01;
                printf("光照超出设定范围，LED开启\n");
                for (led_num = 1; led_num <= 6; led_num++) {
                    ioctl(fd, LED_ON, &led_num);
                }
            }
        }
        // 休眠一段时间继续工作
        sleep(5);
    }
}

/*获取环境数据线程*/
void *getenv_thpread(void *arg)
{
   	msg_t buf;
    int sockfd = *(int *)arg;

    int fd, tmp, hum;
    float tmp_f, hum_f;
    unsigned short als_data; // 光照传感器是16位ADC
    float lux, resolution =  0.0049;

    if ((fd = open("/dev/si7006", O_RDWR)) == -1)
        PRINT_ERR("open error");
    if (ioctl(fd, GET_TMP, &tmp) == -1)
        PRINT_ERR("ioctl error");
    if (ioctl(fd, GET_HUM, &hum) == -1)
        PRINT_ERR("ioctl error");

    close(fd);

    hum_f = 125.0 * hum / 65536 - 6;
    tmp_f = 175.72 * tmp / 65536 - 46.85;

    if((fd = open("/dev/ap3216",O_RDWR)) == -1)
        PRINT_ERR("open error");

    read(fd, &als_data, sizeof(als_data));
    lux = als_data * resolution;

    close(fd);

    // 将获取到的环境数据赋值给buf.envdata
    buf.envdata.temp = tmp_f;
    buf.envdata.hume = hum_f;
    buf.envdata.lux = lux;

    // 将数据存到缓存变量中
    conttemp = buf.envdata.temp;
    conthume = buf.envdata.hume;
    contlux = buf.envdata.lux;
    
    buf.envdata.devstatus = 0x01 | 0x02 | 0x08; // 0000 0001 | 0000 0010 | 0000 1000 = 0000 1011

    //返回环境数据结果
    if (-1 == send(sockfd, &buf, sizeof(buf), 0)) {
        perror("fail to send");
        exit(-1);
    }

    pthread_exit(NULL);
}

/*设置阈值线程*/
void *setlimit_thread(void *arg)
{
    msg_t buf;
    int sockfd = *(int *)arg;
	//将 buf.limitset字段中的数据赋值给参考变量

    settempup = buf.limitset.tempup;
    settempdown = buf.limitset.tempdown;
    sethumeup = buf.limitset.humeup;
    sethumedown = buf.limitset.humedown;
    setluxup = buf.limitset.luxup;
    setluxdown = buf.limitset.luxdown;
    
    printf("温度上限=%.2f 温度下限=%.2f 湿度上限=%.2f 湿度下限=%.2f 光照上限=%.2f 光照下限=%.2f\n", 
        settempup, settempdown, sethumeup, sethumedown, setluxup, setluxdown);

    //将参考变量数据写入到文件中
    FILE *fp = fopen("init.txt", "w");
    if (fp == NULL) {
        perror("fail to fopen");
        exit(-1);
    }

    fprintf(fp, "tempup=%f\n", settempup);
    fprintf(fp, "tempdown=%f\n", settempdown);
    fprintf(fp, "humeup=%f\n", sethumeup);
    fprintf(fp, "humedown=%f\n", sethumedown);
    fprintf(fp, "luxup=%f\n", setluxup);
    fprintf(fp, "luxdown=%f\n", setluxdown);

    printf("温度上限=%.2f 温度下限=%.2f 湿度上限=%.2f 湿度下限=%.2f 光照上限=%.2f 光照下限=%.2f\n", 
        settempup, settempdown, sethumeup, sethumedown, setluxup, setluxdown);

	//将执行结果返回给服务器
    if (-1 == send(sockfd, &buf, sizeof(buf), 0)) {
        perror("fail to send");
        exit(-1);
    }

    fclose(fp);
    pthread_exit(NULL);
}

/*控制设备线程*/
void *ctrldev_thread(void *arg)
{
    msg_t buf;
    int sockfd = *(int *)arg;

    if(buf.devctrl & 0x01){
        //开启LED设备
        buf.envdata.devstatus |= 0x01;
        printf("LED开启\n");
    }else{
        //关闭LED设备
        buf.envdata.devstatus &= ~0x01;
        printf("LED关闭\n");
    }

    if( 0x06 == (buf.devctrl & (0x3 << 1))){
        //开启风扇设备
        buf.envdata.devstatus |= 0x02;
        printf("风扇开启\n");
    }else{
        //关闭风扇设备
        buf.envdata.devstatus &= ~0x02;
        printf("风扇关闭\n");
    }

    if(buf.devctrl & (0x1 << 3)){
        //开启蜂鸣器设备
        buf.envdata.devstatus |= 0x08;
        printf("蜂鸣器开启\n");
    }else{
        //关闭蜂鸣器设备
        buf.envdata.devstatus &= ~0x08;
        printf("蜂鸣器关闭\n");
    }

    // 返回执行结果
    if (-1 == send(sockfd, &buf, sizeof(buf), 0)) {
        perror("fail to send");
        exit(-1);
    }

    pthread_exit(NULL);
}