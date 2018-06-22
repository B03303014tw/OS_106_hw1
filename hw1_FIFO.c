#define _GNU_SOURCE
#include<unistd.h>
#include<stdio.h>
#include<sched.h>
#include<time.h>
#include<sys/wait.h>
#include<sys/shm.h>
#include<sys/ipc.h>
#include<signal.h>
#include<sys/syscall.h>

void busy(){
for(volatile unsigned long i=0;i<1000000UL;i++);
}

clockid_t clock_id=CLOCK_REALTIME;
struct timespec exit_time[32768];
int exit_time_index=0;

void sigchld_handler(int signo){
clock_gettime(clock_id,&exit_time[exit_time_index]);
wait(NULL);
exit_time_index++;
}


int main(){
int child_number;
scanf("%d",&child_number);
char child_name[child_number][33];
int child_start_time[child_number];
char child_run_time[child_number][11];
for(int i=0;i<child_number;i++)
	scanf("%s%d%s",child_name[i],&child_start_time[i],child_run_time[i]);
//INPUT

pid_t pid[child_number];
int cpunum=sysconf(_SC_NPROCESSORS_ONLN);
if(cpunum<2){
	dprintf(2,"NEED MORE THAN ONE CPU FOR TIMING\n");
	_exit(1);
	}
cpu_set_t child_cpu_mask;
CPU_ZERO(&child_cpu_mask);
CPU_SET(cpunum-1,&child_cpu_mask);
struct sched_param mother_param,child_param;
mother_param.sched_priority=1;
child_param.sched_priority=2;
if(sched_setscheduler(0,SCHED_RR,&mother_param)==-1){
	dprintf(2,"NEED PRIVILEGE\n");
	_exit(2);
	}
struct sigaction act;
act.sa_handler=sigchld_handler;
act.sa_flags=SA_NOCLDSTOP;
sigemptyset(&act.sa_mask);
sigaddset(&act.sa_mask,SIGCHLD);
sigaction(SIGCHLD,&act,NULL);
int shm_id=shmget(IPC_PRIVATE,child_number*(sizeof(struct timespec)),IPC_CREAT|0600);
struct timespec *pointer_start_time;
pointer_start_time=(struct timespec*)shmat(shm_id,NULL,0);
//SETTING

int time=0;
for(int i=0;i<child_number;i++){
	while(time<child_start_time[i]){
		busy();
		time++;
		}
	pid[i]=fork();
	if(pid[i]==0){
		clock_gettime(clock_id,&pointer_start_time[i]);
		sched_setscheduler(0,SCHED_FIFO,&child_param);
		sched_setaffinity(0,sizeof(child_cpu_mask),&child_cpu_mask);
		if(execl("./child","./child",child_run_time[i],(char *)NULL)==-1){
			dprintf(2,"exec error\n");
			_exit(4);
			}
		}
	else if(pid[i]<0){
		dprintf(2,"fork error\n");
		_exit(3);
		}
	}
//RUNNING

sigprocmask(SIG_BLOCK,&act.sa_mask,NULL);
while(exit_time_index<child_number){
	wait(NULL);
	clock_gettime(clock_id,&exit_time[exit_time_index]);
	exit_time_index++;
	}
//WAITING

for(int i=0;i<child_number;i++){
	printf("%s %d\n",child_name[i],pid[i]);
	syscall(333,pid[i],pointer_start_time[i],exit_time[i]);	
	/*
	Because FIFO we don't need to sort
	*/
	}
//OUTPUT
return 0;
}
