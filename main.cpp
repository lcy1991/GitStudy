#include "LightRefBase.h"
#include "StrongPointer.h"
#include <stdio.h>
class AAA :public LightRefBase<AAA>
{
public:
	AAA(){};
	~AAA()
	{
		printf("A is deleted\n");
	};
};

int main()
{
	sp<AAA> AAAp;
	{
		sp<AAA> AObj = new AAA();
		AAAp = AObj;
	}
	printf("exit\n");
	return 0;
}


