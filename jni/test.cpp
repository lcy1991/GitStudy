#include "handler_1.h"
#include "foundation/AString.h"
#include "foundation/ALooper.h"
#include <unistd.h>
#include "foundation/ADebug.h"
#include "rtsp/MyRTSPHandler.h"

int main()
{
	uint32_t i;
	char* tmp = "1234567890";
	handler_1* handler1 = new handler_1();
	handler_1* handler2 = new handler_1();
	ALooper* looper1 =  new ALooper;
	ALooper* looper2 =  new ALooper;
	AString* astring = new AString(tmp,3);
	ssize_t space1 = astring->find("3");
	LOGI("log_tag","status: %s %d\n", astring->c_str(),space1);
	looper1->registerHandler(handler1);
	looper2->registerHandler(handler2);
	handler1->setTarget(handler2->id());
	handler2->setTarget(handler1->id());
	handler1->start(0,0);
	looper1->start();
	looper2->start();
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
	


	
	return 0;
}




