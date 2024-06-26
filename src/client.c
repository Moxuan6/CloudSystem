#include "../include/myhead.h"
#include "../mydev/temp-hume/si7006.h"

void close_device(void)
{
    // 关闭led设备
    for (int led_num = 1; led_num <= 6; led_num++) {
        ioctl(led_fd, LED_OFF, &led_num);
    }

    // 关闭风扇
    if (write(fan_fd, "0", 1) < 0)
        PRINT_ERR("write error");

    // 关闭电机
    if (write(motor_fd, "0", 1) < 0)
        PRINT_ERR("write error");

    // 关闭设备文件描述符
    close(si7006_fd);
    close(ap3216_fd);
    close(led_fd);
    close(fan_fd);
    close(motor_fd);

    // 显示一条消息
    printf("Device file descriptor closed\n");

}

void handle_sigint(int sig)
{
    // 关闭设备
    close_device();
    // 退出程序
    exit(0);
}


/*主函数*/
int main(int argc, char const *argv[])
{
    // 注册SIGINT信号的处理函数
    signal(SIGINT, handle_sigint);

    if (argc != 3) {
        fprintf(stderr, "usage:%s <ip> <port>\n", argv[0]);
        exit(-1);
    }

    // 读取参考文件
    if(read_config(&msgarm) == -1)
        PRINT_ERR("read config error");

    // 打开设备节点
    if ((si7006_fd = open("/dev/si7006", O_RDWR)) == -1)
        PRINT_ERR("open si7006 device error");
    
    if ((ap3216_fd = open("/dev/ap3216", O_RDWR)) == -1)
        PRINT_ERR("open ap3216 device error");

    if ((led_fd = open("/dev/myled", O_RDWR)) == -1)
        PRINT_ERR("open led device error");

    if ((fan_fd = open("/dev/fan", O_RDWR)) == -1)
        PRINT_ERR("open fan device error");

    if((motor_fd = open("/dev/motor",O_RDWR))==-1)
        PRINT_ERR("open motor device error");

    struct sockaddr_in server_addr;
    sockfd = client_network_init(sockfd, &server_addr, argv[1], atoi(argv[2]));

    // 连接服务器
    if (-1 == connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr))) 
        PRINT_ERR("fail to connect");
  
    // 连接上位机
    strcpy(msgarm.user.username, "admin");
    strcpy(msgarm.user.password, "123456");

    // 发送登录信息
    if (-1 == send(sockfd, &msgarm, sizeof(msg_arm_t), 0)) 
        PRINT_ERR("fail to send");

    // 接收登录信息
    if (-1 == recv(sockfd, &msgarm, sizeof(msg_arm_t), 0)) 
        PRINT_ERR("fail to recv");

    if( 1 == msgarm.user.flags){
        puts("login success");
        /*创建维护环境线程*/
		pthread_create(&tid,NULL,hold_envthread,NULL);
        pthread_detach(tid); // 分离线程

        msg_arm_t threadbuf;
        
        /*等待用户下发指令，执行指令，返回结果*/
        while(1){
            memset(&msgarm,0,sizeof(msg_arm_t));
            // 接收上位机的指令
            if (0 == recv(sockfd, &msgarm, sizeof(msg_arm_t), 0)) {
                printf("Server closed the connection. Exiting.\n");
                close_device(); // 关闭设备
                exit(-1);
            }
            threadbuf = msgarm; // 保存数据
            switch (msgarm.commd) {
            case 1:
                /*获取环境数据*/
                if (pthread_create(&tid, NULL, getenv_thpread, NULL) != 0)
                    PRINT_ERR("fail to create thread");
                pthread_detach(tid);
                break;
            case 2:
                /*设置阈值*/
                if (pthread_create(&tid, NULL, setlimit_thread, &threadbuf) != 0)
                    PRINT_ERR("fail to create thread");
                pthread_detach(tid);
                break;
            case 3:
                /*控制设备*/
                if (pthread_create(&tid, NULL, ctrldev_thread, &threadbuf) != 0)
                    PRINT_ERR("fail to create thread");
                pthread_detach(tid);
                break;
            case 255:
                if(-1 == send(sockfd, &msgarm, sizeof(msg_arm_t), 0))
                    PRINT_ERR("fail to send");
                break;
            default:
                break;
            }
        }
    } else {
        printf("login fail\n");
        return -1;
    }
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
void *hold_envthread(void *argv)
{
    // 获取环境数据
    int tmp, hum;
    unsigned short als_data; // 光照传感器是16位ADC

    contdevstatus = 0x00;
    setflags = 1;

    while (1) {
        // 获取环境数据
        if (ioctl(si7006_fd, GET_TMP, &tmp) == -1)
            PRINT_ERR("ioctl error");
        if (ioctl(si7006_fd, GET_HUM, &hum) == -1)
            PRINT_ERR("ioctl error");
        if (read(ap3216_fd, &als_data, sizeof(als_data)) == -1)
            PRINT_ERR("read error");

        conthume = 125.0 * hum / 65536 - 6;
        conttemp = 175.72 * tmp / 65536 - 46.85;
        contlux = als_data * 0.0049;

        system("clear");
        printf("temp:%.2f hume:%.2f%% lux:%.2f\n",conttemp,conthume,contlux);

        if(setflags){
            // 判断温度是否在阈值范围内
            if(conttemp < settempup && conttemp > settempdown){
                fan_duty_cycle = 0;
                contdevstatus &= ~(0x3<<1);
            } else {
                fan_duty_cycle = 3;
                contdevstatus |= (0x3<<1);
            }

            // 判断湿度是否在阈值范围内
            if(conthume < sethumeup && conthume > sethumedown){
                motor_duty_cycle = 0;
                contdevstatus &= ~(0x01<<3);
            } else {
                motor_duty_cycle = 1;
                contdevstatus |= (0x01<<3);
            }

            // 判断光照是否在阈值范围内
            if(contlux < setluxup && contlux > setluxdown){
                for (int led_num = 1; led_num <= 6; led_num++) {
                    ioctl(led_fd, LED_OFF, &led_num);
                }
                contdevstatus &= ~(0x01<<0);
            } else {
                for (int led_num = 1; led_num <= 6; led_num++) {
                    ioctl(led_fd, LED_ON, &led_num);
                }
                contdevstatus |= (0x01<<0);
            }

            sprintf(fanbuf, "%d", fan_duty_cycle);
            if (write(fan_fd, fanbuf, sizeof(fanbuf)) < 0)
                PRINT_ERR("write error");
            
            sprintf(motorbuf, "%d", motor_duty_cycle);
            if (write(motor_fd, motorbuf, sizeof(motorbuf)) < 0)
                PRINT_ERR("write error");
        }
        // 休眠一段时间继续工作
        if(!setflags) {
            sleep(120);
            setflags = 1;
        } else {
            sleep(3);
        }
    }
}

