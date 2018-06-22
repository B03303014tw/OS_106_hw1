#define _GNU_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sched.h>
#include<time.h>
#include<signal.h>
#include<sys/wait.h>
#include<sys/shm.h>
#include<sys/ipc.h>
#include<sys/syscall.h>

void busy(){
for(volatile unsigned long i=0;i<1000000UL;i++);
return;
}

struct time_header{
pid_t pid;
struct timespec end_time;
struct time_header *pre;
};

struct queue_header{
pid_t pid;
int run_time;
int runed_time;
struct queue_header *next;
};

clockid_t clock_id=CLOCK_REALTIME;
int wait_queue_num=0;
int ended_queue_num=0;
int running_time=0;
struct time_header *ended_queue_temp ,*ended_queue=NULL;
struct queue_header *wait_queue_temp ,*wait_queue=NULL;

void sigchld_handler(int signo){
struct timespec time_temp;
clock_gettime(clock_id,&time_temp);
pid_t pid_temp=wait(NULL);
ended_queue_num++;
if(wait_queue_num==1){
	ended_queue->end_time=time_temp;
	ended_queue->pid=pid_temp;
	ended_queue_temp=(struct time_header *)malloc(sizeof(struct time_header));
	ended_queue_temp->pre=ended_queue;
	ended_queue=ended_queue_temp;
	free(wait_queue);
	wait_queue=wait_queue->next;
	wait_queue_num=0;
	running_time=0;
	return;
	}
ended_queue->end_time=time_temp;
ended_queue->pid=pid_temp;
ended_queue_temp=(struct time_header *)malloc(sizeof(struct time_header));
ended_queue_temp->pre=ended_queue;
ended_queue=ended_queue_temp;
free(wait_queue);
wait_queue=wait_queue->next;
wait_queue_num--;
running_time=wait_queue->runed_time;
kill(wait_queue->pid,SIGCONT);
return;
}

void insert_queue(pid_t input,int time){
wait_queue_num++;
if(wait_queue->run_time>time+running_time){
	if(kill(wait_queue->pid,SIGSTOP)==-1){
		wait_queue_temp=(struct queue_header *)malloc(sizeof(struct queue_header));
		wait_queue_temp->pid=input;
		wait_queue_temp->run_time=time;
		wait_queue_temp->runed_time=0;
		wait_queue_temp->next=wait_queue->next;
		wait_queue->next=wait_queue_temp;
		return;	
		}
	wait_queue_temp=(struct queue_header *)malloc(sizeof(struct queue_header));
	wait_queue_temp->pid=input;
	wait_queue_temp->run_time=time;
	wait_queue_temp->runed_time=0;
	wait_queue_temp->next=wait_queue;
	wait_queue->runed_time=running_time;
	wait_queue=wait_queue_temp;
	kill(wait_queue->pid,SIGCONT);
	return;
	}
struct queue_header *temp=wait_queue;
while(temp->next->run_time<=time+temp->next->runed_time)
	temp=temp->next;
wait_queue_temp=(struct queue_header *)malloc(sizeof(struct queue_header));
wait_queue_temp->pid=input;
wait_queue_temp->run_time=time;
wait_queue_temp->runed_time=0;
wait_queue_temp->next=temp->next;
temp->next=wait_queue_temp;
return;
}

void sort(pid_t input,struct timespec *output){
struct time_header *temp=ended_queue;
while(temp->pid!=input){
	temp=temp->pre;
	}
*output=temp->end_time;
return;
}

