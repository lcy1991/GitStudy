#include "rtsp/MyRTSPHandler.h"
#include "rtsp/md5.h"
#include "rtsp/uuid.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <errno.h>


#define RTSP_PORT 554
#define LOG_TAG __FUNCTION__
const char* passwd = "abc";
const char* user = "abc";
const char* realm = "android";
const char* uri="/ch0/live";
MyRTSPHandler::MyRTSPHandler()
	: mRunningFlag(false),
	  session_id(0),
	  mtempSessionID(0)
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
				onReceiveRequest(msg);
			    break;
		}
}

void MyRTSPHandler::onReceiveRequest(const sp<AMessage> &msg)
{
	int32_t sessionID=0 ;
	ARTSPConnection* Conn;
	AString method;
	AString cseq;
	AString tmpStr;
	AString URI;
	int cseqNum;
	AString response;
	ReqMethod ReqMethodNum;
	uuid_t uuid;
	char randomID[33];
	map<uint32_t,ARTSPConnection*>::iterator iter;

	msg->findInt32("SessionID",&sessionID);
	LOGW(LOG_TAG,"findInt32 SessionID %d\n",sessionID);
	iter = mSessions.find(sessionID);	
	if(iter != mSessions.end()){
		Conn = iter->second;
	    if (Conn == NULL) {
	        LOGW(LOG_TAG,"failed to find session %d object ,it is NULL\n",sessionID);
	        mSessions.erase(iter);
	        return;
		}
	}
	else{
		LOGW(LOG_TAG,"There is no session ID %d\n",sessionID);
    	return;
	}
	msg->findString("Method",&method);
	msg->findString("CSeq",&cseq);
	cseqNum = atoi(cseq.c_str());
	LOGI(LOG_TAG,"findString Method %s CSeq %d URI:%s",method.c_str(),cseqNum,URI.c_str());
	do{
		if(strcmp(method.c_str(),"DESCRIBE")==0)
			{
				ReqMethodNum = DESCRIBE;
				msg->findString("URI",&URI);
				if(passwd!=""&&user!="")
					{
						if(msg->findString("Authorization",&tmpStr)==false)
							{
								uuid_generate(uuid);
								sprintf(randomID,"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
									uuid[0],uuid[1],uuid[2],uuid[3],uuid[4],uuid[5],uuid[6],uuid[7],
									uuid[8],uuid[9],uuid[10],uuid[11],uuid[12],uuid[13],uuid[14],uuid[15]);
								response.append("RTSP/1.0 401 Unauthorized\r\n");
								response.append("CSeq: ");
								response.append(cseqNum);
								response.append("\r\n");
								response.append("WWW-Authenticate: Digest realm=\"");
								response.append(realm);
								response.append("\",nonce=\"");
								response.append(randomID);
								response.append("\"\r\n\r\n");
								//LOGI(LOG_TAG,"%s",response.c_str());
								Conn->sendResponse(response.c_str());
							}
						else
							{
								
							}					
					}

				break;
			}
		if (strcmp(method.c_str(),"OPTIONS")==0)
			{
				response.append("RTSP/1.0 200 OK\r\n");
				response.append("CSeq: ");
				response.append(cseqNum);
				response.append("\r\n");
				response.append("Public: DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE\r\n\r\n");
				LOGI(LOG_TAG,"%s",response.c_str());
				Conn->sendResponse(response.c_str());
				ReqMethodNum = OPTION;
				break;
			}

		if (strcmp(method.c_str(),"SETUP")==0)
			{
				ReqMethodNum = SETUP;
				break;
			}
		if (strcmp(method.c_str(),"PLAY")==0)
			{
				ReqMethodNum = PLAY;
				break;
			}
		if (strcmp(method.c_str(),"PAUSE")==0)
			{
				ReqMethodNum = PAUSE;
				break;
			}
		if (strcmp(method.c_str(),"TEARDOWN")==0)
			{
				ReqMethodNum = TEARDOWN;
				break;
			}
			
		if (strcmp(method.c_str(),"SET_PARAMETER")==0)
			{
				ReqMethodNum = SET_PARAMETER;
				break;
			}
		if (strcmp(method.c_str(),"REDIRECT")==0)
			{
				ReqMethodNum = REDIRECT;
				break;
			}

	}while(0);

		
	
		
	
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
		    LOGI(LOG_TAG,"rtsp Start accept\n");
			mSocketAccept = accept(mSocket,NULL,NULL);
			mtempSessionID++;
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
	rtspConn->StartListen(handlerPt->mSocketAccept,handlerPt->id(),handlerPt->mtempSessionID);
	pthread_mutex_unlock(&handlerPt->mMutex);
	while(rtspConn->mState != DISCONNECTED)
		{
			sleep(1);
		}
	sp<AMessage> notify = new AMessage(kwhatCloseSession,handlerPt->id());
	notify->setInt32("sessionID",rtspConn->mSessionID);
	notify->post();
	LOGI(LOG_TAG,"session closed\n");
}