/*获取环境数据线程*/
void *getenv_thpread(void *argv)
{
   	msg_arm_t buf;
    //读取设备数据，赋值给 buf.envdata成员
    buf.envdata.temp = conttemp;
    buf.envdata.hume = conthume;
    buf.envdata.lux = contlux;
    buf.envdata.devstatus = contdevstatus;

    buf.limitset.tempup = settempup;
    buf.limitset.tempdown = settempdown;
    buf.limitset.humeup = sethumeup;
    buf.limitset.humedown = sethumedown;
    buf.limitset.luxup = setluxup;
    buf.limitset.luxdown = setluxdown;

    //返回环境数据结果
    buf.user.flags = 1;
    if (-1 == send(sockfd, &buf, sizeof(msg_arm_t), 0)) 
        PRINT_ERR("fail to send");

    pthread_exit(NULL);
}

/*设置阈值线程*/
void *setlimit_thread(void *argv)
{
    msg_arm_t buf = *(msg_arm_t *)argv;
    setflags = 1;

    //将 buf.limitset字段中的数据赋值给参考变量
    settempup = buf.limitset.tempup;
    settempdown = buf.limitset.tempdown;

    sethumeup = buf.limitset.humeup;
    sethumedown = buf.limitset.humedown;

    setluxup = buf.limitset.luxup;
    setluxdown = buf.limitset.luxdown;

    printf("set config : temp{%.2f,%.2f} hume{%.2f,%.2f} lux{%.2f,%.2f}\n",settempup,settempdown,sethumeup,sethumedown,setluxup
        ,setluxdown);

    //将参考变量数据写入到文件中
    FILE *fp = fopen("init.txt", "w");
    if (fp == NULL) PRINT_ERR("fail to fopen");

    fprintf(fp, "tempup=%.2f\n", settempup);
    fprintf(fp, "tempdown=%.2f\n", settempdown);
    fprintf(fp, "humeup=%.2f\n", sethumeup);
    fprintf(fp, "humedown=%.2f\n", sethumedown);
    fprintf(fp, "luxup=%.2f\n", setluxup);
    fprintf(fp, "luxdown=%.2f\n", setluxdown);

    fclose(fp);

	//将执行结果返回给服务器
    buf.user.flags = 1;
    if (-1 == send(sockfd, &buf, sizeof(msg_arm_t), 0)) 
        PRINT_ERR("fail to send");
    puts("set limit success");
    pthread_exit(NULL);
}

