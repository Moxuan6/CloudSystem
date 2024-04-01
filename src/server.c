
#include <../include/myhead.h>

int network_init(int sockfd, int port)
{
    // 创建套接字
    if (-1 == (sockfd = socket(AF_INET, SOCK_STREAM, 0))) {
        perror("socket");
        return -1;
    }

    // 设置端口复用
    int opt = 1;
    if (-1 == setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("fail to setsockopt");
        exit(-1);
    }

    // 填充网络信息结构体
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // 绑定
    if (-1 == bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr))) {
        perror("fail to bind");
        exit(-1);
    }

    if (-1 == listen(sockfd, 5)) {
        perror("fail to listen");
        exit(-1);
    }

    return sockfd;
}

void sig_handler(int signo)
{
    if (signo == SIGINT || signo == SIGTERM) {
        msgctl(msgid, IPC_RMID, NULL); // 删除消息队列
        exit(0);
    }
}

int main(int argc, char const *argv[])
{
    if (signal(SIGINT, sig_handler) == SIG_ERR 
        || signal(SIGTERM, sig_handler) == SIG_ERR) {
        
        printf("\ncan't catch SIGINT or SIGTERM\n");
        exit(-1);
    }

    if (argc != 2) {
        fprintf(stderr, "usage:%s <port>\n", argv[0]);
        exit(-1);
    }

    // 网络初始化
    int sockfd = network_init(sockfd, atoi(argv[1]));
    if (sockfd == -1) {
        perror("fail to network_init");
        exit(-1);
    }

    // 消息队列初始化
    key_t key = ftok(MSGPATH, 'm'); // 生成key
    if (key == -1)
    {
        perror("fail to ftok");
        close(sockfd);
        exit(-1);
    }

    // 创建消息队列
    msgid = msgget(key, 0666 | IPC_CREAT);
    if (msgid == -1)
    {
        perror("fail to msgget");
        close(sockfd);
        exit(-1);
    }

    // 创建链表
    head = create_link();
    if (head == NULL) {
        perror("fail to create_link");
        close(sockfd);
        msgctl(msgid, IPC_RMID, NULL); // 删除消息队列
        exit(-1);
    }

    /*信号量初始化*/
    if(sem_init(&linksem, 0, 1) == -1){
        perror("fail to sem_init");
        close(sockfd);
        msgctl(msgid, IPC_RMID, NULL); // 删除消息队列
        destroy_link(&head);
        exit(-1);
    }

    /*创建心跳包线程，检索下位机是否存活*/
    if (pthread_create(&tid, NULL, insprct_thread, NULL) != 0) {
        perror("fail to pthread_create");
        close(sockfd);
        msgctl(msgid, IPC_RMID, NULL); // 删除消息队列
        destroy_link(&head);
        exit(-1);
    }
    pthread_detach(tid); // 分离线程

    // 创建处理检索下位机在线状态的线程
    if (pthread_create(&tid, NULL, login_thread, NULL) != 0) {
        perror("fail to pthread_create");
        close(sockfd);
        msgctl(msgid, IPC_RMID, NULL); // 删除消息队列
        destroy_link(&head);
        exit(-1);
    }
    pthread_detach(tid); // 分离线程

    struct sockaddr_in client_addr;          // 客户端网络信息结构体
    socklen_t addrlen = sizeof(client_addr); // 客户端网络信息结构体大小
    int accept_fd = 0;

    while (1) {
        // 接受客户端连接
        if (-1 == (accept_fd = accept(sockfd, (struct sockaddr *)&client_addr, &addrlen))) {
            perror("fail to accept");
            msgctl(msgid, IPC_RMID, NULL); // 删除消息队列
            exit(-1);
        }

        puts("有下位机连接成功");

        // 创建处理线程
        if (pthread_create(&tid, NULL, handl_thread, &accept_fd) != 0) {
            perror("fail to pthread_create");
            close(sockfd);
            msgctl(msgid, IPC_RMID, NULL); // 删除消息队列
            destroy_link(&head);
            exit(-1);
        }
        pthread_detach(tid); // 分离线程
    }

    return 0;
}

