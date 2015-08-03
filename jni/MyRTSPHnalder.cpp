#include "rtsp/MyRTSPHandler.h"
#define RTSP_PORT 554
MyRTSPHandler::MyRTSPHandler()
	: mRunningFlag(false),
{

}
MyRTSPHandler::~MyRTSPHandler()
{

}

void MyRTSPHandler::onMessageReceived(const sp<AMessage> &msg)
{
	switch(msg->kwhat())
		{
			case:
		}
}

void MyRTSPHandler::StartServer()
{
	mlooper.registerHandler(this);
	mlooper.start();
    mSocket = socket(AF_INET, SOCK_STREAM, 0);
	mRunningFlag = true;
    MakeSocketBlocking(mSocket, false);

    struct sockaddr_in server_addr;
    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htons(INADDR_ANY); ;
    server_addr.sin_port = htons(RTSP_PORT);
	if(-1 == (bind(mSocket, (struct sockaddr*)&server_addr, sizeof(server_addr)))) 
	  { 
		LOGE(LOG_TAG,"Server Bind Failed:"); 
		//exit(1); 
		return ;
	  } 
	// socket监听 
	err = listen(mSocket, BACKLOG);//使主动连接套接字变成监听套接字
	if(err < 0){

		//mState = DISCONNECTED;
	}
	while(mRunningFlag)
		{
		   //mSocketAccept需要锁保护
		    pthread_mutex_lock(&mMutex);
		    mtempSessionID++;
			mSocketAccept = accept(mSocket,NULL,NULL);
			pthread_create(&mTID,NULL,NewSession,(void*)this);		
			LOGE(LOG_TAG,"mConnectedNum[%d]",mConnectionID);			
		}

}

/*
session_id
socket_fd
status
*/
void MyRTSPHandler::StopServer()
{
	mRunningFlag = false;
}

static void MyRTSPHandler::NewSession(void* arg)
{
	MyRTSPHandler* handlerPt = (MyRTSPHandler*)arg;
	ARTSPConnection* rtspConn = new ARTSPConnection();
	handlerPt->mlooper.registerHandler(rtspConn);
	rtspConn->StartListen(handlerPt->mSocketAccept,handlerPt->id(),mtempSessionID);
	pthread_mutex_unlock(&mMutex);
	while(rtspConn->)
		{
			sleep(1);
		}
	delete(rtspConn);
	LOGI(LOG_TAG,"session closeed\n");
}

