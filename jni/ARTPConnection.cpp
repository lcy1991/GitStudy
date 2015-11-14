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
	if(code >= (1<<27)){                    //�ñ���Ϊ9bit����
		code >>= 32 - 9;                    //��buf����23bit�õ����ش�
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
	GetAnnexbNALU(&nalu,buf);//ÿִ��һ�Σ��ļ���ָ��ָ�򱾴��ҵ���NALU��ĩβ����һ��λ�ü�Ϊ�¸�NALU����ʼ��0x000001
	dump(&nalu);//���NALU���Ⱥ�TYPE

	//memset(sendbuf,0,1500);//���sendbuf����ʱ�Ὣ�ϴε�ʱ�����գ������Ҫts_current�������ϴε�ʱ���ֵ

	//rtp�̶���ͷ��Ϊ12�ֽ�,�þ佫sendbuf[0]�ĵ�ַ����rtp_hdr���Ժ��rtp_hdr��д�������ֱ��д��sendbuf��
	rtp_hdr =(RTP_FIXED_HEADER*)&sendbuf[0]; 
	
	//����RTP HEADER��
	rtp_hdr->version = 2;	//�汾�ţ��˰汾�̶�Ϊ2
	rtp_hdr->marker  = 0;	//��־λ���ɾ���Э��涨��ֵ��
	rtp_hdr->payload = 96;//H264;//�������ͺţ�
	rtp_hdr->ssrc	 = htonl(10);//���ָ��Ϊ10�������ڱ�RTP�Ự��ȫ��Ψһ

	//��һ��NALUС��1400�ֽڵ�ʱ�򣬲���һ����RTP������
	if(nalu.len <= UDP_MAX_SIZE){
		//����rtp M λ��
		rtp_hdr->marker=1;
		rtp_hdr->seq_no = htons(seqNum ++); //���кţ�ÿ����һ��RTP����1
		rtp_hdr->timestamp=htonl(timeStamp);
		//����NALU HEADER,�������HEADER����sendbuf[12]
		nalu_hdr =(NALU_HEADER*)&sendbuf[12]; //��sendbuf[12]�ĵ�ַ����nalu_hdr��֮���nalu_hdr��д��ͽ�д��sendbuf�У�
		nalu_hdr->F=nalu.forbidden_bit;
		nalu_hdr->NRI=nalu.nal_reference_idc>>5;//��Ч������n->nal_reference_idc�ĵ�6��7λ����Ҫ����5λ���ܽ���ֵ����nalu_hdr->NRI��
		nalu_hdr->TYPE=nalu.nal_unit_type;

		nalu_payload=&sendbuf[13];//ͬ��sendbuf[13]����nalu_payload
		memcpy(nalu_payload,nalu.buf+1,nalu.len-1);//ȥ��naluͷ��naluʣ������д��sendbuf[13]��ʼ���ַ�����

		//timeStamp = timeStamp + timestamp_increse;
		
		bytes = nalu.len + 12 ; //���sendbuf�ĳ���,Ϊnalu�ĳ��ȣ�����NALUͷ����ȥ��ʼǰ׺������rtp_header�Ĺ̶�����12�ֽ�
		//send(socket1,sendbuf,bytes,0);//����RTP��
		//Sleep(100);
	}else{
		int packetNum = nalu.len/UDP_MAX_SIZE;
		if (n->len%UDP_MAX_SIZE != 0)
			packetNum ++;

		int lastPackSize = nalu.len - (packetNum-1)*UDP_MAX_SIZE;
		int packetIndex = 1 ;

		//timeStamp = timeStamp + timestamp_increse;

		rtp_hdr->timestamp = htonl(timeStamp);

		//���͵�һ����FU��S=1��E=0��R=0

		rtp_hdr->seq_no = htons(seqNum++); //���кţ�ÿ����һ��RTP����1
		//����rtp M λ��
		rtp_hdr->marker=0;
		//����FU INDICATOR,�������HEADER����sendbuf[12]
		fu_ind =(FU_INDICATOR*)&sendbuf[12]; //��sendbuf[12]�ĵ�ַ����fu_ind��֮���fu_ind��д��ͽ�д��sendbuf�У�
		fu_ind->F=nalu.forbidden_bit;
		fu_ind->NRI=nalu.nal_reference_idc>>5;
		fu_ind->TYPE=28;

		//����FU HEADER,�������HEADER����sendbuf[13]
		fu_hdr =(FU_HEADER*)&sendbuf[13];
		fu_hdr->S=1;
		fu_hdr->E=0;
		fu_hdr->R=0;
		fu_hdr->TYPE=nalu.nal_unit_type;

		nalu_payload=&sendbuf[14];//ͬ��sendbuf[14]����nalu_payload
		memcpy(nalu_payload,nalu.buf+1,UDP_MAX_SIZE);//ȥ��NALUͷ
		bytes=UDP_MAX_SIZE+14;//���sendbuf�ĳ���,Ϊnalu�ĳ��ȣ���ȥ��ʼǰ׺��NALUͷ������rtp_header��fu_ind��fu_hdr�Ĺ̶�����14�ֽ�
		//send( socket1, sendbuf, bytes, 0 );//����RTP��

		//�����м��FU��S=0��E=0��R=0
		for(packetIndex=2;packetIndex<packetNum;packetIndex++)
		{
			rtp_hdr->seq_no = htons(seqNum++); //���кţ�ÿ����һ��RTP����1
	 
			//����rtp M λ��
			rtp_hdr->marker=0;
			//����FU INDICATOR,�������HEADER����sendbuf[12]
			fu_ind =(FU_INDICATOR*)&sendbuf[12]; //��sendbuf[12]�ĵ�ַ����fu_ind��֮���fu_ind��д��ͽ�д��sendbuf�У�
			fu_ind->F=nalu.forbidden_bit;
			fu_ind->NRI=nalu.nal_reference_idc>>5;
			fu_ind->TYPE=28;

			//����FU HEADER,�������HEADER����sendbuf[13]
			fu_hdr =(FU_HEADER*)&sendbuf[13];
			fu_hdr->S=0;
			fu_hdr->E=0;
			fu_hdr->R=0;
			fu_hdr->TYPE=nalu.nal_unit_type;

			nalu_payload=&sendbuf[14];//ͬ��sendbuf[14]�ĵ�ַ����nalu_payload
			memcpy(nalu_payload,nalu.buf+(packetIndex-1)*UDP_MAX_SIZE+1,UDP_MAX_SIZE);//ȥ����ʼǰ׺��naluʣ������д��sendbuf[14]��ʼ���ַ�����
			bytes=UDP_MAX_SIZE+14;//���sendbuf�ĳ���,Ϊnalu�ĳ��ȣ���ȥԭNALUͷ������rtp_header��fu_ind��fu_hdr�Ĺ̶�����14�ֽ�
			//send( socket1, sendbuf, bytes, 0 );//����rtp��				
		}

		//�������һ����FU��S=0��E=1��R=0
	
		rtp_hdr->seq_no = htons(seqNum ++);
		//����rtp M λ����ǰ����������һ����Ƭʱ��λ��1		
		rtp_hdr->marker=1;
		//����FU INDICATOR,�������HEADER����sendbuf[12]
		fu_ind =(FU_INDICATOR*)&sendbuf[12]; //��sendbuf[12]�ĵ�ַ����fu_ind��֮���fu_ind��д��ͽ�д��sendbuf�У�
		fu_ind->F=nalu.forbidden_bit;
		fu_ind->NRI=nalu.nal_reference_idc>>5;
		fu_ind->TYPE=28;

		//����FU HEADER,�������HEADER����sendbuf[13]
		fu_hdr =(FU_HEADER*)&sendbuf[13];
		fu_hdr->S=0;
		fu_hdr->E=1;
		fu_hdr->R=0;
		fu_hdr->TYPE=nalu.nal_unit_type;

		nalu_payload=&sendbuf[14];//ͬ��sendbuf[14]�ĵ�ַ����nalu_payload
		memcpy(nalu_payload,nalu.buf+(packetIndex-1)*UDP_MAX_SIZE+1,lastPackSize-1);//��nalu���ʣ���-1(ȥ����һ���ֽڵ�NALUͷ)�ֽ�����д��sendbuf[14]��ʼ���ַ�����
		bytes=lastPackSize-1+14;//���sendbuf�ĳ���,Ϊʣ��nalu�ĳ���l-1����rtp_header��FU_INDICATOR,FU_HEADER������ͷ��14�ֽ�
		//send( socket1, sendbuf, bytes, 0 );//����rtp��		
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
	return 0;//����������ʼ�ַ�֮�������ֽ�������������ǰ׺��NALU�ĳ���
}

void ARTPConnection::dump(NALU_t *n)
{
	if (!n)return;
	LOGI(LOG_TAG,"nal length:%d nal_unit_type: %x\n", n->len,n->nal_unit_type);
}

