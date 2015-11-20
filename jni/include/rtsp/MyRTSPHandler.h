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
#include "rtsp/ARTSPConnection.h"
#include "rtsp/ARTPConnection.h"

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
	int getHostIP (char addressBuffer[40]) ;
	void makeSDP(uint8_t* SPS,uint32_t SPS_len,uint8_t* PPS,uint32_t PPS_len);
	bool mRunningFlag;
	void setRTPConnection(ARTPConnection* RTPConn);
	map<uint32_t,ARTSPConnection*> mSessions;//<session_id,ARTSPConnection pointer>
protected:
static void* NewSession(void* arg);
virtual void onMessageReceived(const sp<AMessage> &msg);

	void onReceiveRequest(const sp<AMessage> &msg);

	void onCloseSession(const sp<AMessage> &msg);
	
	void getDigest(const char* NONCE,const char* public_method,AString *result);

	int mSocket;
	int mSocketAccept;
    uint32_t session_id;
	ALooper mlooper;
	pthread_mutex_t mMutex;
	pthread_t mtempTID;
	int32_t mtempSessionID;
	AString mSDP;
private:
	    enum {
        kwhatCloseSession   = 'clse',
    };
	char mMD5part1[33];
	char mMD5part3[33];
	AString mURI;
	ARTPConnection* mRTPConnPt;
	bool isAuthenticate(const char* NONCE,AString& tmpStr,const char* method);

	void sendUnauthenticatedResponse(ARTSPConnection* Conn,int cseqNum);	
	
};

/*
Authorization: Digest username="test1", realm="Streaming Server", nonce="a72a7266d6f903a8384b874d3e29e666", uri="/sample_100kbit.mp4", response="f92167a7e9f64fb45d5dffd39f73f91f"\r\n

WWW-Authenticate: Digest realm="StreamingServer", nonce="a72a7266d6f903a8384b874d3e29e666"\r\n

*/
	void bumpSocketBufferSize(int s) {
			int size = 256 * 1024;
			CHECK_EQ(setsockopt(s, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)), 0);
	}
	
	unsigned MakePortPair(
			int *rtpSocket, int *rtcpSocket) 
	{
		unsigned port ;
		struct sockaddr_in addr;
		*rtpSocket = socket(AF_INET, SOCK_DGRAM, 0);
		CHECK_GE(*rtpSocket, 0);
	
		bumpSocketBufferSize(*rtpSocket);
	
		*rtcpSocket = socket(AF_INET, SOCK_DGRAM, 0);
		CHECK_GE(*rtcpSocket, 0);
	
		bumpSocketBufferSize(*rtcpSocket);
	
		unsigned start = (rand() * 1000)/ RAND_MAX + 15550;
		start &= ~1;
	
		for (port = start; port < 65536; port += 2) {
	
			addr.sin_port = htons(port);
	
			if (bind(*rtpSocket,
					 (const struct sockaddr *)&addr, sizeof(addr)) < 0) {
				continue;
			}
	
			addr.sin_port = htons(port + 1);
	
			if (bind(*rtcpSocket,
					 (const struct sockaddr *)&addr, sizeof(addr)) == 0) {
				break;//return port;
			}
		}
	
		addr.sin_port = htons(port);
		if (connect(*rtpSocket,(const struct sockaddr *)&addr, sizeof(addr)) < 0) {
			LOGE(LOG_TAG,"rtpSocket connect to client failed!");
			return -1;
		}
		
		addr.sin_port = htons(port + 1);
		if (connect(*rtcpSocket,(const struct sockaddr *)&addr, sizeof(addr)) < 0) {
			LOGE(LOG_TAG,"rtcpSocket connect to client failed!");
			return -1;
		}
		return port;// return rtp port and rtcp port
	
	}



#endif
