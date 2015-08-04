#include "rtsp/MyRTSPHandler.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <openssl/md5.h>
#include <sys/socket.h>
#include <errno.h>


#define RTSP_PORT 554
#define LOG_TAG __FUNCTION__
MyRTSPHandler::MyRTSPHandler()
	: mRunningFlag(false),
	  session_id(0)
{

}
MyRTSPHandler::~MyRTSPHandler()
{

}

void MyRTSPHandler::onMessageReceived(const sp<AMessage> &msg)
{
	switch(msg->what())
		{
			case kwhatCloseSession:
				break;
			case kWhatRequest:
				LOGI(LOG_TAG,"MyRTSPHandler receive client request\n");
			    break;
		}
}

static void MakeSocketBlocking(int s, bool blocking) {
    // Make socket non-blocking.
    int flags = fcntl(s, F_GETFL, 0);
    CHECK_NE(flags, -1);

    if (blocking) {
        flags &= ~O_NONBLOCK;
    } else {
        flags |= O_NONBLOCK;
    }

    CHECK_NE(fcntl(s, F_SETFL, flags), -1);
}

void MyRTSPHandler::StartServer()
{
	mlooper.registerHandler(this);
	mlooper.start();
    mSocket = socket(AF_INET, SOCK_STREAM, 0);
	mRunningFlag = true;
    MakeSocketBlocking(mSocket, true);

    struct sockaddr_in server_addr;
    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htons(INADDR_ANY); ;
    server_addr.sin_port = htons(RTSP_PORT);
	LOGI(LOG_TAG,"rtsp StartServer\n");
	if(-1 == (bind(mSocket, (struct sockaddr*)&server_addr, sizeof(server_addr)))) 
	  { 
		LOGE(LOG_TAG,"Server Bind Failed:"); 
		//exit(1); 
		return ;
	  } 
	// socket监听
	int err;
	err = listen(mSocket, 15);//使主动连接套接字变成监听套接字
	if(err < 0){

		//mState = DISCONNECTED;
	}
	LOGI(LOG_TAG,"rtsp Start listening\n");
	while(mRunningFlag)
		{
		   //mSocketAccept需要锁保护
		    pthread_mutex_lock(&mMutex);
		    mtempSessionID++;
		    LOGI(LOG_TAG,"rtsp Start accept\n");
			mSocketAccept = accept(mSocket,NULL,NULL);
			pthread_create(&mtempTID,NULL,NewSession,(void *)this);		
			LOGE(LOG_TAG,"mtempSessionID[%d]\n",mtempSessionID);			
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

void* MyRTSPHandler::NewSession(void* arg)
{
	MyRTSPHandler* handlerPt = (MyRTSPHandler*)arg;
	ARTSPConnection* rtspConn = new ARTSPConnection();
	handlerPt->mSessions.insert(make_pair(handlerPt->mtempSessionID, rtspConn));
	handlerPt->mlooper.registerHandler(rtspConn);
	rtspConn->StartListen(handlerPt->mSocketAccept,handlerPt->mID,handlerPt->mtempSessionID);
	pthread_mutex_unlock(&handlerPt->mMutex);
	while(rtspConn->mState != DISCONNECTED)
		{
			sleep(1);
		}
	sp<AMessage> notify = new AMessage(kwhatCloseSession,handlerPt->mID);
	notify->setInt32("sessionID",rtspConn->mSessionID);
	notify->post();
	LOGI(LOG_TAG,"session closed\n");
}

