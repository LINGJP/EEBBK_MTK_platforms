#include<stdio.h>
void main(void)
{
	int temp;
	char temps;

	sscanf("lyq123","%s",&temps);

	sscanf("lyq123","lyq%d",&temp);
	printf("%s\n",&temps);
	printf("%d\n",temp);
}
