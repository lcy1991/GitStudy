#include <queue>
#include "foundation/ABuffer.h"
#include "foundation/Mutex.h"

using namespace std;
struct MyQueue
{
	MyQueue();
	~MyQueue();
	int push(const sp<ABuffer> &buf);
	int pop(sp<ABuffer> &buf);
	int getQSize();
	int getEmptyBufSize();

private:
	queue<sp<ABuffer> > mBufQue;
	uint32_t mEmptyBufSize;
};




struct ARTPSource
{
	ARTPSource(uint32_t bufNum, uint32_t bufSize);
	~ARTPSource();
	int inputQPop(sp<ABuffer> &buf);
	int inputQPush(const sp<ABuffer> &buf);
	int outputQPop(sp<ABuffer> &buf);
	int outputQPush(const sp<ABuffer> &buf);
	
private:
	uint32_t mBufSize;
	uint32_t mBufNum;
	MyQueue mInputQueue;
	MyQueue mOutputQueue;
	MyQueue* pInInputQ;
	MyQueue* pOutOutputQ;	
	Mutex mOutputQLock;
	Mutex mInputQLock;
	
};









