#include <stdio.h>

void main(int argc, char **argv)
{
	struct tp{
		int c;
	};
	struct lyq{
		int a;
		int b;
		struct tp temp;
	}ee,*p;

	p = &ee;

	ee.a=10;
	*(&p->a)=2222;

	printf("%d\n",p->a);
}
