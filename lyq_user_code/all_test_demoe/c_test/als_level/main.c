#include <stdio.h> 
#include <stdlib.h> 
#include<sys/time.h> 
#include <sys/signal.h>
/*********
//这个.c文件的函数，主要是解决在屏幕亮度在阈值区间跳动的问题.
//还有一种方法，可以不这么麻烦，不需要增加als_level，只需要定位到哪个als_level的index的时候，在这个index和las_index的级,再判断下是否小于这个Index的值的 0.9倍，小于才切换等级。
********/
unsigned int als_level[] = {0,0,8,15,30,35,1000,1100};
unsigned int als_value[] = {0,8,10,160};

// >15 8 >250 10
int main(int argc, char** argv) {
	
	int project;
	int i;
	int index;
	static last_index = 0;

	if(argc == 2)
        {
                project = atoi(argv[1]);
                printf("lyq: als_ramdata is %d\n",project);
		if(project>1100)
		{
			printf("error: number shouble smaller 1100\n");
			return 0;
       		}
	 }
	else
		printf("error: need input one number\n");

	for (i=0;i<8;i++)
	{
		if(project <= als_level[i])
		{
			break;
		}
	}
	
		if((last_index >0)project>als_level[last_index*2-2] && project <[last_index*2+1])
		{
			return last_index;
		}
			if(i<2)
			{
				index = 0;
				printf("als_value is %d, and index is %d\n",als_value[index],index);
			}
			else if(i%2==0)
			{
				index = i/2;
				printf(" ou shu als_value is %d, and index is %d, i is %d\n",als_value[index],index,i);
			}
			else
			{
					index = (i-1)/2;
				printf(" qi shu als_value is %d, and index is %d\n",als_value[index],index);

			}
			last_index = index;
			return last_index;
	return 0;	 
} 
