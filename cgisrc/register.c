#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "./cgisrc/cgic.h"

char username[20] = {0};
char password[20] = {0};
char password2[20] = {0};

int cgiMain(void)
{
    // 设置http头部和编码
    cgiHeaderContentType("text/html; charset=utf-8"); 
    // 获取表单数据
    cgiFormString("username1", username, 20);
    cgiFormString("password1", password, 20);
    cgiFormString("password2", password2, 20);

     // 检查用户名和密码是否为空
    if(strlen(username) == 0 || strlen(password) == 0){
        printf("<script>alert('用户名或密码不能为空'); window.location.href = '/register.html';</script>");
        return 0;
    }

    // 检查两次输入的密码是否一致
    if(strcmp(password, password2) != 0){
        printf("<script>alert('两次输入的密码不一致'); window.location.href = '/register.html';</script>");
        return 0;
    }

    sqlite3 *db = NULL;
    if(sqlite3_open("../Iot.db", &db) != SQLITE_OK){
        printf("<script>alert('数据库打开失败'); window.location.href = '/register.html';</script>");
        return 0;
    }

    char sql[1024] = {0};
    char *errmsg = NULL;

    // 检查用户名是否已存在
    sprintf(sql, "select * from Iot where username = '%s';", username);
    char **result = NULL;
    int nrow = 0, ncolumn = 0;
    if(sqlite3_get_table(db, sql, &result, &nrow, &ncolumn, &errmsg) != SQLITE_OK){
        printf("<script>alert('查询失败: %s'); window.location.href = '/register.html';</script>", errmsg);
        return 0;
    }
    if(nrow > 0){
        printf("<script>alert('用户名已存在'); window.location.href = '/register.html';</script>");
        return 0;
    }

    // 插入新用户
    sprintf(sql, "insert into Iot (username, password) values ('%s', '%s');", username, password);
    if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK){
        printf("<script>alert('注册失败: %s'); window.location.href = '/register.html';</script>", errmsg);
        return 0;
    }

    // 释放数据库
    sqlite3_close(db);

    printf("<script>alert('注册成功'); window.location.href = '/index.html';</script>");

    return 0;
}