#include <sys/ioctl.h> 
#include<stdio.h> //return 
#include<stdlib.h> //atoi 
#include<string.h>  
#include<sys/types.h>  
#include<sys/stat.h>  
#include<fcntl.h>  
#include<unistd.h>  
#include<time.h>
#include<errno.h>  

#define DEVICE_NAME "/dev/als_ps"  
#define DEVICE_NAME_PS "/sys/bus/platform/drivers/als_ps/ps_rawdata"  
#define DEVICE_NAME_ALS "/sys/bus/platform/drivers/als_ps/als_rawdata"  
#define DEVICE_NAME_CHANCHE "/sys/class/misc/m_alsps_misc/changealsps"
#define DEVICE_NAME_TEST "/sys/class/misc/m_alsps_misc/test"

#define NVRAM_IOCTL_SET_HOR_LIGHT  _IOW('C', 0x01, unsigned int[3])//数据[0] is flag, [1] is dark数据，[2]是 normal;flag 为1是已校准                                      
#define NVRAM_IOCTL_SET_VER_LIGHT  _IOW('C', 0x02, unsigned int[3])//同上                                       
#define NVRAM_IOCTL_SET_HOR_PS  _IOW('C', 0x03, unsigned int[2])       //数据0 is flag , 1 是25cm 值； flag为1是已校准                                   
#define NVRAM_IOCTL_SET_VER_PS  _IOW('C', 0x04, unsigned int[2])	//同上
#define NVRAM_IOCTL_GET_CAIL  _IOR('C', 0x05, unsigned int[2])

#define ALSPS_GET_ALS_RAW_DATA                          _IOR(ALSPS, 0x08, int)
#define ALSPS_GET_PS_RAW_DATA                           _IOR(ALSPS, 0x04, int)

