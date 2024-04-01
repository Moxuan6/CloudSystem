#ifndef __MYHEAD_H__
#define __MYHEAD_H__

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <semaphore.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sqlite3.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <errno.h>
#include "cgic.h"

#define MSGPATH "/home/jeremy/" //消息队列路径
#define PRINT_ERR(massge) do { perror(massge); close_device(); exit(1); } while (0)

/* 统一数据结构 */
/* 登录数据 */
typedef struct {
    char username[32];
    char password[32];
    char flags; // 成功 1 失败 0
} user_t;

/* 查看环境数据 */
typedef struct{
    float temp;                 // 温度
    float hume;         // 湿度
    float lux;         // 光照
    unsigned char devstatus;    // 0-7bit 0照明  1-2温控挡位(00==off 01==1挡 10==2挡 11=3挡) 3加湿 
} env_t;

/* 设置阈值数据 */
typedef struct{
    float tempup;               // 温度上限
    float tempdown;             // 温度下限
    float humeup;       // 湿度上限
    float humedown;     // 湿度下限
    float luxup;        // 光照上限
    float luxdown;      // 光照下限
} limitset_t;

typedef struct {
    long msgtype;
    char commd;
    user_t user;
    env_t envdata;
    limitset_t limitset;
    /* 控制设备数据 */
    char devctrl;
} msg_t;

typedef struct {
    long long msgtype;
    char commd;
    user_t user;
    env_t envdata;
    limitset_t limitset;
    /* 控制设备数据 */
    char devctrl;
} msg_arm_t;

msg_arm_t  msgarm;     //消息结构体
msg_t msg;          //消息结构体

char setflags = 0;      //设置阈值标志
int fan_duty_cycle = 0;
int motor_duty_cycle = 0;
char fanbuf[10];
char motorbuf[10];

/*采集到的数据缓冲变量*/
float conttemp;
float conthume;
float contlux;
unsigned char contdevstatus;

/*用户设置的参考变量*/
float settempup;
float settempdown;
float sethumeup;
float sethumedown;
float setluxup;
float setluxdown;

/* 链表节点 */
typedef struct node{
    char ID[32];
    int fd;
    pthread_t tid;
    struct node *next;
}link_t;

link_t *head;       //链表头指针
key_t key;          //消息队列 key
int msgid;          //消息队列 id
pthread_t tid;      //线程ID
int led_fd,motor_fd,si7006_fd,ap3216_fd,fan_fd; //设备文件描述符
int sockfd;         //网络套接字
sem_t linksem;      //信号量

#define LED_ON _IOW('L', 0, int)
#define LED_OFF _IOW('L', 1, int)

/* 信号处理 */
void sig_handler(int signo);

/* 网络初始化 */
int network_init(int sockfd, int port);

/* 线程处理 */
void *handl_thread(void *argv);

/* 登录处理 */
void *login_thread(void *argv);

/* 创建链表 */
link_t *create_link();

/* 增加数据 */
void insert_link(link_t *head, const char *ID, int fd);

/* 根据ID查找 */
int find_link(link_t *head, const char *ID);

/* 根据fd删除 */
void delete_link(link_t *head, int fd);

/* 销毁链表 */
void destroy_link(link_t **head);

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

/*获取文件配置*/
int read_config(void);

#endif