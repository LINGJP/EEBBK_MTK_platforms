#include <linux/init.h>  
#include <linux/sched.h>  
#include <linux/module.h>  
  
MODULE_LICENSE("GPL");  
MODULE_AUTHOR("feixiaoxing");  
MODULE_DESCRIPTION("This is just a hello module!\n");  
  
static int __init hello_init(void)  
{  
        printk(KERN_EMERG "hello, init\n");  
        return 0;  
}  
  
static void __exit hello_exit(void)  
{  
        printk(KERN_EMERG "hello, exit\n");  
}  
  
module_init(hello_init);  
module_exit(hello_exit);  
