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
{{
sp<AAA> AObj = new AAA();}
	printf("exit\n");
	return 0;
}


