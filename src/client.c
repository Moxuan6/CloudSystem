#include "../include/myhead.h"
#include "../mydev/temp-hume/si7006.h"

/*主函数*/
int main(int argc, char const *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "usage:%s <ip> <port>\n", argv[0]);
        exit(-1);
    }

    // 读取参考文件
    read_config();

    // 打开设备节点
    if ((si7006_fd = open("/dev/si7006", O_RDWR)) == -1)
        PRINT_ERR("打开si7006设备节点失败");
    
    if ((ap3216_fd = open("/dev/ap3216", O_RDWR)) == -1)
        PRINT_ERR("打开ap3216设备节点失败");

    if ((led_fd = open("/dev/myled", O_RDWR)) == -1)
        PRINT_ERR("打开led设备节点失败");

    if ((fan_fd = open("/dev/fan", O_RDWR)) == -1)
        PRINT_ERR("打开fan设备节点失败");

    struct sockaddr_in server_addr;
    sockfd = client_network_init(sockfd, &server_addr, argv[1], atoi(argv[2]));

    // 连接服务器
    if (-1 == connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr))) {
        perror("fail to connect");
        exit(-1);
    }

    // 连接上位机
    strcpy(msgarm.user.username, "admin");
    strcpy(msgarm.user.password, "123456");

    // 发送登录信息
    if (-1 == send(sockfd, &msgarm, sizeof(msg_arm_t), 0)) {
        perror("fail to send");
        exit(-1);
    }

    // 接收登录信息
    if (-1 == recv(sockfd, &msgarm, sizeof(msg_arm_t), 0)) {
        perror("fail to recv");
        exit(-1);
    }

    if( 1 == msgarm.user.flags){

        puts("login success");
        /*创建维护环境线程*/
		pthread_create(&tid,NULL,hold_envthread,NULL);
        pthread_detach(tid); // 分离线程
        puts("维护环境线程创建成功");

        msg_arm_t buf;
        buf = msgarm;
        /*等待用户下发指令，执行指令，返回结果*/
        while(1){
            memset(&msgarm,0,sizeof(msg_arm_t));
            // 接收上位机的指令
            if (0 == recv(sockfd, &msgarm, sizeof(msg_arm_t), 0)) {
                printf("Server closed the connection. Exiting.\n");
                exit(-1);
            }

            switch(msgarm.commd){
                case 1:
                    /*获取环境数据*/
                    puts("获取环境数据");
                    if (pthread_create(&tid, NULL, getenv_thpread, &sockfd) != 0) {
                        perror("fail to create thread");
                        exit(-1);
                    }
                    pthread_detach(tid);
                    break;
                case 2:
                    /*设置阈值*/
                    puts("设置阈值");
                    if (pthread_create(&tid, NULL, setlimit_thread, &buf) != 0) {
                        perror("fail to create thread");
                        exit(-1);
                    }
                    pthread_detach(tid);
                    break;
                case 3:
                    /*控制设备*/
                    puts("控制设备");
                    devctrl = msgarm.devctrl;
                    if (pthread_create(&tid, NULL, ctrldev_thread, &buf) != 0) {
                        perror("fail to create thread");
                        exit(-1);
                    }
                    pthread_detach(tid);
                    break;
                default:
                    break;
            }
        }
    } else {
        printf("login fail\n");
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
    float lux, resolution = 0.0049;

    contdevstatus = 0x00;
    setflags = 1;

    while (1) {
        
        // 获取环境数据
        if (ioctl(si7006_fd, GET_TMP, &tmp) == -1)
            PRINT_ERR("ioctl error");
        if (ioctl(si7006_fd, GET_HUM, &hum) == -1)
            PRINT_ERR("ioctl error");
        if(read(ap3216_fd, &als_data, sizeof(als_data)) == -1)
            PRINT_ERR("read error");

        conthume = 125.0 * hum / 65536 - 6;
        conttemp = 175.72 * tmp / 65536 - 46.85;
        contlux = als_data * resolution;

        printf("温度:%.2f 湿度:%hhd%% 光强:%d\n",conttemp,conthume,contlux);

        int duty_cycle = 0;
        char buf[10];

        if(setflags){
            if(conttemp>(settempup+0.5)){
                duty_cycle = 3;
                contdevstatus |= (0x3<<1);
            }else if(conttemp>(settempup-0.5)){
                duty_cycle = 0;
                contdevstatus &= ~(0x3<<1);
            }

            if(conthume>(sethumeup+5)){
				close(motor_fd);
				contdevstatus &= ~(0x01<<3);
			}else if(conthume<(sethumedown-5)){
				if((motor_fd = open("/dev/motor",O_RDWR))==-1)
                    PRINT_ERR("open error");
				contdevstatus |= (0x01<<3);
			}

			if(contlux>(setluxup+10)){
				for (int led_num = 1; led_num <= 6; led_num++) {
                    ioctl(led_fd, LED_OFF, &led_num);
                }
				contdevstatus &= ~(0x01<<0);
			}else if(contlux<(setluxdown-10)){
				for (int led_num = 1; led_num <= 6; led_num++) {
                    ioctl(led_fd, LED_ON, &led_num);
                }
				contdevstatus |= (0x01<<0);
			}
        }

        sprintf(buf, "%d", duty_cycle);
        if (write(fan_fd, buf, sizeof(buf)) < 0)
            PRINT_ERR("write error");
        // 休眠一段时间继续工作
        sleep(3);
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

    //返回环境数据结果
    buf.user.flags = 1;
    if (-1 == send(sockfd, &buf, sizeof(msg_arm_t), 0)) {
        perror("fail to send");
        exit(-1);
    }

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

    printf("limit temp{%.2f,%.2f} hume{%d,%d} lux{%d,%d}",settempup,settempdown,sethumeup,sethumedown,setluxup
        ,setluxdown);

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

	//将执行结果返回给服务器
    buf.user.flags = 1;
    if (-1 == send(sockfd, &buf, sizeof(msg_arm_t), 0)) {
        perror("fail to send");
        exit(-1);
    }

    pthread_exit(NULL);
}

/*控制设备线程*/
void *ctrldev_thread(void *argv)
{
    msg_arm_t buf = *(msg_arm_t *)argv;
    setflags = 0;
    int duty_cycle = 0;
    char fanbuf[10];

    //根据 buf 中devctrl 字段对应的位，控制设备的启停操作

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

	if(0x08==(buf.devctrl&(0x01<<3))){
        if((motor_fd = open("/dev/motor",O_RDWR))==-1)
            PRINT_ERR("open error"); 
		contdevstatus |= (0x01<<3);
	}else{
		close(motor_fd); 
		contdevstatus &= ~(0x01<<3);
	}

	if(0x02==(buf.devctrl&(0x03<<1))){
		puts("1");
		duty_cycle = 1;
		contdevstatus &= ~(0x03<<1);
		contdevstatus |= (0x01<<1);
	}else if(0x04==(buf.devctrl&(0x03<<1))){
		puts("2");
		duty_cycle = 2;
		contdevstatus &= ~(0x03<<1);
		contdevstatus |= (0x02<<1);
	}else if(0x06==(buf.devctrl&(0x03<<1))){
		puts("3");
		duty_cycle = 3;
		contdevstatus &= ~(0x03<<1);
		contdevstatus |= (0x03<<1);
	}else{
		duty_cycle = 0;
		contdevstatus &= ~(0x03<<1);
	}

    sprintf(fanbuf, "%d", duty_cycle);
    if (write(fan_fd, fanbuf, sizeof(fanbuf)) < 0)
        PRINT_ERR("write error");

    // 返回执行结果
    buf.user.flags = 1;
    if (-1 == send(sockfd, &buf, sizeof(msg_arm_t), 0)) {
        perror("fail to send");
        exit(-1);
    }

    pthread_exit(NULL);
}

int read_config(void)
{
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

    return 0;
}