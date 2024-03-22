#include "../include/myhead.h"
/*
   void handler(int argc)
   {
   while(1){
   msgrcv(msqid,&msg,sizeof(msg_t)-sizeof(long),1,IPC_NOWAIT);
   if(0==strcmp(username,msg.user.username)){
   break;
   }else{
   msgsnd(msqid,&msg,sizeof(msg_t)-sizeof(long),0);
   }
   }
   printf("Content-type: text/html;charset=\"UTF-8\"\n\n");//固定格式 必须要加
   printf("printf("<!DOCTYPE html>");
   printf("printf("<html>");
   printf("printf("<body>");
   printf("printf("<center>");
   printf("printf("<h1>网络原因导致数据不可达，请稍后重试</h1>");//自动跳转
   printf("printf("<a href=\"../index.html\" target=\"blank\">点击返回登录</a> ");//点击返回
   printf("printf("</center>");
   printf("printf("</body>");
   printf("printf("</html>");
   exit(0);
   }
   */
int cgiMain(int argc, const char *argv[])
{
#if 0
	/*创建消息队列*/
	key = ftok(MSGPATH,'m');
	if(-1==key){
		return -1;;
	}

	msqid = msgget(key,IPC_CREAT|0666);
	if(-1==msqid){
		return -1;
	}

	/*注册alarm信号*/
	signal(SIGALRM,handler);
#endif
	char name[128] ={0};
	float temp = 23.52;
	char hume = 45;
	short lux = 350;
	char led[] = "开";
	char fan[] = "关";
	char beep[] = "关";
	
    memcpy(name,cgiCookie,128);
    char *s=name;
    while(*s!='='){
        s++;
    }
    s=s+1;
    int msgtype=0;
    char buf[120]={0};//登录的用户ID
    int i=0;
    while(*s){
        buf[i]=*s;
        i++;
        s++;
    }
    
	printf("Content-type: text/html;charset=\"UTF-8\"\n\n");//固定格式 必须要加
	printf("<!DOCTYPE html>");
	printf("<html>");
	printf("<body bgcolor=\"#00FFFF\">");
	printf("<center>");
	printf("<table border = \"3\">");
	printf("<tr>");
	printf("<td bgcolor=\"#00FFFF\">%s</td>",buf);	
	printf("</tr>");
	printf("</table>");
	printf("<h4>环境数据</h4>");
	printf("<table border = \"1\">");
	printf("<th bgcolor=\"#00FFFF\">温度</th>");
	printf("<th bgcolor=\"#00FFFF\">湿度</th>");
	printf("<th bgcolor=\"#00FFFF\">光强</th>");
	printf("<tr>");
	printf("<td bgcolor=\"yellow\" width=\"80\" height=\"20\">%.2f</td>",temp);
	printf("<td bgcolor=\"yellow\" width=\"80\" height=\"20\">%hhd%%</td>",hume);
	printf("<td bgcolor=\"yellow\" width=\"80\" height=\"20\">%hdlux</td>",lux);
	printf("</tr>");
	printf("</table>");
	printf("<h4>设备状态</h4>");
	printf("<table border = \"1\">");
	printf("<th bgcolor=\"#00FFFF\" >照明</th>");
	printf("<th bgcolor=\"#00FFFF\">温控</th>");
	printf("<th bgcolor=\"#00FFFF\">湿控</th>");
	printf("<tr>");
	printf("<td bgcolor=\"yellow\" width=\"80\" height=\"20\">%s</td>",led);
	printf("<td bgcolor=\"yellow\" width=\"80\" height=\"20\">%s</td>",fan);
	printf("<td bgcolor=\"yellow\" width=\"80\" height=\"20\">%s</td>",beep);
	printf("</tr>");
	printf("</table>");
	printf("<br>");
	printf("<form action=\"../cgi-bin/garden_stuff_view.cgi\" method=\"get\">");
	printf("<input type=\"submit\" name=\"set_button\" value=\"点击刷新数据\" />");
	printf("</form>");
	printf("</center>");
	printf("</body>");
	printf("</html>");

	return 0;
}