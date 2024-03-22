#include "../include/myhead.h"
#include <pthread.h>

#define MAX_THREADS 100

// 网络初始化
int client_network_init(int sockfd, struct sockaddr_in *addr, const char *ip, int port);
void *control_fan(void *arg);
void *get_temperature_humidity(void *arg);
void *control_light(void *arg);
void *control_buzzer(void *arg);

pthread_t tids[MAX_THREADS]; // 保存线程ID的数组
int tid_index = 0; // 下一个可用的数组索引
pthread_mutex_t lock; // 互斥锁

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
    memset(&msg, 0, sizeof(msg));
    msg.msgtype = 1;
    strcpy(msg.user.username, "admin");
    strcpy(msg.user.password, "123456");

    if (-1 == send(sockfd, &msg, sizeof(msg), 0)) {
        perror("fail to send");
        exit(-1);
    }

    // 初始化互斥锁
    if (pthread_mutex_init(&lock, NULL) != 0) {
        perror("fail to init mutex");
        exit(-1);
    }

    // 处理上位机的发来的消息
    int ret;
    pthread_t tid;

    while (1) {
        memset(&msg, 0, sizeof(msg));
        ret = recv(sockfd, &msg, sizeof(msg), 0);
        if (ret == 0) {
            printf("server close\n");
            close(sockfd); // 关闭套接字
            break;
        }
        else if (ret == -1) {
            perror("fail to recv");
            exit(-1);
        }

        switch (msg.msgtype) {
        case 10:
            if (pthread_create(&tid, NULL, control_fan, NULL) != 0) {
                perror("fail to create thread");
                exit(-1);
            }
            break;
        case 20:
            if (pthread_create(&tid, NULL, get_temperature_humidity, NULL) != 0) {
                perror("fail to create thread");
                exit(-1);
            }
            break;
        case 30:
            if (pthread_create(&tid, NULL, control_light, NULL) != 0) {
                perror("fail to create thread");
                exit(-1);
            }
            break;
        case 40:
            if (pthread_create(&tid, NULL, control_buzzer, NULL) != 0) {
                perror("fail to create thread");
                exit(-1);
            }
            break;
        }

        // 保存线程ID
        if (tid_index < MAX_THREADS) {
            tids[tid_index++] = tid;
        } else {
            fprintf(stderr, "too many threads\n");
            exit(-1);
        }
    }

    // 等待线程结束
    for (int i = 0; i < tid_index; i++) {
        if (pthread_join(tids[i], NULL) != 0) {
            perror("fail to join thread");
            exit(-1);
        }
    }

    // 销毁互斥锁
    pthread_mutex_destroy(&lock);

    close(sockfd);

    return 0;
}

void *control_fan(void *arg)
{
    // 控制风扇的代码
    printf("开始控制风扇\n");

    pthread_mutex_lock(&lock); // 锁定互斥锁

    // 访问或修改共享资源的代码
    printf("访问或修改共享资源\n");

    pthread_mutex_unlock(&lock); // 解锁互斥锁

    printf("结束控制风扇\n");

    return NULL;
}

void *get_temperature_humidity(void *arg)
{
    // 获取温湿度的代码
    printf("开始获取温湿度\n");

    pthread_mutex_lock(&lock); // 锁定互斥锁

    // 访问或修改共享资源的代码
    printf("访问或修改共享资源\n");

    pthread_mutex_unlock(&lock); // 解锁互斥锁

    printf("结束获取温湿度\n");

    return NULL;
}

void *control_light(void *arg)
{
    // 控制灯的代码
    printf("开始控制灯\n");

    pthread_mutex_lock(&lock); // 锁定互斥锁

    // 访问或修改共享资源的代码
    printf("访问或修改共享资源\n");

    pthread_mutex_unlock(&lock); // 解锁互斥锁

    printf("结束控制灯\n");

    return NULL;
}

void *control_buzzer(void *arg)
{
    // 控制蜂鸣器的代码
    printf("开始控制蜂鸣器\n");

    pthread_mutex_lock(&lock); // 锁定互斥锁

    // 访问或修改共享资源的代码
    printf("访问或修改共享资源\n");

    pthread_mutex_unlock(&lock); // 解锁互斥锁

    printf("结束控制蜂鸣器\n");

    return NULL;
}