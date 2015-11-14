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

//#define LOG_NDEBUG 0
#define LOG_TAG "ARTPConnection"


#include "rtsp/ARTPConnection.h"

#include "rtsp/ARTPSource.h"
//#include "ASessionDescription.h"

#include <foundation/ABuffer.h>
#include <foundation/ADebug.h>
#include <foundation/AMessage.h>
#include <foundation/AString.h>


#include <arpa/inet.h>
#include <sys/socket.h>


const uint8_t ff_ue_golomb_vlc_code[512]=
{ 32,32,32,32,32,32,32,32,31,32,32,32,32,32,32,32,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,
   7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9,10,10,10,10,11,11,11,11,12,12,12,12,13,13,13,13,14,14,14,14,
   3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
   5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
   2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  };


static const size_t kMaxUDPSize = 1500;

static uint16_t u16at(const uint8_t *data) {
    return data[0] << 8 | data[1];
}

static uint32_t u32at(const uint8_t *data) {
    return u16at(data) << 16 | u16at(&data[2]);
}

static uint64_t u64at(const uint8_t *data) {
    return (uint64_t)(u32at(data)) << 32 | u32at(&data[4]);
}

// static
const int64_t ARTPConnection::kSelectTimeoutUs = 1000ll;



ARTPConnection::ARTPConnection(uint32_t flags)
    : mFlags(flags),
      mPollEventPending(false),
      mLastReceiverReportTimeUs(-1) {
}

ARTPConnection::~ARTPConnection() {
}

void ARTPConnection::addStream(
        int rtpSocket, int rtcpSocket,
        size_t index,
        const sp<AMessage> &notify,
        bool injected) {
    sp<AMessage> msg = new AMessage(kWhatAddStream, id());
    msg->setInt32("rtp-socket", rtpSocket);
    msg->setInt32("rtcp-socket", rtcpSocket);
    msg->setSize("index", index);
    msg->setMessage("notify", notify);
    msg->setInt32("injected", injected);
    msg->post();
}

void ARTPConnection::removeStream(int rtpSocket, int rtcpSocket) {
    sp<AMessage> msg = new AMessage(kWhatRemoveStream, id());
    msg->setInt32("rtp-socket", rtpSocket);
    msg->setInt32("rtcp-socket", rtcpSocket);
    msg->post();
}

static void bumpSocketBufferSize(int s) {
    int size = 256 * 1024;
    CHECK_EQ(setsockopt(s, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)), 0);
}


