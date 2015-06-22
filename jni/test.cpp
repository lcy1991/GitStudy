#include "handler_1.h"
#include "foundation/ALooper.h"
#include <unistd.h>

int main()
{
	uint32_t i;
	handler_1* handler1 = new handler_1();
	handler_1* handler2 = new handler_1();
	ALooper* looper =  new ALooper;
	looper->registerHandler(handler1);
	looper->registerHandler(handler2);
	handler1->setTarget(handler2->id());
	handler2->setTarget(handler1->id());
	looper->start();
	for(i=0;i<100;i++)
		{
			handler1->start(0,2*i);
			handler2->start(0,2*i+1);
			usleep(1000);
		}

	return 0;
}




