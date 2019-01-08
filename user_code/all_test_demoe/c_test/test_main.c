#include"stdio.h"
#include<stdlib.h>
#include<string.h>
#include<linux/kernel.h>

int main()
{
    int a;
    char pNum[]="lyq110";
    char *p = NULL;

	p=strstr(pNum,"110");
    a=strtoul(p,0,16);//最后如果是0，表示自动识别pNum是几进制
    printf("%02x\n",a);
    return 0;
}
/*
int main(void)
{
	//char a = "int32ac";
	//char * buf = &a;
	//int delay;
	int res = -1;
//	kstrtoint(buf,10,&delay);	//only in kernel
//	printf("lyq: delay is %d\n",delay);

	if(res)
		printf("lyq %d\n",res);
	printf("lyq1\n");
//	int i = 0;
//	i>>=1;

//	printf("lyq: just for test,i is %d\n",i);
	return 0;
}*/
