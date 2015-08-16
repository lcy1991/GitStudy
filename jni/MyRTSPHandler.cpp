#include "rtsp/MyRTSPHandler.h"
#include "our_md5.h"
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
//向服务器发送心跳包
MyRTSPHandler::MyRTSPHandler()
	: mRunningFlag(false),
	  session_id(0),
	  mtempSessionID(0)
{
	AString tmpStr;
	char hostip[40];
	getHostIP(hostip);
	mURI.append("rtsp://");
	mURI.append(hostip);
	mURI.append(uri);
	LOGI(LOG_TAG,"uri--->%s",mURI.c_str());
	tmpStr.append(user);
	tmpStr.append(":");
	tmpStr.append(realm);
	tmpStr.append(":");
	tmpStr.append(passwd);
	//md5(username:realm:password)
	MD5_encode(tmpStr.c_str(),mMD5part1);
}
MyRTSPHandler::~MyRTSPHandler()
{

}

void MyRTSPHandler::onMessageReceived(const sp<AMessage> &msg)
{
	switch(msg->what())
		{
			case kwhatCloseSession:
				onCloseSession(msg);
				break;
			case kWhatRequest:
				LOGI(LOG_TAG,"MyRTSPHandler receive client request\n");
				onReceiveRequest(msg);
			    break;
		}
}
void MyRTSPHandler::onCloseSession(const sp<AMessage> &msg)
{
	int32_t sessionID = 0 ;
	int32_t result = 0 ;
	ARTSPConnection* Conn;
//	return;
	map<uint32_t,ARTSPConnection*>::iterator iter;
	msg->findInt32("sessionID",&sessionID);
	msg->findInt32("result",&result);
	iter = mSessions.find(sessionID);	
	if(iter != mSessions.end()){
		Conn = iter->second;
	    if (Conn != NULL) {
			mlooper.unregisterHandler(Conn->id());
			delete Conn;
	        LOGI(LOG_TAG,"Session %d is deleted ,result %d",sessionID,result);
	        mSessions.erase(iter);
		}
	}
	else LOGI(LOG_TAG,"Can not find the session %d!",sessionID);
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
//	LOGW(LOG_TAG,"findInt32 SessionID %d\n",sessionID);
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
	
	do{
		if(strcmp(method.c_str(),"DESCRIBE")==0)
			{
				ReqMethodNum = DESCRIBE;
				msg->findString("URI",&URI);
				LOGI(LOG_TAG,"findString Method %s CSeq %d URI:%s",method.c_str(),cseqNum,URI.c_str());
				if(passwd!=""&&user!="")
					{
						if(msg->findString("Authorization",&tmpStr)==false)//no Authorization
							{
								sendUnauthenticatedResponse(Conn,cseqNum);
							}
						else
							{
								if(!isAuthenticate(Conn->getNonce(),tmpStr))
									sendUnauthenticatedResponse(Conn,cseqNum);
									
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
		    //pthread_mutex_lock(&mMutex);
		    LOGI(LOG_TAG,"rtsp Start accept\n");
			mSocketAccept = accept(mSocket,NULL,NULL);
			mtempSessionID++;
			
			ARTSPConnection* rtspConn = new ARTSPConnection();
			mSessions.insert(make_pair(mtempSessionID, rtspConn));
			mlooper.registerHandler(rtspConn);
			rtspConn->StartListen(mSocketAccept,id(),mtempSessionID);


			
			//pthread_create(&mtempTID,NULL,NewSession,(void *)this);		
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
	LOGI(LOG_TAG,"session %d is closed\n",rtspConn->mSessionID);
}

// response= md5(md5(username:realm:password):nonce:md5(public_method:url));
void MyRTSPHandler::getDigest(const char* NONCE,const char* public_method,AString *result)
{
	AString tmpStr;
	AString part3;
	char part3_md5[33];
	char ret[33];
	tmpStr.append(mMD5part1);
	//LOGI(LOG_TAG,"mMD5part1:%s",mMD5part1);
	tmpStr.append(":");
	tmpStr.append(NONCE);
	//LOGI(LOG_TAG,"NONCE:%s",NONCE);
	tmpStr.append(":");
	part3.append(public_method);
	part3.append(":");
	part3.append(mURI);
	MD5_encode(part3.c_str(),part3_md5);
	//LOGI(LOG_TAG,"part3_md5:%s",part3_md5);
	
	tmpStr.append(part3_md5);
	//LOGI(LOG_TAG,"[response]:%s",tmpStr.c_str());
	MD5_encode(tmpStr.c_str(),ret);
	
	result->setTo(ret);
	LOGI(LOG_TAG,"Digest result:%s",ret);
}


#ifdef ANDROID   
int MyRTSPHandler::getHostIP (char addressBuffer[40]) 
{
	sprintf(addressBuffer,"%s","192.168.1.100");
	return 0;
}

#else
#include <ifaddrs.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

int MyRTSPHandler::getHostIP(char addressBuffer[40]) 
{

    struct ifaddrs * ifAddrStruct=NULL;
    void * tmpAddrPtr=NULL;

    getifaddrs(&ifAddrStruct);

    while (ifAddrStruct!=NULL) 
	{
        if (ifAddrStruct->ifa_addr->sa_family==AF_INET)
		{   // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr = &((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
			if(strcmp(ifAddrStruct->ifa_name,"eth0")==0)
				{
					inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            		//printf("%s IPV4 Address %s\n", ifAddrStruct->ifa_name, addressBuffer); 
           			break;
				}

        }
		else if (ifAddrStruct->ifa_addr->sa_family==AF_INET6)
		{   // check it is IP6
            // is a valid IP6 Address
            //tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
            //char addressBuffer[INET6_ADDRSTRLEN];
            //inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
            //printf("%s IPV6 Address %s\n", ifAddrStruct->ifa_name, addressBuffer); 
        } 
        ifAddrStruct = ifAddrStruct->ifa_next;
    }
    return 0;
}
#endif


bool MyRTSPHandler::isAuthenticate(const char* NONCE,AString& tmpStr)
{
	AString Digest;
	ssize_t resp;
	resp = tmpStr.find("response");
	
	AString response(tmpStr, resp + 10, 32);
	LOGI(LOG_TAG,"~~~Client's response %s",response.c_str());
	getDigest(NONCE,"DESCRIBE",&Digest);
	if(Digest==response)
		{
			LOGI(LOG_TAG,"Authenticate passed !!!!");
			return true;
		}
	else return false;
}

void MyRTSPHandler::sendUnauthenticatedResponse(ARTSPConnection* Conn,int cseqNum)
{
	AString response;
	uuid_t uuid;
	char randomID[33];
	uuid_generate(uuid);
	sprintf(randomID,"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
		uuid[0],uuid[1],uuid[2],uuid[3],uuid[4],uuid[5],uuid[6],uuid[7],
		uuid[8],uuid[9],uuid[10],uuid[11],uuid[12],uuid[13],uuid[14],uuid[15]);
	Conn->setNonce(randomID);
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



