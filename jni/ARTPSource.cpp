#include "rtsp/ARTPSource.h"
#define LOG_TAG "ARTPSource"
MyQueue::MyQueue()
{
	mEmptyBufSize = 0;
}
MyQueue::~MyQueue()
{
	while(mBufQue.size())
		mBufQue.pop();
}


int MyQueue::push(const sp<ABuffer> &buf)
{
	if(buf->size()==0)
			mEmptyBufSize++;
	else mEmptyBufSize--;
	mBufQue.push(buf);
	LOGI(LOG_TAG,"mEmptyBufSize %d",mEmptyBufSize);
	return 0;
}
int	MyQueue::pop(sp<ABuffer> &buf)
{

	buf = mBufQue.front();
	mBufQue.pop();
	return 0;

}
int MyQueue::getQSize()
{
	return mBufQue.size();
}
int MyQueue::getEmptyBufSize()
{
	return mEmptyBufSize;
}

int ARTPSource::inputQPop(sp<ABuffer> &buf)
{
	mInputQLock.lock();
	if(pInInputQ->getEmptyBufSize()==0)
		{
			LOGI(LOG_TAG,"inputQPop input queue is full,waiting for exchange");
			mInputQLock.unlock();
			pInInputQ = (pInInputQ == &mInputQueue)?&mOutputQueue:&mInputQueue;
			mOutputQLock.lock();//trylock
			LOGI(LOG_TAG,"get mOutputQLock");
			pInInputQ->pop(buf);
			mOutputQLock.unlock();
		}
	else 
		{
			LOGI(LOG_TAG,"pop from input queue");
			pInInputQ->pop(buf);
		}
	
}
int ARTPSource::inputQPush(const sp<ABuffer> &buf)
{
	LOGI(LOG_TAG,"push to input queue");
	pInInputQ->push(buf);
	mInputQLock.unlock();
	return 0;
}
int ARTPSource::outputQPop(sp<ABuffer> &buf)
{
	if(pOutOutputQ->getEmptyBufSize() == pOutOutputQ->getQSize())
		{
			LOGI(LOG_TAG,"output queue is empty ,waiting for exchange");
			mOutputQLock.lock();
			//LOGI(LOG_TAG,"get mOutputQLock");
			pOutOutputQ = (pOutOutputQ == &mOutputQueue)?&mInputQueue:&mOutputQueue;
			mInputQLock.lock();
			pOutOutputQ->pop(buf);
			mInputQLock.unlock();
		}
	else
		{
			LOGI(LOG_TAG,"pop from output queue");
			pOutOutputQ->pop(buf);
		}
	return 0;
}
int ARTPSource::outputQPush(const sp<ABuffer> &buf)
{
	LOGI(LOG_TAG,"push to output queue");
	pOutOutputQ->push(buf);
	if(pOutOutputQ->getEmptyBufSize() == pOutOutputQ->getQSize())
		{
			mOutputQLock.unlock();
		}
	return 0;
}


ARTPSource::ARTPSource(uint32_t bufNum, uint32_t bufSize)
	: mBufSize(bufSize),
	  mBufNum(bufNum)
{
	
	uint32_t i;
	for(i = 0; i < bufNum;i++)
		{
			sp<ABuffer> buf1 = new ABuffer(bufSize);
			sp<ABuffer> buf2 = new ABuffer(bufSize);
			buf1->setRange(0,0);
			buf2->setRange(0,0);
			LOGI(LOG_TAG,"bufsize %d",buf1->size());
			mInputQueue.push(buf1);
			mOutputQueue.push(buf2);
		}
	pInInputQ = &mInputQueue;
	pOutOutputQ = &mOutputQueue;

}

ARTPSource::~ARTPSource()
{

}




