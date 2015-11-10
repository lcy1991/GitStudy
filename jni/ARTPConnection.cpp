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
#include <utils/Log.h>

#include "ARTPConnection.h"

#include "ARTPSource.h"
#include "ASessionDescription.h"

#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/AString.h>
#include <media/stagefright/foundation/hexdump.h>

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

struct ARTPConnection::StreamInfo {
    int mRTPSocket;
    int mRTCPSocket;
//    sp<ASessionDescription> mSessionDesc;
    size_t mIndex;
    sp<AMessage> mNotifyMsg;
//    KeyedVector<uint32_t, sp<ARTPSource> > mSources;

    int64_t mNumRTCPPacketsReceived;
    int64_t mNumRTPPacketsReceived;
    struct sockaddr_in mRemoteRTCPAddr;

//    bool mIsInjected;
};

ARTPConnection::ARTPConnection(uint32_t flags)
    : mFlags(flags),
      mPollEventPending(false),
      mLastReceiverReportTimeUs(-1) {
}

ARTPConnection::~ARTPConnection() {
}

void ARTPConnection::addStream(
        int rtpSocket, int rtcpSocket,
        const sp<ASessionDescription> &sessionDesc,
        size_t index,
        const sp<AMessage> &notify,
        bool injected) {
    sp<AMessage> msg = new AMessage(kWhatAddStream, id());
    msg->setInt32("rtp-socket", rtpSocket);
    msg->setInt32("rtcp-socket", rtcpSocket);
    msg->setObject("session-desc", sessionDesc);
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
    *rtpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    CHECK_GE(*rtpSocket, 0);

    bumpSocketBufferSize(*rtpSocket);

    *rtcpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    CHECK_GE(*rtcpSocket, 0);

    bumpSocketBufferSize(*rtcpSocket);

    unsigned start = (rand() * 1000)/ RAND_MAX + 15550;
    start &= ~1;

    for (unsigned port = start; port < 65536; port += 2) {
//        struct sockaddr_in addr;
//        memset(addr.sin_zero, 0, sizeof(addr.sin_zero));
//        addr.sin_family = AF_INET;
//        addr.sin_addr.s_addr = INADDR_ANY;
		addr.sin_port = htons(port);

        if (bind(*rtpSocket,
                 (const struct sockaddr *)&addr, sizeof(addr)) < 0) {
            continue;
        }

        addr.sin_port = htons(port + 1);

        if (bind(*rtcpSocket,
                 (const struct sockaddr *)&addr, sizeof(addr)) == 0) {
            *rtpPort = port;
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
	return port;

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

        case kWhatFakeTimestamps:
        {
            onFakeTimestamps();
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
//    mStreams.push_back(StreamInfo());
//    StreamInfo *info = &*--mStreams.end();

    int32_t s;
    CHECK(msg->findInt32("rtp-socket", &s));
    mStreams.mRTPSocket = s;
    CHECK(msg->findInt32("rtcp-socket", &s));
    mStreams.mRTCPSocket = s;

//    int32_t injected;
//    CHECK(msg->findInt32("injected", &injected));

//    info->mIsInjected = injected;

//    sp<RefBase> obj;
//    CHECK(msg->findObject("session-desc", &obj));
//    info->mSessionDesc = static_cast<ASessionDescription *>(obj.get());

    CHECK(msg->findSize("index", &mStreams.mIndex));
    CHECK(msg->findMessage("notify", &mStreams.mNotifyMsg));

    mStreams.mNumRTCPPacketsReceived = 0;
    mStreams.mNumRTPPacketsReceived = 0;
    memset(&mStreams.mRemoteRTCPAddr, 0, sizeof(mStreams.mRemoteRTCPAddr));


}

void ARTPConnection::onRemoveStream(const sp<AMessage> &msg) {
    int32_t rtpSocket, rtcpSocket;
    CHECK(msg->findInt32("rtp-socket", &rtpSocket));
    CHECK(msg->findInt32("rtcp-socket", &rtcpSocket));
/*
    List<StreamInfo>::iterator it = mStreams.begin();
    while (it != mStreams.end()
           && (it->mRTPSocket != rtpSocket || it->mRTCPSocket != rtcpSocket)) {
        ++it;
    }

    if (it == mStreams.end()) {
        TRESPASS();
    }

    mStreams.erase(it);*/
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
int ARTPConnection::parseRTP(const sp<ABuffer> &buf) {
	uint8_t header[12];
	ssize_t nbytes;
	uint8_t* data = buf->data();
	if(buf->offset()!=12)return -1;
	if(isFirstNalu(data,buf->size()))
		{
			mRTPHeader.timestamp  += 300;
		}
	mRTPHeader.seq += 1;
	if(buf->size() < 1460)
		{
			makeRTPHeader(&mRTPHeader,header);
			memcpy(buf->base(),header,12);
			nbytes = send(
            mStreams.mRTPSocket,
            buf->base(),
            buf->capacity(),
            0,);
		}

	
	

	
}

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


status_t ARTPConnection::parseRTCP(StreamInfo *s, const sp<ABuffer> &buffer) {
    if (s->mNumRTCPPacketsReceived++ == 0) {
        sp<AMessage> notify = s->mNotifyMsg->dup();
        notify->setInt32("first-rtcp", true);
        notify->post();
    }

    const uint8_t *data = buffer->data();
    size_t size = buffer->size();

    while (size > 0) {
        if (size < 8) {
            // Too short to be a valid RTCP header
            return -1;
        }

        if ((data[0] >> 6) != 2) {
            // Unsupported version.
            return -1;
        }

        if (data[0] & 0x20) {
            // Padding present.

            size_t paddingLength = data[size - 1];

            if (paddingLength + 12 > size) {
                // If we removed this much padding we'd end up with something
                // that's too short to be a valid RTP header.
                return -1;
            }

            size -= paddingLength;
        }

        size_t headerLength = 4 * (data[2] << 8 | data[3]) + 4;

        if (size < headerLength) {
            // Only received a partial packet?
            return -1;
        }

        switch (data[1]) {
            case 200:
            {
                parseSR(s, data, headerLength);
                break;
            }

            case 201:  // RR
            case 202:  // SDES
            case 204:  // APP
                break;

            case 205:  // TSFB (transport layer specific feedback)
            case 206:  // PSFB (payload specific feedback)
                // hexdump(data, headerLength);
                break;

            case 203:
            {
                parseBYE(s, data, headerLength);
                break;
            }

            default:
            {
                LOGW("Unknown RTCP packet type %u of size %d",
                     (unsigned)data[1], headerLength);
                break;
            }
        }

        data += headerLength;
        size -= headerLength;
    }

    return OK;
}

status_t ARTPConnection::parseBYE(
        StreamInfo *s, const uint8_t *data, size_t size) {
    size_t SC = data[0] & 0x3f;

    if (SC == 0 || size < (4 + SC * 4)) {
        // Packet too short for the minimal BYE header.
        return -1;
    }

    uint32_t id = u32at(&data[4]);

    sp<ARTPSource> source = findSource(s, id);

    source->byeReceived();

    return OK;
}

status_t ARTPConnection::parseSR(
        StreamInfo *s, const uint8_t *data, size_t size) {
    size_t RC = data[0] & 0x1f;

    if (size < (7 + RC * 6) * 4) {
        // Packet too short for the minimal SR header.
        return -1;
    }

    uint32_t id = u32at(&data[4]);
    uint64_t ntpTime = u64at(&data[8]);
    uint32_t rtpTime = u32at(&data[16]);

#if 0
    LOGI("XXX timeUpdate: ssrc=0x%08x, rtpTime %u == ntpTime %.3f",
         id,
         rtpTime,
         (ntpTime >> 32) + (double)(ntpTime & 0xffffffff) / (1ll << 32));
#endif

    sp<ARTPSource> source = findSource(s, id);

    if ((mFlags & kFakeTimestamps) == 0) {
        source->timeUpdate(rtpTime, ntpTime);
    }

    return 0;
}

sp<ARTPSource> ARTPConnection::findSource(StreamInfo *info, uint32_t srcId) {
    sp<ARTPSource> source;
    ssize_t index = info->mSources.indexOfKey(srcId);
    if (index < 0) {
        index = info->mSources.size();

        source = new ARTPSource(
                srcId, info->mSessionDesc, info->mIndex, info->mNotifyMsg);

        info->mSources.add(srcId, source);
    } else {
        source = info->mSources.valueAt(index);
    }

    return source;
}

void ARTPConnection::injectPacket(int index, const sp<ABuffer> &buffer) {
    sp<AMessage> msg = new AMessage(kWhatInjectPacket, id());
    msg->setInt32("index", index);
    msg->setObject("buffer", buffer);
    msg->post();
}

void ARTPConnection::onInjectPacket(const sp<AMessage> &msg) {
    int32_t index;
    CHECK(msg->findInt32("index", &index));

    sp<RefBase> obj;
    CHECK(msg->findObject("buffer", &obj));

    sp<ABuffer> buffer = static_cast<ABuffer *>(obj.get());

    List<StreamInfo>::iterator it = mStreams.begin();
    while (it != mStreams.end()
           && it->mRTPSocket != index && it->mRTCPSocket != index) {
        ++it;
    }

    if (it == mStreams.end()) {
        TRESPASS();
    }

    StreamInfo *s = &*it;

    status_t err;
    if (it->mRTPSocket == index) {
        err = parseRTP(s, buffer);
    } else {
        err = parseRTCP(s, buffer);
    }
}

void ARTPConnection::fakeTimestamps() {
    (new AMessage(kWhatFakeTimestamps, id()))->post();
}

void ARTPConnection::onFakeTimestamps() {
    List<StreamInfo>::iterator it = mStreams.begin();
    while (it != mStreams.end()) {
        StreamInfo &info = *it++;

        for (size_t j = 0; j < info.mSources.size(); ++j) {
            sp<ARTPSource> source = info.mSources.valueAt(j);

            if (!source->timeEstablished()) {
                source->timeUpdate(0, 0);
                source->timeUpdate(0 + 90000, 0x100000000ll);

                mFlags |= kFakeTimestamps;
            }
        }
    }
}


