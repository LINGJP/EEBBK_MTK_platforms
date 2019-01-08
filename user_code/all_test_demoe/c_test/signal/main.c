#include <stdio.h> 
#include <stdlib.h> 
#include<sys/time.h> 
#include <sys/signal.h>
void sigroutine(int dunno) { /* 信号处理例程，其中dunno将会得到信号的值 */ 
switch (dunno) { 
case 1: 
printf("Get a signal -- SIGHUP "); 
break; 
case 2: 
printf("Get a signal -- SIGINT "); 
break; 
case 3: 
printf("Get a signal -- SIGQUIT "); 
break; 
} 
return; 
} 

int main() { 
printf("process id is %d ",getpid()); 
signal(SIGHUP, sigroutine); //* 下面设置三个信号的处理方法 
signal(SIGINT, sigroutine); 
signal(SIGQUIT, sigroutine); 
for (;;) ; 
} 
