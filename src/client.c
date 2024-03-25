#include "../include/myhead.h"

#include "../mydev/temp-hume/si7006.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

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

    if( 1 == msg.user.flags){
        /*创建维护环境线程*/
		pthread_create(&tid,NULL,hold_envthread,NULL);

        /*等待用户下发指令，执行指令，返回结果*/
        while(1){
            memset(&msg,0,sizeof(msg));
            
            // 接收上位机的指令
            if(0 == recv(sockfd,&msg,sizeof(msg),0)){
                perror("fail to recv");
                exit(-1);
            }

            switch(msg.commd){
                case 1:
                    /*获取环境数据*/
                    if (pthread_create(&tid, NULL, getenv_thpread, &sockfd) != 0) {
                        perror("fail to create thread");
                        exit(-1);
                    }
                    break;
                case 2:
                    /*设置阈值*/
                    if (pthread_create(&tid, NULL, setlimit_thread, &sockfd) != 0) {
                        perror("fail to create thread");
                        exit(-1);
                    }
                    break;
                case 3:
                    /*控制设备*/
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
        return -1;
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
	//读取参考文件数据赋值给参考变量

    // //读取参考文件数据
    // FILE *fp = fopen("init.txt", "r");
    // if (fp == NULL) {
    //     perror("fail to fopen");
    //     exit(-1);
    // }

    // //fscanf(文件指针, "格式化字符串", 参数列表), %hhu 无符号字符
    // fscanf(fp, "tempup=%f\n", &buf.limitset.tempup);
    // fscanf(fp, "tempdown=%f\n", &buf.limitset.tempdown);
    // fscanf(fp, "humeup=%hhu\n", &buf.limitset.humeup);
    // fscanf(fp, "humedown=%hhu\n", &buf.limitset.humedown);
    // fscanf(fp, "luxup=%hu\n", &buf.limitset.luxup);
    // fscanf(fp, "luxdown=%hu\n", &buf.limitset.luxdown);

    // //打印参考变量数据
    // printf("tempup=%f\n", buf.limitset.tempup);
    // printf("tempdown=%f\n", buf.limitset.tempdown);
    // printf("humeup=%hhu\n", buf.limitset.humeup);
    // printf("humedown=%hhu\n", buf.limitset.humedown);
    // printf("luxup=%hu\n", buf.limitset.luxup);
    // printf("luxdown=%hu\n", buf.limitset.luxdown);

    // fclose(fp);

	while(1){
	// 	//读取环境数据
    //    // 先模拟环境数据
    //     buf.envdata.temp = 30.5;
    //     buf.envdata.hume = 30;
    //     buf.envdata.lux = 20;
    //     buf.envdata.devstatus = 0x01;

    //     //根据环境数据与参考值比较，决定是否开启对应的设备
    //     //判断温度是否超过上限
    //     if(buf.envdata.temp > buf.limitset.tempup){
    //         //开启风扇设备
    //         buf.envdata.devstatus |= 0x02;
    //         printf("风扇开启\n");
    //     }else if(buf.envdata.temp < buf.limitset.tempdown){
    //         //关闭风扇设备
    //         buf.envdata.devstatus &= ~0x02;
    //         printf("风扇关闭\n");
    //     }

    //     //判断湿度是否超过上限
    //     if(buf.envdata.hume > buf.limitset.humeup){
    //         //开启蜂鸣器设备
    //         buf.envdata.devstatus |= 0x08;
    //         printf("蜂鸣器开启\n");
    //     }else if(buf.envdata.hume < buf.limitset.humedown){
    //         //关闭蜂鸣器设备
    //         buf.envdata.devstatus &= ~0x08;
    //         printf("蜂鸣器关闭\n");
    //     }

    //     //判断光照是否超过上限
    //     if(buf.envdata.lux > buf.limitset.luxup){
    //         //开启LED设备
    //         buf.envdata.devstatus |= 0x01;
    //         printf("LED开启\n");
    //     }else if(buf.envdata.lux < buf.limitset.luxdown){
    //         //关闭LED设备
    //         buf.envdata.devstatus &= ~0x01;
    //         printf("LED关闭\n");
    //     }
	// 	//休眠一段时间继续工作
    //     sleep(50);
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

    if((fd = open("/dev/ap3216",O_RDWR))==-1)
        PRINT_ERR("open error");

    read(fd, &als_data, sizeof(als_data));
    lux = als_data * resolution;

    close(fd);

    // 模拟读取 sensor 检测到的环境数据
    buf.envdata.temp = tmp_f;
    buf.envdata.hume = hum_f;
    buf.envdata.lux = lux;
    
    buf.envdata.devstatus = 0x01 | 0x02 | 0x08; // 0000 0001 | 0000 0010 | 0000 1000

    //打印环境数据
    printf("temp=%.2f\n", buf.envdata.temp);
    printf("hume=%.2f\n", buf.envdata.hume);
    printf("lux=%hu\n", buf.envdata.lux);
    printf("devstatus=%hhu\n", buf.envdata.devstatus);

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
    // int sockfd = *(int *)arg;
	// //将 buf.limitset字段中的数据赋值给参考变量
    
    // //将参考变量数据写入到文件中
    // FILE *fp = fopen("init.txt", "w");
    // if (fp == NULL) {
    //     perror("fail to fopen");
    //     exit(-1);
    // }

    // fprintf(fp, "tempup=%f\n", buf.limitset.tempup);
    // fprintf(fp, "tempdown=%f\n", buf.limitset.tempdown);
    // fprintf(fp, "humeup=%hhu\n", buf.limitset.humeup);
    // fprintf(fp, "humedown=%hhu\n", buf.limitset.humedown);
    // fprintf(fp, "luxup=%hhu\n", buf.limitset.luxup);
    // fprintf(fp, "luxdown=%hhu\n", buf.limitset.luxdown);

    // fclose(fp);


	// //将参考变量数据写入到文件中，方便设备下次启动时有一个合理的参考值

	// //将执行结果返回给服务器
    // if (-1 == send(sockfd, &buf, sizeof(buf), 0)) {
    //     perror("fail to send");
    //     exit(-1);
    // }

    // pthread_exit(NULL);
}

/*控制设备线程*/
void *ctrldev_thread(void *arg)
{
    msg_t buf;
    // int sockfd = *(int *)arg;
    // // 根据 buf 中devctrl 字段对应的位，控制设备的启停操作
    
    // if(buf.devctrl & 0x01){
    //     //开启LED设备
    //     buf.envdata.devstatus |= 0x01;
    //     printf("LED开启\n");
    // }else{
    //     //关闭LED设备
    //     buf.envdata.devstatus &= ~0x01;
    //     printf("LED关闭\n");
    // }

    // if(buf.devctrl & 0x02){
    //     //开启风扇设备
    //     buf.envdata.devstatus |= 0x02;
    //     printf("风扇开启\n");
    // }else{
    //     //关闭风扇设备
    //     buf.envdata.devstatus &= ~0x02;
    //     printf("风扇关闭\n");
    // }

    // if(buf.devctrl & 0x04){
    //     //开启蜂鸣器设备
    //     buf.envdata.devstatus |= 0x08;
    //     printf("蜂鸣器开启\n");
    // }else{
    //     //关闭蜂鸣器设备
    //     buf.envdata.devstatus &= ~0x08;
    //     printf("蜂鸣器关闭\n");
    // }

    // // 返回执行结果
    // if (-1 == send(sockfd, &buf, sizeof(buf), 0)) {
    //     perror("fail to send");
    //     exit(-1);
    // }

    // pthread_exit(NULL);
}