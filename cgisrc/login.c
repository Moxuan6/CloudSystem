#include "../include/myhead.h"

char username[20] = {0};
char password[20] = {0};

void handler(int argc)
{
    while(1){
        msgrcv(msgid,&msg,sizeof(msg_t)-sizeof(long),1,IPC_NOWAIT);
		if(0 == strcmp(username,msg.user.username)){
			break;
		}else{
			msgsnd(msgid,&msg,sizeof(msg_t)-sizeof(long),0);
		}
    }

    printf("Content-Type:text/html;charset=UTF-8\n\n"); // 设置 Content-Type 头部
    printf("<script>alert('网络原因导致数据不可达，请稍后重试'); window.location.href = '/index.html';</script>");
    exit(0);
}

int cgiMain(int argc, const char *argv[])
{
    // 设置http头部和编码
    // cgiHeaderContentType("text/html; charset=utf-8"); 
    
    // 获取表单数据
    cgiFormString("ID", username, 20);
    cgiFormString("PASSWORD", password, 20);

    // 创建消息队列
    key = ftok(MSGPATH, 'm');   // 生成key
    if(key  == -1){
        exit(-1);
    }

    msgid = msgget(key, IPC_CREAT | 0666); // 创建消息队列
    if(msgid == -1 ){
        exit(-1);
    }

    // 注册信号处理函数
    signal(SIGALRM, handler);
    
    // 打开数据库
    sqlite3 *db = NULL;
    if(sqlite3_open("../Iot.db", &db) != SQLITE_OK){
        printf("Content-Type:text/html;charset=UTF-8\n\n"); // 设置 Content-Type 头部
        printf("<script>alert('数据库打开失败'); window.location.href = '/index.html';</script>");
        exit(0);
    }

    char sql[1024] = {0};
    char *errmsg = NULL, **result = NULL;
    int nrow = 0, ncolumn = 0;

    // 查询用户名和密码
    sprintf(sql, "select * from Iot where username = '%s' and password = '%s';", username, password);
    if(sqlite3_get_table(db, sql, &result, &nrow, &ncolumn, &errmsg) != SQLITE_OK){
        printf("Content-Type:text/html;charset=UTF-8\n\n"); // 设置 Content-Type 头部
        printf("<script>alert('error : %s'); window.location.href = '/index.html';</script>", errmsg);
        exit(0);
    }

    if(nrow == 0){
        printf("Content-Type:text/html;charset=UTF-8\n\n"); // 设置 Content-Type 头部
        printf("<script>alert('用户名或密码错误'); window.location.href = '/index.html';</script>");
        exit(0);
    } else {
        
        memset(&msg, 0, sizeof(msg));
        msg.msgtype = 1;
        strcpy(msg.user.username, username);

        msgsnd(msgid, &msg, sizeof(msg_t) - sizeof(long), 0);

        alarm(3);
        msgrcv(msgid, &msg, sizeof(msg_t) - sizeof(long), 2, 0);

        if(1 == msg.user.flags){
            printf("Set-Cookie:username=%s;path=/;\n",username);//设置cookie
            printf("Content-Type:text/html;charset=UTF-8\n\n"); // 设置 Content-Type 头部
            printf("<script>window.location.href = '/Iot_select.html';</script>");//自动跳转
        } else {
            printf("Content-Type:text/html;charset=UTF-8\n\n"); // 设置 Content-Type 头部
            printf("<script>alert('下位机不在线，请将下位机连接系统后重试！'); window.location.href = '/index.html';</script>");
        }
    }
    // 释放数据库
    sqlite3_close(db);
    exit(0);
}