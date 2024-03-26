#include <../include/myhead.h>

long rettype;	//消息队列接收类型

int cgiMain(int argc, const char *argv[])
{
	/*创建消息队列*/
	key = ftok(MSGPATH,'m');
	if(-1==key){
		return -1;;
	}

	msgid = msgget(key, IPC_CREAT | 0666);
	if(-1==msgid){
		return -1;
	}

	memset(&msg,0,sizeof(msg_t));

	/*获取 用户 ID (login.cgi 设置的 cookie 值)*/
	char namebuf[128] = {0};                            
	memcpy(namebuf,cgiCookie,128);
	
	char *s = namebuf;
	char buf[20] = {0};

	while(*s!='='){
		s++;
	}
	s++;
	int i = 0;
	long msgtype = 0;
	memset(&msg,0,sizeof(msg_t));
	while(*s){
		buf[i] = *s;
		msg.msgtype += *s;
		s++;
		i++;
	}

	//封装设置阈值的消息
	rettype = msg.msgtype * 2;//封装发送消息类型
	msg.commd = 3;//告诉下位机 将设置阈值字段 赋值给 下位机的参考变量
	
	/*获取网页数据*/
	char devstatu[4] = {0};//放置 从网页获用户对设备的控制操作状态

	/*获取网页设置的 LED状态*/
	cgiFormString("LED",devstatu,4);
	if(!strcmp("ON",devstatu)){
		msg.devctrl |= (0x1<<0);/*开灯*/
	}else{
		msg.devctrl &= ~(0x1<<0);/*关灯*/
	
	}
	memset(devstatu,0,4);

	/*获取网页空调状态*/
	cgiFormString("FAN",devstatu,4);
	if(!strcmp("ON",devstatu)){
		msg.devctrl |= (0x3<<1);/*开空调*/
	}else{
		msg.devctrl &= ~(0x3<<1);/*关空调*/
	}
	memset(devstatu,0,4);
	
	/*获取网页加湿器的控制状态*/
	cgiFormString("BEEP",devstatu,4);
	if(!strcmp("ON",devstatu)){
		msg.devctrl |= (0x1<<3);/*开加湿器*/
	}else{
		msg.devctrl &= ~(0x1<<3);/*关加湿器*/
	}

	/*将请求消息通过消息队列---socket发送给下位机*/
	msgsnd(msgid,&msg,sizeof(msg_t)-sizeof(long),0);

	memset(&msg,0,sizeof(msg_t));

	msgrcv(msgid,&msg,sizeof(msg_t)-sizeof(long), rettype, 0);//构建闭环控制
	
	/*获取用户ID ASCII 码求和值 作为等待的消息类型
	 * 避免发送与接收相同消息类型混淆*/
	printf("Content-type: text/html;charset=\"UTF-8\"\n\n");//固定格式 必须要加
	printf("<!DOCTYPE html>");
	printf("<html>");
	printf("<body>");
	printf("<center>");
	printf("<script>alert('提交成功'); window.location.href = '/greenhouse/garden_stuff.html';</script>");
	printf("</center>");
	printf("</body>");
	printf("</html>");
	return 0;

}