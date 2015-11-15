#include "handler_1.h"
#include "foundation/AString.h"
#include "foundation/ALooper.h"
#include <unistd.h>
#include "foundation/ADebug.h"
#include "rtsp/MyRTSPHandler.h"
#include "rtsp/ARTPSource.h"
#include "rtsp/ARTPConnection.h"


ARTPSource mysource(5,1500); 


void* sendbuf(void* agr)
{
	int i;
	sp<ABuffer> buf;
	char neirong[20];
	for(i=0;i<100;i++)
		{
			sprintf(neirong,"neirong %d",i);
			if(mysource.inputQPop(buf)>=0)
				{
					memcpy(buf->data(),neirong,strlen(neirong));
					buf->setRange(0,strlen(neirong));
					mysource.inputQPush(buf);				
				}
			else i--;
		}
}

void* getbuf(void* arg)
{
	int i;
	sp<ABuffer> buf;
	char neirong[20];
	for(i=0;i<200;i++)
		{
			//sprintf(neirong,"neirong %d",i);
			if(mysource.outputQPop(buf)>=0)
				{
			/*		if(buf->size()==0)
						{
							buf->setRange(0,0);
							mysource.outputQPush(buf);
							continue;
						}
			*/			
					memcpy(neirong,buf->data(),buf->size());
					printf("%s\n",neirong);
					buf->setRange(0,0);
					mysource.outputQPush(buf);
				}
		}	
}
int main()
{
	printf("abc\n");
#if 0
	uint32_t i;
	char* tmp = "1234567890\r\n1234567";
	handler_1* handler1 = new handler_1();
	handler_1* handler2 = new handler_1();
	ALooper* looper1 =  new ALooper;
	ALooper* looper2 =  new ALooper;
	AString* astring = new AString(tmp,19);
	ssize_t space1 = astring->find("\r\n");
	LOGI("log_tag","status: %s %d\n", astring->c_str(),space1);
		//return 0;
	looper1->registerHandler(handler1);
	looper2->registerHandler(handler2);
	handler1->setTarget(handler2->id());
	handler2->setTarget(handler1->id());
	looper1->start();
	looper2->start();
	handler1->start(0,0);
/*	for(i=0;i<100;i++)
		{
			handler1->start(0,2*i);
			handler2->start(0,2*i+1);
			usleep(1000);
		}*/
	sleep(1);
	delete handler1;
	delete handler2;
	delete looper1;
	delete looper2;

	MyRTSPHandler handler_rtsp;
	handler_rtsp.StartServer();
#elseif 0

pthread_t idsend;
pthread_t idget;

int i,ret;


ret=pthread_create(&idsend,NULL,sendbuf,NULL);

ret=pthread_create(&idget,NULL,getbuf,NULL);

//sleep(1);
void* status;
pthread_join(idsend,&status);
pthread_join(idget,&status);

#else

ARTPConnection* RTPConn = new ARTPConnection();
ALooper* looper1 =	new ALooper;
looper1->registerHandler(RTPConn);
looper1->start();
struct sockaddr_in address;//处理网络通信的地址  
int rtpsock;
int rtcpsock;

bzero(&address,sizeof(address));	
address.sin_family=AF_INET;  
address.sin_addr.s_addr=inet_addr("127.0.0.1");//这里不一样  
address.sin_port=htons(6789); 
RTPConn->setSource(&mysource);

//MakePortPair(&rtpsock,&rtcpsock,address);
printf("MakePortPair %d\n",MakePortPair(&rtpsock,&rtcpsock,address));
RTPConn->addStream(rtpsock,rtcpsock,0,&address);

sp<ABuffer> tmpbuf;

uint8_t testbuf[1300];
testbuf[0]=0x65;
testbuf[0]=0x88;	
testbuf[0]=0x20;
testbuf[0]=0x00;
for(int i = 0; i< 100 ;i++)
{
	if(mysource.inputQPop(tmpbuf)>=0)
		{
			memcpy(tmpbuf->data(),testbuf,1300);
			tmpbuf->setRange(0,1300);
			mysource.inputQPush(tmpbuf);
		}
}




#endif

	return 0;
}




