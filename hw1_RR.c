#define _GNU_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sched.h>
#include<signal.h>
#include<time.h>
#include<sys/syscall.h>
#include<sys/wait.h>
#include<sys/shm.h>
#include<sys/ipc.h>

void busy(){
for(volatile unsigned long i=0;i<1000000UL;i++);
return;
}

struct queue_head{
pid_t pid;
struct timespec end_time;
struct queue_head *next;
struct queue_head *prev;
};

clockid_t clock_id=CLOCK_REALTIME;
struct queue_head *working_queue=NULL;
struct queue_head *ended_data_queue=NULL;
int working_child_num=0;
int ended_child_num=0;
int RR=0;

void wait_child(){
struct timespec end;
clock_gettime(clock_id,&end);
pid_t pid_temp=wait(NULL);
ended_data_queue->pid=pid_temp;
ended_data_queue->end_time=end;
ended_child_num++;
//printf("%d waited\n",pid_temp);
return;
}

void delete_all_queue(){
free(working_queue);
working_queue=NULL;
working_child_num=0;
return;
}

void delete_queue(){
working_queue->next->prev=working_queue->prev;
working_queue->prev->next=working_queue->next;
free(working_queue);
working_queue=working_queue->next;
working_child_num--;
return;
}

void sigchld_handler(int signo){
RR=0;
if(working_child_num==1){
	wait_child();
	delete_all_queue();
	struct queue_head *temp=(struct queue_head *)malloc(sizeof(struct queue_head));
	temp->prev=ended_data_queue;
	ended_data_queue=temp;
	return;
	}
wait_child();
delete_queue();
kill(working_queue->pid,SIGCONT);
//printf("%d con\n",working_queue->pid);
struct queue_head *temp=(struct queue_head *)malloc(sizeof(struct queue_head));
temp->prev=ended_data_queue;
ended_data_queue=temp;
return;
}

void RR_routine(){
if(kill(working_queue->pid,SIGSTOP)==-1){
	return;
	}
//printf("%d stop\n",working_queue->pid);
RR=0;
working_queue=working_queue->next;
kill(working_queue->pid,SIGCONT);
//printf("%d continue\n",working_queue->pid);
return;
}

void insert_working_queue(pid_t input){
working_child_num++;
struct queue_head *temp=(struct queue_head *)malloc(sizeof(struct queue_head));
temp->pid=input;
temp->next=working_queue;
temp->prev=working_queue->prev;
temp->prev->next=temp;
working_queue->prev=temp;
return;
}

void sort(pid_t input,struct timespec *ptr){
struct queue_head *temp=ended_data_queue;
while(temp->pid!=input){
	temp=temp->prev;
	}
*ptr=temp->end_time;
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
	write(2,"NEED MORE THAN ONE CPU TO PERFORM A CLOCK\n",sizeof("NEED MORE THAN ONE CPU TO PERFORM A CLOCK\n"));
	_exit(1);
	}
cpu_set_t child_cpu_mask;
CPU_ZERO(&child_cpu_mask);
CPU_SET(cpunum-1,&child_cpu_mask);
pid_t pid[child_number];
struct sched_param mother_param,child_param;
mother_param.sched_priority=1;
child_param.sched_priority=2;
if(sched_setscheduler(0,SCHED_RR,&mother_param)==-1){
	write(2,"NEED ROOT PEVILEGE TO SET SCHEDULER\n",sizeof("NEED ROOT PEVILEGE TO SET SCHEDULER\n"));
	_exit(2);
	}
int shm_id=shmget(IPC_PRIVATE,child_number*sizeof(struct timespec),IPC_CREAT|0600);
struct timespec *start_time=(struct timespec *)shmat(shm_id,NULL,0);
struct sigaction act;
act.sa_handler=sigchld_handler;
act.sa_flags=SA_NOCLDSTOP;
sigemptyset(&act.sa_mask);
sigaddset(&act.sa_mask,SIGCHLD);
sigaction(SIGCHLD,&act,NULL);
//SETTING

int time=0;
ended_data_queue=(struct queue_head *)malloc(sizeof(struct queue_head));
for(int i=0;i<child_number;i++){
	if(working_child_num<1){
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
				write(2,"PLEASE COMPILE CHILD\n",sizeof("PLEASE COMPILE CHILD\n"));
				_exit(1);
				}
			}
		struct queue_head *temp=(struct queue_head *)malloc(sizeof(struct queue_head));
		temp->pid=pid[i];
		temp->prev=temp;
		temp->next=temp;
		working_queue=temp;
		while(start_time[i].tv_sec==0);
		working_child_num=1;
		continue;
		}
	while(time<child_start_time[i]){
		busy();
		time++;
		RR++;
		sigprocmask(SIG_BLOCK,&act.sa_mask,NULL);
		if(RR==500){
			RR_routine();
			}
		sigprocmask(SIG_UNBLOCK,&act.sa_mask,NULL);
		}
	sigprocmask(SIG_BLOCK,&act.sa_mask,NULL);
	pid[i]=fork();
	if(pid[i]==0){//child
		clock_gettime(clock_id,&start_time[i]);
		sched_setscheduler(0,SCHED_FIFO,&child_param);
		sched_setaffinity(0,sizeof(child_cpu_mask),&child_cpu_mask);
		if(execl("./child","./child",child_run_time[i],(char *)NULL)==-1){
			write(2,"PLEASE COMPILE CHILD\n",sizeof("PLEASE COMPILE CHILD\n"));
			_exit(1);
			}
		}
	while(start_time[i].tv_sec==0);
	kill(pid[i],SIGSTOP);
	insert_working_queue(pid[i]);
	sigprocmask(SIG_UNBLOCK,&act.sa_mask,NULL);
	}
//RUNNING UP CHILD

while(1){
	busy();
	RR++;
	sigprocmask(SIG_BLOCK,&act.sa_mask,NULL);
	if(RR==500&&working_child_num>1)
		RR_routine();
	sigprocmask(SIG_UNBLOCK,&act.sa_mask,NULL);
	//printf("%d\n",ended_child_num);
	if(ended_child_num==child_number)
		break;	
	}
//WAITING

struct timespec sorted_ended_time[child_number];
for(int i=0;i<child_number;i++)
	sort(pid[i],&sorted_ended_time[i]);
//PREPARE ENDED TIME
for(int i=0;i<child_number;i++){
	printf("%s %d\n",child_name[i],pid[i]);
	syscall(333,pid[i],start_time[i],sorted_ended_time[i]);
	}
//OUTPUT
return 0;
}

