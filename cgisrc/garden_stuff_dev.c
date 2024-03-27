#include <../include/myhead.h>

long rettype;	//消息队列接收类型

void handler(int argc)
{
	msgrcv(msgid,&msg,sizeof(msg_t)-sizeof(long),rettype/2,IPC_NOWAIT);

	printf("Content-type: text/html;charset=\"UTF-8\"\n\n");//固定格式 必须要加
	printf("<script>alert('网络原因导致数据不可达，请稍后重试'); window.location.href = '/greenhouse/garden_stuff.html';</script>");
	exit(0);
}

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

	/*设置信号处理函数*/
	signal(SIGALRM,handler);

	char name[128] = {0};
	memcpy(name,cgiCookie,128);
	char *s = name;
	char buf[128] = {0};

	while(*s!='='){
		s++;	
	}
	s++;
	long msgtype = 0;
	int i = 0;
	memset(&msg,0,sizeof(msg_t));
	while(*s){
		buf[i] = *s;
		msg.msgtype+=*s;
		s++;
		i++;
	}

	/*封装消息*/
	rettype = msg.msgtype*2;
	msg.commd = 3;//获取环境数据
	
	char data[10] = {0};

	/*获取网页数据*/
	cgiFormString("LED",data,10);
	if(!strcmp("ON",data)){
		msg.devctrl |= 0x01<<0;
	}else{
		msg.devctrl &= ~(0x01<<0);
	}

	memset(data,0,sizeof(data));
	cgiFormString("FAN",data,2);
	switch(data[0]){
		case '0':
		msg.devctrl &= ~(0x03<<1);
			break;
		case '1':
		msg.devctrl &= ~(0x03<<1);
		msg.devctrl |= (0x01<<1);
			break;
		case '2':
		msg.devctrl &= ~(0x03<<1);
		msg.devctrl |= (0x02<<1);
			break;
		case '3':
		msg.devctrl &= ~(0x03<<1);
		msg.devctrl |= (0x03<<1);
			break;
		default:
		msg.devctrl &= ~(0x03<<1);
			break;
	}

	memset(data,0,sizeof(data));
	cgiFormString("BEEP",data,10);
	if(!strcmp("ON",data)){
		msg.devctrl |= (0x01<<3);
	}else{
		msg.devctrl &= ~(0x01<<3);
	}

	msgsnd(msgid,&msg,sizeof(msg_t)-sizeof(long),0);
	alarm(5);//设置闹钟时间

	memset(&msg,0,sizeof(msg_t));
	msgrcv(msgid,&msg,sizeof(msg_t)-sizeof(long),rettype,0);

	if(1==msg.user.flags){
		printf("Content-type: text/html;charset=\"UTF-8\"\n\n");//固定格式 必须要加
		printf("<script>alert('提交成功'); window.location.href = '/greenhouse/garden_stuff.html';</script>");
	}else{
		printf("Content-type: text/html;charset=\"UTF-8\"\n\n");//固定格式 必须要加
		printf("<script>alert('网络原因导致数据不可达，请稍后重试'); window.location.href = '/greenhouse/garden_stuff.html';</script>");
	}

	return 0;
}