#include "../include/myhead.h"

long long rettype;	//消息队列接收类型

void handler(int argc)
{
	msgrcv(msgid,&msg,sizeof(msg_t)-sizeof(long),rettype/2,IPC_NOWAIT);
	
	printf("Content-type: text/html;charset=\"UTF-8\"\n\n");//固定格式 必须要加
	printf("<!DOCTYPE html>");
	printf("<html>");
	printf("<body>");
	printf("<center>");
	printf("<h1>网络原因导致数据不可达，请稍后重试</h1>");
	printf("</center>");
	printf("</body>");
	printf("</html>");
	exit(0);
}

int cgiMain(int argc, const char *argv[])
{
	/*创建消息队列*/
	key = ftok(MSGPATH,'m');
	if(-1==key){
		return -1;;
	}

	msgid = msgget(key,IPC_CREAT|0666);
	if(-1==msgid){
		return -1;
	}

	/*注册alarm信号*/
	signal(SIGALRM,handler);
	
	char name[128] = {0};
	memcpy(name,cgiCookie,128);
	char *s = name;
	char buf[128] = {0};

	while(*s!='='){
		s++;	
	}
	s++;
	long long msgtype = 0;
	int i = 0;
	memset(&msg,0,sizeof(msg_t));
	while(*s){
		buf[i] = *s;
		msg.msgtype+=*s;
		s++;
		i++;
	}

	/*封装消息*/
	rettype = msg.msgtype * 2;
	msg.commd = 1; 	//查看环境数据
	msgsnd(msgid,&msg,sizeof(msg_t)-sizeof(long),0);	//发送消息

	/*设置超时时间*/
	alarm(5);

	/*接收消息*/
	memset(&msg,0,sizeof(msg_t));
	msgrcv(msgid,&msg,sizeof(msg_t)-sizeof(long),rettype,0);

	msg.user.flags = 1;
	if( 1 == msg.user.flags){
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
		printf("<td bgcolor=\"yellow\" width=\"80\" height=\"20\">%.2f</td>",msg.envdata.temp);
		printf("<td bgcolor=\"yellow\" width=\"80\" height=\"20\">%hhd%%</td>",msg.envdata.hume);
		printf("<td bgcolor=\"yellow\" width=\"80\" height=\"20\">%hdlux</td>",msg.envdata.lux);
		printf("</tr>");
		printf("</table>");
		printf("<h4>设备状态</h4>");
		printf("<table border = \"1\">");
		printf("<th bgcolor=\"#00FFFF\" >照明</th>");
		printf("<th bgcolor=\"#00FFFF\">温控</th>");
		printf("<th bgcolor=\"#00FFFF\">湿控</th>");
		printf("<tr>");

		/*设备状态甄别显示*/
		if(msg.envdata.devstatus & 0x01){
			printf("<td bgcolor=\"yellow\" width=\"80\" height=\"20\">开</td>");
		}else{
			printf("<td bgcolor=\"yellow\" width=\"80\" height=\"20\">关</td>");
		}

		/*解析温控状态*/
		switch(msg.envdata.devstatus & 0x3 << 1){ 
			case 0:
				printf("<td bgcolor=\"yellow\" width=\"80\" height=\"20\">关</td>");
				break;
			case 1:
				printf("<td bgcolor=\"yellow\" width=\"80\" height=\"20\">1挡</td>");
				break;
			case 2:
				printf("<td bgcolor=\"yellow\" width=\"80\" height=\"20\">2挡</td>");
				break;
			case 3:
				printf("<td bgcolor=\"yellow\" width=\"80\" height=\"20\">3挡</td>");
				break;
			default:
				break;
		}

		/*解析湿控状态*/
		if(msg.envdata.devstatus & 0x1 << 3){
			printf("<td bgcolor=\"yellow\" width=\"80\" height=\"20\">开</td>");
		}else{
			printf("<td bgcolor=\"yellow\" width=\"80\" height=\"20\">关</td>");
		}

		printf("</tr>");
		printf("</table>");
		printf("<br>");
		printf("<form action=\"../cgi-bin/garden_stuff_view.cgi\" method=\"get\">");
		printf("<input type=\"submit\" name=\"set_button\" value=\"点击刷新数据\">");
		printf("</form>");
		printf("</center>");
		printf("</body>");
		printf("</html>");

	} else {
		printf("Content-type: text/html;charset=\"UTF-8\"\n\n");//固定格式 必须要加
		printf("<!DOCTYPE html>");
		printf("<html>");
		printf("<body bgcolor=\"#00FFFF\">");
		printf("<center>");
		printf("<h4>网络原因导致数据不可达，请稍后重试2</h4>");
		printf("</center>");
		printf("</body>");
		printf("</html>");
	}

	return 0;
}