int main(){
int child_number;
scanf("%d",&child_number);
char child_name[child_number][33];
int child_start_time[child_number];
char child_run_time[child_number][13];
for(int i=0;i<child_number;i++)
	scanf("%s%d%s",child_name[i],&child_start_time[i],child_run_time[i]);
//INPUT

int cpunum=sysconf(_SC_NPROCESSORS_ONLN);
if(cpunum<2){
	write(2,"NEED MORE THAN ONE CPU TO PERFORM A CLOCK",sizeof("NEED MORE THAN ONE CPU TO PERFORM A CLOCK"));
	_exit(1);
	}
cpu_set_t child_cpu_mask;
CPU_ZERO(&child_cpu_mask);
CPU_SET(cpunum-1,&child_cpu_mask);
struct sched_param mother_param,child_param,temp_param;
mother_param.sched_priority=1;
temp_param.sched_priority=2;
child_param.sched_priority=3;
if(sched_setscheduler(0,SCHED_RR,&mother_param)==-1){
	write(2,"NEED PRIVILEGE TO SET SCHEDULER\n",sizeof("NEED PRIVILEGE TO SET SCHEDULER\n"));
	_exit(2);
	}
pid_t pid[child_number];
int shm_id=shmget(IPC_PRIVATE,child_number*sizeof(struct timespec),IPC_CREAT|0600);
struct timespec *start_time=(struct timespec *)shmat(shm_id,NULL,0);
struct sigaction act;
act.sa_flags=SA_NOCLDSTOP;
act.sa_handler=sigchld_handler;
sigemptyset(&act.sa_mask);
sigaddset(&act.sa_mask,SIGCHLD);
sigaction(SIGCHLD,&act,NULL);
//SETTING

int time=0;
wait_queue=(struct queue_header *)malloc(sizeof(struct queue_header));
ended_queue=(struct time_header *)malloc(sizeof(struct time_header));
wait_queue->run_time=2147483647;
for(int i=0;i<child_number;i++){
	if(wait_queue_num==0){
		wait_queue_num=1;
		while(time<child_start_time[i]){
			busy();
			time++;
			}
		pid[i]=fork();
		if(pid[i]==0){//child
			clock_gettime(clock_id,&start_time[i]);
			sched_setscheduler(0,SCHED_FIFO,&child_param);
			sched_setaffinity(0,sizeof(child_cpu_mask),&child_cpu_mask);
			if(execl("./child","./child",child_run_time[i],(char *)NULL)==-1){
				write(2,"exec error",sizeof("exec error"));
				_exit(1);
				}
			}
		wait_queue_temp=(struct queue_header *)malloc(sizeof(struct queue_header));
		wait_queue_temp->pid=pid[i];
		wait_queue_temp->run_time=atoi(child_run_time[i]);
		wait_queue_temp->runed_time=0;
		wait_queue_temp->next=wait_queue;
		wait_queue=wait_queue_temp;
		continue;
		}
	while(time<child_start_time[i]){
		busy();
		running_time++;
		time++;
		}
	sigprocmask(SIG_BLOCK,&act.sa_mask,NULL);
	pid[i]=fork();
	if(pid[i]==0){//child
		clock_gettime(clock_id,&start_time[i]);
		sigprocmask(SIG_UNBLOCK,&act.sa_mask,NULL);
		sched_setscheduler(0,SCHED_FIFO,&child_param);
		sched_setaffinity(0,sizeof(child_cpu_mask),&child_cpu_mask);
		if(execl("./child","./child",child_run_time[i],(char *)NULL)==-1){
			write(2,"exec error(check ./child)\n",sizeof("exec error(check ./child)\n"));
			_exit(1);
			}
		}
	while(start_time[i].tv_sec==0);
	kill(pid[i],SIGSTOP);
	insert_queue(pid[i],atoi(child_run_time[i]));
	sigprocmask(SIG_UNBLOCK,&act.sa_mask,NULL);
	}
//RUN UP
while(ended_queue_num<child_number)
	sleep(1);
//WAITING
struct timespec ended_sorted_time[child_number];
for(int i=0;i<child_number;i++)
	sort(pid[i],&ended_sorted_time[i]);
//SORT

for(int i=0;i<child_number;i++){
	printf("%s %d\n",child_name[i],pid[i]);
	syscall(333,pid[i],start_time[i],ended_sorted_time[i]);
	}
//OUTPUT
return 0;
}