/*控制设备线程*/
void *ctrldev_thread(void *argv)
{
    msg_arm_t buf = *(msg_arm_t *)argv;
    setflags = 0;

    //根据 buf 中devctrl 字段对应的位，控制设备的启停操作
    // 灯光控制
   if(buf.devctrl&(0x01<<0)){
        for (int led_num = 1; led_num <= 6; led_num++) {
            ioctl(led_fd, LED_ON, &led_num);
        }
        contdevstatus |= (0x01<<0);
	}else{
        for (int led_num = 1; led_num <= 6; led_num++) {
            ioctl(led_fd, LED_OFF, &led_num);
        }
		contdevstatus &= ~(0x01<<0);
	}

    // 马达控制
	if(0x08 == (buf.devctrl&(0x01<<3))){
        motor_duty_cycle = 1;
		contdevstatus |= (0x01<<3);
	}else{
		motor_duty_cycle = 0; 
		contdevstatus &= ~(0x01<<3);
	}

    // 风扇控制
	if(0x02 == (buf.devctrl&(0x03<<1))){
		puts("1");
		fan_duty_cycle = 1;
		contdevstatus &= ~(0x03<<1);
		contdevstatus |= (0x01<<1);
	}else if(0x04==(buf.devctrl&(0x03<<1))){
		puts("2");
		fan_duty_cycle = 2;
		contdevstatus &= ~(0x03<<1);
		contdevstatus |= (0x02<<1);
	}else if(0x06==(buf.devctrl&(0x03<<1))){
		puts("3");
		fan_duty_cycle = 3;
		contdevstatus &= ~(0x03<<1);
		contdevstatus |= (0x03<<1);
	}else{
		fan_duty_cycle = 0;
		contdevstatus &= ~(0x03<<1);
	}

    sprintf(fanbuf, "%d", fan_duty_cycle);
    if (write(fan_fd, fanbuf, sizeof(fanbuf)) < 0)
        PRINT_ERR("write error");

    sprintf(motorbuf, "%d", motor_duty_cycle);
    if(write(motor_fd, motorbuf, sizeof(motorbuf)) < 0)
        PRINT_ERR("write error");

    // 返回执行结果
    buf.user.flags = 1;
    if (-1 == send(sockfd, &buf, sizeof(msg_arm_t), 0)) 
        PRINT_ERR("fail to send");
    puts("ctrl dev success");
    
    pthread_exit(NULL);
}

int read_config(msg_arm_t *armbuf)
{
    // 读取参考文件数据赋值给参考变量
    FILE *fp = fopen("init.txt", "r");
    if (fp == NULL) PRINT_ERR("fail to fopen");

    // fscanf(文件指针, "格式化字符串", 参数列表), %hhu 无符号字符
    fscanf(fp, "tempup=%f\n", &armbuf->limitset.tempup);
    fscanf(fp, "tempdown=%f\n", &armbuf->limitset.tempdown);
    fscanf(fp, "humeup=%f\n", &armbuf->limitset.humeup);
    fscanf(fp, "humedown=%f\n", &armbuf->limitset.humedown);
    fscanf(fp, "luxup=%f\n", &armbuf->limitset.luxup);
    fscanf(fp, "luxdown=%f\n", &armbuf->limitset.luxdown);

    fclose(fp);

    settempup = armbuf->limitset.tempup;
    settempdown = armbuf->limitset.tempdown;
    sethumeup = armbuf->limitset.humeup;
    sethumedown = armbuf->limitset.humedown;
    setluxup = armbuf->limitset.luxup;
    setluxdown = armbuf->limitset.luxdown;
    printf("open config : temp{%.2f,%.2f} hume{%.2f,%.2f} lux{%.2f,%.2f}\n",settempup,settempdown,sethumeup,sethumedown,setluxup
    ,setluxdown);
    puts("read config success");
    
    return 0;
}