// static
/**/
unsigned ARTPConnection::MakePortPair(
        int *rtpSocket, int *rtcpSocket, struct sockaddr_in addr) {
    unsigned port ;
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

void startSendPacket()
{
	
}

void ARTPConnection::onMessageReceived(const sp<AMessage> &msg) {
    switch (msg->what()) {
        case kWhatAddStream:
        {
            onAddStream(msg);
            break;
        }

        case kWhatSendPacket:
        {
            onSendPacket(msg);
            break;
        }		

        case kWhatRemoveStream:
        {
            onRemoveStream(msg);
            break;
        }

        case kWhatInjectPacket:
        {
            onInjectPacket(msg);
            break;
        }
        default:
        {
            TRESPASS();
            break;
        }
    }
}

void ARTPConnection::onAddStream(const sp<AMessage> &msg) {
    mStreams.push_back(StreamInfo());
    StreamInfo *info = &*--mStreams.end();

    int32_t s;
    CHECK(msg->findInt32("rtp-socket", &s));
    mStreams.mRTPSocket = s;
    CHECK(msg->findInt32("rtcp-socket", &s));
    mStreams.mRTCPSocket = s;

    CHECK(msg->findSize("index", &mStreams.mIndex));
    CHECK(msg->findMessage("notify", &mStreams.mNotifyMsg));

    memset(&mStreams.mRemoteRTCPAddr, 0, sizeof(mStreams.mRemoteRTCPAddr));

}

void ARTPConnection::onRemoveStream(const sp<AMessage> &msg) {
    int32_t rtpSocket, rtcpSocket;
    CHECK(msg->findInt32("rtp-socket", &rtpSocket));
    CHECK(msg->findInt32("rtcp-socket", &rtcpSocket));

    list<StreamInfo>::iterator it = mStreams.begin();
    while (it != mStreams.end()
           && (it->mRTPSocket != rtpSocket || it->mRTCPSocket != rtcpSocket)) {
        ++it;
    }

    if (it == mStreams.end()) {
        TRESPASS();
    }

    mStreams.erase(it);
}


void ARTPConnection::onSendPacket(const sp<AMessage> &msg) {
	
	
}

void ARTPConnection::makeRTPHeader(RTPHeader* h,uint8_t out[12])
{
	uint16_t tmp16;
	uint32_t tmp32;
	out[0] = h->v << 6 | h->p << 5 | h->x << 4 | h->cc;
	out[1] = h->m << 7 | h->pt;
	tmp16 = htons(h->seq);
	out[2] = tmp16 >> 8;
	out[3] = tmp16;
	tmp32 = htonl(h->timestamp);
	out[4] = tmp32 >> 24;
	out[5] = tmp32 >> 16;
	out[6] = tmp32 >> 8;
	out[7] = tmp32;
	tmp32 = htonl(h->ssrc);
	out[8] = tmp32 >> 24;
	out[9] = tmp32 >> 16;
	out[10] = tmp32 >> 8;
	out[11] = tmp32;	
	
}
/*
nal < MUT 1460
nal > MUT

*/

bool ARTPConnection::isFirstNalu(uint8_t* data,size_t length) {

	uint8_t vlu;
	uint32_t BUF[4];
	uint32_t code;
	if(length < 5)
		{
			LOGE(LOG_TAG,"golomb decode error! buf length < 5");
			return false;
		}
	BUF[0] = data[1];
	BUF[1] = data[2];
	BUF[2] = data[3];
	BUF[3] = data[4];
	code = BUF[0]<<24 | BUF[1]<<16 | BUF[2]<<8 | BUF[3];
	if(code >= (1<<27)){                    //该编码为9bit以内
		code >>= 32 - 9;                    //将buf右移23bit得到比特串
		vlu = ff_ue_golomb_vlc_code[code];
	}
	else LOGE(LOG_TAG,"golomb decode error!");
	if(vlu == 0) return true;
	else return false;

}



status_t ARTPConnection::sendRTP(StreamInfo *s,const sp<ABuffer> &buffer) {

    ssize_t nbytes = send(
            s->mRTPSocket,
            buffer->data(),
            buffer->capacity(),
            0,);

    if (nbytes < 0) {
        return -1;
    }

    LOGI(LOG_TAG,"send %d bytes.", buffer->size());

    //return err;
}



void ARTPConnection::setSource(ARTPSource* src)
{
	mRTPSource = src;
}

status_t ARTPConnection::RTPPacket(sp<ABuffer> buf)
{
	static uint32_t timeStamp = 0;
	static uint16_t seqNum;
	uint32_t bytes;
	//static sp<ABuffer> preBuf = new ABuffer(MTU_SIZE);
	//sp<ABuffer> swapBuf;
	char* nalu_payload;
	//RTP header 12 bytes
	if(isFirstNalu(buf->data(),buf->size()))
		timeStamp+=(unsigned int)(90000.0 / mRTPSource->mFramerate);
	NALU_t nalu;

while(1) 
{
	memset(&nalu,0,sizeof(NALU_t));
	GetAnnexbNALU(&nalu,buf);//每执行一次，文件的指针指向本次找到的NALU的末尾，下一个位置即为下个NALU的起始码0x000001
	dump(&nalu);//输出NALU长度和TYPE

	//memset(sendbuf,0,1500);//清空sendbuf；此时会将上次的时间戳清空，因此需要ts_current来保存上次的时间戳值

	//rtp固定包头，为12字节,该句将sendbuf[0]的地址赋给rtp_hdr，以后对rtp_hdr的写入操作将直接写入sendbuf。
	rtp_hdr =(RTP_FIXED_HEADER*)&sendbuf[0]; 
	
	//设置RTP HEADER，
	rtp_hdr->version = 2;	//版本号，此版本固定为2
	rtp_hdr->marker  = 0;	//标志位，由具体协议规定其值。
	rtp_hdr->payload = 96;//H264;//负载类型号，
	rtp_hdr->ssrc	 = htonl(10);//随机指定为10，并且在本RTP会话中全局唯一

	//当一个NALU小于1400字节的时候，采用一个单RTP包发送
	if(nalu.len <= UDP_MAX_SIZE){
		//设置rtp M 位；
		rtp_hdr->marker=1;
		rtp_hdr->seq_no = htons(seqNum ++); //序列号，每发送一个RTP包增1
		rtp_hdr->timestamp=htonl(timeStamp);
		//设置NALU HEADER,并将这个HEADER填入sendbuf[12]
		nalu_hdr =(NALU_HEADER*)&sendbuf[12]; //将sendbuf[12]的地址赋给nalu_hdr，之后对nalu_hdr的写入就将写入sendbuf中；
		nalu_hdr->F=nalu.forbidden_bit;
		nalu_hdr->NRI=nalu.nal_reference_idc>>5;//有效数据在n->nal_reference_idc的第6，7位，需要右移5位才能将其值赋给nalu_hdr->NRI。
		nalu_hdr->TYPE=nalu.nal_unit_type;

		nalu_payload=&sendbuf[13];//同理将sendbuf[13]赋给nalu_payload
		memcpy(nalu_payload,nalu.buf+1,nalu.len-1);//去掉nalu头的nalu剩余内容写入sendbuf[13]开始的字符串。

		//timeStamp = timeStamp + timestamp_increse;
		
		bytes = nalu.len + 12 ; //获得sendbuf的长度,为nalu的长度（包含NALU头但除去起始前缀）加上rtp_header的固定长度12字节
		//send(socket1,sendbuf,bytes,0);//发送RTP包
		//Sleep(100);
	}else{
		int packetNum = nalu.len/UDP_MAX_SIZE;
		if (n->len%UDP_MAX_SIZE != 0)
			packetNum ++;

		int lastPackSize = nalu.len - (packetNum-1)*UDP_MAX_SIZE;
		int packetIndex = 1 ;

		//timeStamp = timeStamp + timestamp_increse;

		rtp_hdr->timestamp = htonl(timeStamp);

		//发送第一个的FU，S=1，E=0，R=0

		rtp_hdr->seq_no = htons(seqNum++); //序列号，每发送一个RTP包增1
		//设置rtp M 位；
		rtp_hdr->marker=0;
		//设置FU INDICATOR,并将这个HEADER填入sendbuf[12]
		fu_ind =(FU_INDICATOR*)&sendbuf[12]; //将sendbuf[12]的地址赋给fu_ind，之后对fu_ind的写入就将写入sendbuf中；
		fu_ind->F=nalu.forbidden_bit;
		fu_ind->NRI=nalu.nal_reference_idc>>5;
		fu_ind->TYPE=28;

		//设置FU HEADER,并将这个HEADER填入sendbuf[13]
		fu_hdr =(FU_HEADER*)&sendbuf[13];
		fu_hdr->S=1;
		fu_hdr->E=0;
		fu_hdr->R=0;
		fu_hdr->TYPE=nalu.nal_unit_type;

		nalu_payload=&sendbuf[14];//同理将sendbuf[14]赋给nalu_payload
		memcpy(nalu_payload,nalu.buf+1,UDP_MAX_SIZE);//去掉NALU头
		bytes=UDP_MAX_SIZE+14;//获得sendbuf的长度,为nalu的长度（除去起始前缀和NALU头）加上rtp_header，fu_ind，fu_hdr的固定长度14字节
		//send( socket1, sendbuf, bytes, 0 );//发送RTP包

		//发送中间的FU，S=0，E=0，R=0
		for(packetIndex=2;packetIndex<packetNum;packetIndex++)
		{
			rtp_hdr->seq_no = htons(seqNum++); //序列号，每发送一个RTP包增1
	 
			//设置rtp M 位；
			rtp_hdr->marker=0;
			//设置FU INDICATOR,并将这个HEADER填入sendbuf[12]
			fu_ind =(FU_INDICATOR*)&sendbuf[12]; //将sendbuf[12]的地址赋给fu_ind，之后对fu_ind的写入就将写入sendbuf中；
			fu_ind->F=nalu.forbidden_bit;
			fu_ind->NRI=nalu.nal_reference_idc>>5;
			fu_ind->TYPE=28;

			//设置FU HEADER,并将这个HEADER填入sendbuf[13]
			fu_hdr =(FU_HEADER*)&sendbuf[13];
			fu_hdr->S=0;
			fu_hdr->E=0;
			fu_hdr->R=0;
			fu_hdr->TYPE=nalu.nal_unit_type;

			nalu_payload=&sendbuf[14];//同理将sendbuf[14]的地址赋给nalu_payload
			memcpy(nalu_payload,nalu.buf+(packetIndex-1)*UDP_MAX_SIZE+1,UDP_MAX_SIZE);//去掉起始前缀的nalu剩余内容写入sendbuf[14]开始的字符串。
			bytes=UDP_MAX_SIZE+14;//获得sendbuf的长度,为nalu的长度（除去原NALU头）加上rtp_header，fu_ind，fu_hdr的固定长度14字节
			//send( socket1, sendbuf, bytes, 0 );//发送rtp包				
		}

		//发送最后一个的FU，S=0，E=1，R=0
	
		rtp_hdr->seq_no = htons(seqNum ++);
		//设置rtp M 位；当前传输的是最后一个分片时该位置1		
		rtp_hdr->marker=1;
		//设置FU INDICATOR,并将这个HEADER填入sendbuf[12]
		fu_ind =(FU_INDICATOR*)&sendbuf[12]; //将sendbuf[12]的地址赋给fu_ind，之后对fu_ind的写入就将写入sendbuf中；
		fu_ind->F=nalu.forbidden_bit;
		fu_ind->NRI=nalu.nal_reference_idc>>5;
		fu_ind->TYPE=28;

		//设置FU HEADER,并将这个HEADER填入sendbuf[13]
		fu_hdr =(FU_HEADER*)&sendbuf[13];
		fu_hdr->S=0;
		fu_hdr->E=1;
		fu_hdr->R=0;
		fu_hdr->TYPE=nalu.nal_unit_type;

		nalu_payload=&sendbuf[14];//同理将sendbuf[14]的地址赋给nalu_payload
		memcpy(nalu_payload,nalu.buf+(packetIndex-1)*UDP_MAX_SIZE+1,lastPackSize-1);//将nalu最后剩余的-1(去掉了一个字节的NALU头)字节内容写入sendbuf[14]开始的字符串。
		bytes=lastPackSize-1+14;//获得sendbuf的长度,为剩余nalu的长度l-1加上rtp_header，FU_INDICATOR,FU_HEADER三个包头共14字节
		//send( socket1, sendbuf, bytes, 0 );//发送rtp包		
	}

}

}

void ARTPConnection::sendRTPPacket()
{

}



int ARTPConnection::GetAnnexbNALU (NALU_t *nalu,const sp<ABuffer> &buf)
{
	nalu->len = buf->size();
	nalu->buf = buf->data();
	nalu->forbidden_bit = nalu->buf[0] & 0x80;		//1 bit
	nalu->nal_reference_idc = nalu->buf[0] & 0x60;	//2 bit
	nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;	//5 bit
	return 0;//返回两个开始字符之间间隔的字节数，即包含有前缀的NALU的长度
}

void ARTPConnection::dump(NALU_t *n)
{
	if (!n)return;
	LOGI(LOG_TAG,"nal length:%d nal_unit_type: %x\n", n->len,n->nal_unit_type);
}

