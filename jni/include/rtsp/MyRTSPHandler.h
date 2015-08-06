#ifndef MYRTSPHANDLER_H
#define MYRTSPHANDLER_H


#include<netinet/in.h> // sockaddr_in 
#include<sys/types.h>  // socket 
#include<sys/socket.h> // socket 
#include<stdio.h>    // printf 
#include<stdlib.h>   // exit 
#include<string.h>   // bzero 
#include<pthread.h>
#include<map>
#include "foundation/AHandler.h"
#include "foundation/AMessage.h"
#include "foundation/ADebug.h"
#include "ARTSPConnection.h"

enum ReqMethod
{
	OPTION,
	DESCRIBE,
	SETUP,
	PLAY,
	PAUSE,
	TEARDOWN,
	SET_PARAMETER,
	REDIRECT

};

struct MyRTSPHandler : public AHandler
{
	void StartServer();
	void StopServer();
	MyRTSPHandler();
    virtual ~MyRTSPHandler();
	bool mRunningFlag;
	map<uint32_t,ARTSPConnection*> mSessions;//<session_id,ARTSPConnection pointer>
protected:
static void* NewSession(void* arg);
virtual void onMessageReceived(const sp<AMessage> &msg);
	void onReceiveRequest(const sp<AMessage> &msg);
	int mSocket;
	int mSocketAccept;
    uint32_t session_id;
	ALooper mlooper;
	pthread_mutex_t mMutex;
	pthread_t mtempTID;
	int32_t mtempSessionID;
private:
	    enum {
        kwhatCloseSession   = 'clse',
    };

	
};

#endif
