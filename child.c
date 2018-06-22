#include<stdlib.h>

int main(int arc ,char **arv){
int j=atoi(arv[1]);
while(j){
	for(volatile unsigned long i=0;i<1000000UL;i++);
	j--;
	}
return 0;
}
