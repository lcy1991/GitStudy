#ifndef MYRTSPHANDLER_H
#define MYRTSPHANDLER_H


#include<netinet/in.h> // sockaddr_in 
#include<sys/types.h>  // socket 
#include<sys/socket.h> // socket 
#include<stdio.h>    // printf 
#include<stdlib.h>   // exit 
#include<string.h>   // bzero 

#include "foundation/AHandler.h"
#include "foundation/AMessage.h"
struct MyRTSPHandler : public AHandler
{
	void StartServer();
	bool mRunningFlag;
protected:
    virtual ~MyRTSPHandler();
    virtual void onMessageReceived(const sp<AMessage> &msg);
	int mSocket;
	int mSocketAccept;
	uint32_t session_id=0;
	ALooper mlooper;

	
};

#endif
