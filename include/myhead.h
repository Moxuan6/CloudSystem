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

#include "cgic.h"

#define MSGPATH "/home/jeremy/"

/* 统一数据结构 */
typedef struct {
    char username[32];
    char password[32];
    char flags; // 成功 1 失败 0
} user_t;

/* 查看环境数据 */
typedef struct{
    float temp;                 // 温度
    unsigned char hume;         // 湿度
    unsigned char lux;          // 光照
    unsigned char devstatus;    // 设备状态
} env_t;

/* 设置阈值数据 */
typedef struct{
    float tempup;               // 温度上限
    float tempdown;             // 温度下限
    unsigned char humeup;       // 湿度上限
    unsigned char humedown;     // 湿度下限
    unsigned char luxup;        // 光照上限
    unsigned char luxdown;      // 光照下限
} limitset_t;

typedef struct {
    long long msgtype;
    char commd;
    user_t user;
    env_t envdata;
    limitset_t limitset;
    /* 控制设备数据 */
    char devctrl;
} msg_t;

/* 链表节点 */
typedef struct node{
    char ID[32];
    int fd;
    struct node *next;
}link_t;

msg_t msg;
key_t key;
int msgid;
link_t *head;
pthread_t tid;

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

#endif