int main(int argc, char** argv)  
{  
	int fd = -1;  
	int val = 0;
	char buf [100];	
	int project =0;
	int times = 0;
 	unsigned int temp[2]={1,110};
	unsigned int temp_ls[3]={13,14,15};
	unsigned int pingjun=0;
	time_t  now;
	time_t  last;
	int count;
	
	char test[]="lyq for test string";
	
//	strcpy(buf,test);
//	printf("%s\n",&buf[0]);
	if(argc == 2)
	{	
		project = atoi(argv[1]);
		printf("lyq: projec %d\n",project);
	}
	else if(argc==3)
	{
		 project = atoi(argv[1]);
		times = atoi(argv[2]);
		printf("lyq: project%d, times is %d\n",project, times);
	}

if(project ==0&&times==1)
{
    fd = open(DEVICE_NAME, O_RDWR);  
    if(fd == -1) {  
        printf("Failed to open device %s.\n", DEVICE_NAME);  
        return -1;  
    }  
      
/*    printf("Read original value:\n");  
    read(fd, &val, sizeof(val));  
    printf("%d.\n\n", val);  
    val = 5;  
    printf("Write value %d to %s.\n\n", val, DEVICE_NAME);  
        write(fd, &val, sizeof(val));  
      
    printf("Read the value again:\n");  
        read(fd, &val, sizeof(val));  
        printf("%d.\n\n", val);  
 */  
	printf("lyq: NVRAM_IOCTL_SET_HOR_LIGHT set data is %d %d %d\n",temp_ls[0],temp_ls[1],temp_ls[2]); 
	ioctl(fd,NVRAM_IOCTL_SET_HOR_LIGHT,&temp_ls);
	
	ioctl(fd,NVRAM_IOCTL_SET_VER_LIGHT,&temp_ls);

/*************************************************************ps sensor**********************/
	printf("lyq: NVRAM_IOCTL_SET_HOR_PS set data is %d %d \n",temp[0],temp[1]);
	ioctl(fd,NVRAM_IOCTL_SET_HOR_PS,&temp);//110

	temp[1]=60;
	printf("lyq:NVRAM_IOCTL_SET_VER_PS set data is %d %d\n",temp[0],temp[1]);	
	ioctl(fd,NVRAM_IOCTL_SET_VER_PS,&temp);
/************************************get data**********************/	
	ioctl(fd,NVRAM_IOCTL_GET_CAIL,&temp);//读取	
	printf("lyq:NVRAM_IOCTL_GET_CAIL get data  is %d %d\n",temp[0],temp[1]);
	close(fd); 
}
else if(project == 1)//隔2ms一次，times 是次数
{
	fd = open(DEVICE_NAME_PS, O_RDONLY);
	    if(fd == -1) {
       		 printf("Failed to open device %s.\n", DEVICE_NAME_PS);
        	return -1;
    		}
	count = times;
	time(&last);
	while(times!=0)
	{
		time(&now);
		if(now - last ==1)
		{
		fd = open(DEVICE_NAME_PS, O_RDONLY);
            if(fd == -1) {
                 printf("Failed to open device %s.\n", DEVICE_NAME_PS);
                return -1;
                }
		read(fd, &buf,18);
		val = atoi(&buf[12]);
		close(fd);	
		pingjun += val;
		printf("lyq: ps_rawdata is %d\n",val);
		times--;
		memcpy(&last,&now,sizeof(now));
		}
	}
		printf("lyq:pingjun is %d\n",(pingjun/count));
		close(fd);
}
else if(project ==2)
{
	fd = open(DEVICE_NAME_ALS, O_RDONLY);
            if(fd == -1) {
                 printf("Failed to open device %s.\n", DEVICE_NAME_ALS);
                return -1;
                }
        count = times;
        time(&last);
        while(times!=0)
        {
                time(&now);
                if(now - last ==1)
                {
		fd = open(DEVICE_NAME_ALS, O_RDONLY);
            	if(fd == -1) {
                 printf("Failed to open device %s.\n", DEVICE_NAME_ALS);
                return -1;
                }

                read(fd, &buf,20);
                val = atoi(&buf[13]);
		close(fd);
                pingjun += val;
                printf("lyq: als_rawdata is %d\n",val);
                times--;
                memcpy(&last,&now,sizeof(now));
                }
        }
                printf("lyq:als pingjun is %d\n",(pingjun/count));
}
else if(project ==3)
{

	temp_ls[0]= 100;
	temp_ls[1] = 32;
	temp_ls[2] = 43;	
	memset(buf,0,sizeof(buf));
	if((times>1)|| (times<0))
	{
		printf("lyq:error the change times is only 0 or 1");
		return -1;
	}
	fd = open(DEVICE_NAME_CHANCHE, O_RDWR);//O_RDONLY);
            if(fd == -1) {
                 printf("Failed to open device %s.\n", DEVICE_NAME_CHANCHE);
                return -1;
                }
	if(times ==0)
		buf[0] = '0';
	if(times ==1)
		buf[0] ='1';
	write(fd,buf,1);
	close(fd);

	fd = open(DEVICE_NAME_TEST, O_RDWR);//O_RDONLY);
            if(fd == -1) {
                 printf("Failed to open device %s.\n",DEVICE_NAME_TEST);
                return -1;
                }
	write(fd,temp_ls,sizeof(temp_ls));	
	close(fd);
	
	fd = open(DEVICE_NAME_TEST, O_RDWR);//O_RDONLY);
            if(fd == -1) {
                 printf("Failed to open device %s.\n",DEVICE_NAME_TEST);
                return -1;
                }
	read(fd,&temp_ls[0],sizeof(temp_ls));
	printf("lyq: read test data is %d %d %d\n",temp_ls[0],temp_ls[1],temp_ls[2]);
	close(fd);
}

else
	printf("error useage ./lyq_test_main project times, 0 is ioctl 1 is ps rawdata, 2 is als rawdata, 3 is change ic\n");

return 0;  
}
