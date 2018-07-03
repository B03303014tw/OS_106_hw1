#include<linux/linkage.h>
#include<linux/kernel.h>
#include<asm/unistd.h>
#include<linux/time.h>
#include<linux/types.h>

asmlinkage void sys_hw1(pid_t pid,struct timespec start,struct timespec end){
printk("[Project1] %d %ld.%09ld %ld.%09ld\n",pid,start.tv_sec,start.tv_nsec,end.tv_sec,end.tv_nsec);
return ;
}
