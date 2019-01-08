#include<stdio.h>

#define  LOG(...)                                \  
        do {                                       \  
            __android_log_print(                   \  
                ANDROID_LOG_INFO,                  \  
                "adbd",                            \  
                __VA_ARGS__ );                     \  
        } while (0)  

void main()
{
	LOG("I LOVE YOU\N");
} 
