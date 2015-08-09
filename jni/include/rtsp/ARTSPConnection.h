/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef A_RTSP_CONNECTION_H_

#define A_RTSP_CONNECTION_H_

#include "foundation/AHandler.h"
#include "foundation/AString.h"
#include "foundation/LightRefBase.h"
#include "foundation/StrongPointer.h"

#include <map>
#include <pthread.h>

struct ABuffer;

enum State {
	DISCONNECTED,
	CONNECTING,
	CONNECTED,
	INIT,
	READY,
	PLAYING,	
};

enum {
	kWhatListening			= 'list',
	kWhatDisconnect 		= 'disc',
	kWhatStartListen		= 'star',
	kWhatCompleteConnection = 'comc',
	kWhatSendResponse		= 'sres',
	kWhatReceiveResponse	= 'rres',
	kWhatReceiveRequest 	= 'rreq',
	kWhatObserveBinaryData	= 'obin',
	kWhatRequest			= 'requ',
};



struct ARTSPResponse : public LightRefBase<ARTSPResponse> {            //响应消息格式
    unsigned long mStatusCode;                     //状态码
    AString mStatusLine;                            
    map<AString,AString> mHeaders;                 //消息头 
    sp<ABuffer> mContent;                          //消息体
};

struct ARTSPRequest : public LightRefBase<ARTSPRequest> {              //请求消息格式
//请求行: 方法＋空格＋URI(Uniform Resource Identifier)＋空格＋RTSP版本+回车换行
//e.g.  DESCRIBE rtsp://172.16.42.219/channel1/live RTSP/1.0\r\n
    AString mRequestLine;                          //请求行
//消息头:消息内容
	map<AString,AString> mHeaders;                 //消息头 
    sp<ABuffer> mContent;                          //消息体
};


struct ARTSPConnection : public AHandler {
    ARTSPConnection();
	void listening(const sp<AMessage> &reply);
    void connect(const char *url, const sp<AMessage> &reply);
    void disconnect(const sp<AMessage> &reply);

    void sendResponse(const char *responses);

    void observeBinaryData(const sp<AMessage> &reply);
	
	void postReceiveRequestEvent();
	void StartListen(int socket,handler_id handlerID,uint32_t mtempSessionID);	
    State mState;
    int32_t mSessionID;
//	int mSocketAccept;
    int mSocket;	
	pthread_t mTID;

protected:
    virtual ~ARTSPConnection();
    virtual void onMessageReceived(const sp<AMessage> &msg);

private:
    enum AuthType {
        NONE,
        BASIC,
        DIGEST
    };

    static const int64_t kSelectTimeoutUs;


	uint32_t mConnectedNum;
	map<pthread_t, int> mConnectSocket;//<pthread_t_ID,socketfd>
    AString mUser, mPass;
    AuthType mAuthType;
    AString mNonce;

	int mSocket_listen;
	int mSocket_client;


	handler_id mhandlerID;// request msg target
    int32_t mNextCSeq;
	bool mReceiveRequestEventPending;

    map<int32_t, sp<AMessage> > mPendingRequests;

    sp<AMessage> mObserveBinaryMessage;
	sp<AMessage> mRequestReturn;
    void onDisconnect(const sp<AMessage> &msg);
    void onCompleteConnection(const sp<AMessage> &msg);
    void onSendResponse(const sp<AMessage> &msg);
    void onReceiveRequest();

//    void flushPendingRequests();
//    void postReceiveReponseEvent();

    // Return false iff something went unrecoverably wrong.
    bool receiveRTSPReponse();
    status_t receive(void *data, size_t size);
    bool receiveLine(AString *line);
    sp<ABuffer> receiveBinaryData();
	bool receiveRTSPRequest();
    bool notifyResponseListener(const sp<ARTSPResponse> &response);

    bool parseAuthMethod(const sp<ARTSPResponse> &response);
    void addAuthentication(AString *request);

    status_t findPendingRequest(
            const sp<ARTSPResponse> &response, ssize_t *index) const;

    static bool ParseSingleUnsignedLong(
            const char *from, unsigned long *x);

    DISALLOW_EVIL_CONSTRUCTORS(ARTSPConnection);
};


#endif  // A_RTSP_CONNECTION_H_
