#include<unistd.h>
#include<stdio.h>
#include<sys/wait.h>

int main(){
char method[5];
scanf("%s",method);
pid_t pid=fork();
if(pid<0){
	dprintf(2,"fork error\n");
	_exit(1);
	}
else if(pid==0){
	if(method[0]=='F'){
		if(execl("./hw1_FIFO","./hw1_FIFO",(char *)NULL)==-1){
			dprintf(2,"execl error ! check file!\n");
			_exit(2);
			}
		}
	else if(method[0]=='R'){
		if(execl("./hw1_RR","./hw1_RR",(char *)NULL)==-1){
			dprintf(2,"execl error ! check file!\n");
			_exit(2);
			}	
		}
	else if(method[0]=='P'){
		if(execl("./hw1_PSJF","./hw1_PSJF",(char *)NULL)==-1){
			dprintf(2,"execl error ! check file!\n");
			_exit(2);
			}
		}
	else if(method[0]=='S'){
		if(execl("./hw1_SJF","./hw1_SJF",(char *)NULL)==-1){
			dprintf(2,"execl error ! check file!\n");
			_exit(2);
			}
		}
	}
wait(NULL);
_exit(0);
}
