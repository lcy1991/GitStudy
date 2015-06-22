#include "handler_1.h"
#include "foundation/ADebug.h"
#define LOG_TAG __FILE__


handler_1::handler_1()
{
	
}

handler_1::~handler_1()
{
	
}
void handler_1::setTarget(handler_id Target)
{
	mTarget = Target;
}

void handler_1::start(uint32_t mWhat,uint32_t startNum)
{
	
	AMessage* msg = new AMessage(mWhat, mTarget);
	msg->setInt32("hello",startNum);
	msg->post(100);
}

void handler_1::onMessageReceived(AMessage * msg)
{
	int32_t value;
	int32_t what;
	AMessage* msg_send;
	bool ret;
	what = msg->what();
	if(what == 0)
		{
			if(msg->findInt32("hello",&value))
				{
					LOGI(LOG_TAG,"received the num %d message hello %d\n",what,value);
				}
		}
}