void *handl_thread(void *argv)
{
    int accept_fd = *(int *)argv;
    char ID[] = "admin";
    char PS[] = "123456";
    long recvmsgtype, sendmsgtype;  // 消息队列接收类型
    msg_t buf;
    int ret;

    /*处理登录*/
    if(0 == recv(accept_fd, &buf, sizeof(buf), 0)){
        perror("fail to recv");
        close(accept_fd);
        pthread_exit(NULL);
    }

    printf("ID:%s\n", buf.user.username);

    if( 0 == strcmp(buf.user.username, ID) && 0 == strcmp(buf.user.password, PS)){
        buf.user.flags = 1;     // 成功
        recvmsgtype = 0;        // 接收消息类型
        int i = 0;
        while(buf.user.username[i] != '\0'){
            recvmsgtype += buf.user.username[i];   // 计算接收消息类型
            i++;
        }
        
        sendmsgtype = recvmsgtype * 2;  // 发送消息类型
        printf("等待的消息类型为%ld,发送的消息类型为%ld\n",recvmsgtype,sendmsgtype);
        send(accept_fd, &buf, sizeof(msg_t), 0);
        
        pthread_t tid = pthread_self();

        /*获取信号量，插入链表*/
        sem_wait(&linksem);
        insert_link(head, buf.user.username, accept_fd, tid);    // 插入链表
        sem_post(&linksem);

        while(1){
            memset(&buf, 0, sizeof(msg_t));
            // 等待消息
            msgrcv(msgid, &buf, sizeof(msg_t) - sizeof(long), recvmsgtype, 0);
            puts("接收到消息...");
            // 发送消息

            sem_wait(&linksem);
            if (find_fd(head, accept_fd)) {
                sem_wait(&linksem);
                delete_link(head, accept_fd);
                sem_post(&linksem);
                buf.msgtype = sendmsgtype;  // 发送消息类型
                buf.user.flags = 0;         // 失败
                msgsnd(msgid, &buf, sizeof(msg_t) - sizeof(long), 0);
                close(accept_fd);
                pthread_exit(NULL);
            }
            sem_post(&linksem);
            send(accept_fd, &buf, sizeof(msg_t), 0);
            // 接收消息       
            ret = recv(accept_fd, &buf, sizeof(msg_t), 0);
            if(ret == 0){
                close(accept_fd);
                delete_link(head, accept_fd); // 删除链表中的节点
                pthread_exit(NULL);
            }
            // 返回结果
            buf.msgtype = sendmsgtype;  // 发送消息类型
            msgsnd(msgid, &buf, sizeof(msg_t) - sizeof(long), 0);
        }
    
    } else {
        printf("登录失败...\n");
        buf.user.flags = 0;     // 失败
        send(accept_fd, &buf, sizeof(msg_t), 0);
        close(accept_fd);
        pthread_exit(NULL);     // 退出线程
    }
}

void *insprct_thread(void *argv) {
    msg_t buf;
    while (1) {
        show_list(head);
        sleep(10);
        puts("检测下位机在线状态...");
    }
}

void *login_thread(void *argv)
{
    msg_t buf;

    while (1) {
        memset(&buf, 0, sizeof(buf));

        // 监控消息队列中的 1 号消息
        if (msgrcv(msgid, &buf, sizeof(msg_t) - sizeof(long), 1, 0) == -1) {
            perror("fail to msgrcv");
            exit(-1);
        }

        // 遍历链表
        printf("ID:%s\n", buf.user.username);
        if (find_link(head, buf.user.username) == -1) {
            buf.user.flags = 0; // 失败
        } else {
            buf.user.flags = 1; // 成功
        }

        // 返回结果
        buf.msgtype = 2; // 发送到 2 号消息队列
        if (msgsnd(msgid, &buf, sizeof(msg_t) - sizeof(long), 0) == -1) {
            perror("fail to msgsnd");
            exit(-1);
        }
    }
}

/*创建链表*/
link_t *create_link()
{
    link_t *head = (link_t *)malloc(sizeof(link_t));
    if (head == NULL) {
        perror("fail to malloc");
        exit(-1);
    }
    memset(head, 0, sizeof(link_t));
    head->next = NULL;
    return head;
}

/*增加数据*/
void insert_link(link_t *head, const char *ID, int fd, pthread_t tid)
{
    if (head == NULL || ID == NULL) {
        return;
    }
    link_t *p = (link_t *)malloc(sizeof(link_t));
    if (p == NULL) {
        perror("fail to malloc");
        exit(-1);
    }
    memset(p, 0, sizeof(link_t));
    strcpy(p->ID, ID);
    p->fd = fd;
    p->tid = tid;
    p->next = head->next;
    head->next = p;
}

/*根据ID查找*/
int find_link(link_t *head, const char *ID)
{
    if (head == NULL || ID == NULL) {
        return -1;
    }
    link_t *p = head->next;
    while (p != NULL) {
        if (strcmp(p->ID, ID) == 0)
        {
            return p->fd;
        }
        p = p->next;
    }
    return -1;
}

/*根据fd查找*/
int find_fd(link_t *head, int fd) 
{
    link_t *p = head->next;
    while (p) {
        if (p->fd == fd){
            return 0;
        }
        p = p->next;
    }

    return -1;
}

/*根据fd删除*/
void delete_link(link_t *head, int fd)
{
    if (head == NULL || fd < 0) {
        return;
    }
    link_t *p = head->next;
    link_t *q = head;
    while (p != NULL) {
        if (p->fd == fd)
        {
            q->next = p->next;
            free(p);
            p = NULL;
            return;
        }
        q = p;
        p = p->next;
    }
}

/*销毁链表*/
void destroy_link(link_t **head)
{
    if (head == NULL || *head == NULL) {
        return;
    }
    link_t *p = (*head)->next;
    link_t *q = NULL;
    while (p != NULL) {
        q = p->next;
        free(p);
        p = q;
    }
    free(*head);
    *head = NULL;
}

int show_list(link_t *head) {
    msg_t buf;
    link_t *p = head->next;
    while (p) {
        memset(&buf, 0, sizeof(msg_t));
        buf.commd = 255;
        send(p->fd, &buf, sizeof(msg_t), 0);
        if(0 >> recv(p->fd, &buf, sizeof(msg_t), 0)){
            printf("%s 离线\n", p->ID);
            pthread_cancel(p->tid);
            sem_wait(&linksem);
            delete_link(head, p->fd);
            sem_post(&linksem);
            p = head->next;
        } else {
            printf("当前 %s 在线\n", p->ID);
            p = p->next;
        }
    }
    return 0;
}