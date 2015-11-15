#include "rtsp/ARTPSource.h"
#define LOG_TAG "ARTPSource"
MyQueue::MyQueue()
{
	mEmptyBufSize = 0;
}
MyQueue::~MyQueue()
{
	while(mBufQue.size())
		{
			mBufQue.pop();
			//LOGI(LOG_TAG,"~MyQueue");
		}
		
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
	LOGI(LOG_TAG,"mInputQ is Lock");
	if(pInInputQ->getEmptyBufSize()==0)//input queue is full, we need empty queue
		{
			LOGI(LOG_TAG,"inputQPop input queue is full,waiting for exchange");
			mInputQLock.unlock();
			LOGI(LOG_TAG,"mInputQ is unLock");
			mOutputQLock.lock();//get the output queue
			if(pOutOutputQ->getEmptyBufSize()!=0)//if output queue is not empty, don't change
				{
					mOutputQLock.unlock();
					return -1;
				}
			pOutOutputQ = (pOutOutputQ == &mOutputQueue)?&mInputQueue:&mOutputQueue;
			pInInputQ = (pInInputQ == &mInputQueue)?&mOutputQueue:&mInputQueue;

			pInInputQ->pop(buf);
			mOutputQLock.unlock();
			LOGI(LOG_TAG,"mOutputQ is unLock--------------------------");
		}
	else 
		{
			LOGI(LOG_TAG,"pop from input queue");
			pInInputQ->pop(buf);
		}
	
}
int ARTPSource::inputQPush(const sp<ABuffer> &buf)
{
	LOGI(LOG_TAG,"push to input queue %d",pInInputQ);
	pInInputQ->push(buf);
	mInputQLock.unlock();
	LOGI(LOG_TAG,"mInputQ is unLock");
	return 0;
}
int ARTPSource::outputQPop(sp<ABuffer> &buf)
{

	mOutputQLock.lock();

	if(pOutOutputQ->getEmptyBufSize() == pOutOutputQ->getQSize())//queue is empty
		{
			
			LOGI(LOG_TAG,"output queue is empty ,waiting for exchange");
			mOutputQLock.unlock();
			LOGI(LOG_TAG,"mOutputQ is unlock");
			mInputQLock.lock();
			LOGI(LOG_TAG,"mInputQ is lock");
			if(pInInputQ->getEmptyBufSize()!=0)//intput queue is not empty, we change in and out
				{
					mInputQLock.unlock();
					LOGI(LOG_TAG,"mInputQ is unlock");
					mOutputQLock.unlock();
					LOGI(LOG_TAG,"mOutputQ is unlock");
					LOGI(LOG_TAG,"input Q is also empty");
					return -1;
				}
			pOutOutputQ = (pOutOutputQ == &mOutputQueue)?&mInputQueue:&mOutputQueue;
			pInInputQ = (pInInputQ == &mInputQueue)?&mOutputQueue:&mInputQueue;
			pOutOutputQ->pop(buf);
			if(buf->size()==0)//buf is empty
				{
					pOutOutputQ->push(buf);
					return -1;
				}			
			mInputQLock.unlock();
			LOGI(LOG_TAG,"mInputQ is unlock +++++++++++++++++++++++++++");
		}
	else
		{
			LOGI(LOG_TAG,"pop from output queue %d",pOutOutputQ);
			pOutOutputQ->pop(buf);
			if(buf->size()==0)//ABuffer is empty
				{
					pOutOutputQ->push(buf);
					return -1;
				}
		}
	return 0;
}
int ARTPSource::outputQPush(const sp<ABuffer> &buf)
{
	LOGI(LOG_TAG,"push to output queue");
	pOutOutputQ->push(buf);
	//if(pOutOutputQ->getEmptyBufSize() == pOutOutputQ->getQSize())
	//	{
	mOutputQLock.unlock();
	//		LOGI(LOG_TAG,"mOutputQ is unlock");
	//	}
	return 0;
}


ARTPSource::ARTPSource(uint32_t bufNum, uint32_t bufSize)
	: mBufSize(bufSize),
	  mBufNum(bufNum),
	  mFramerate(20)
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




