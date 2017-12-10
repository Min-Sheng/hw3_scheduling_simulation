#include "scheduling_simulator.h"

void hw_suspend(int msec_10)
{
	return;
}

void hw_wakeup_pid(int pid)
{
	return;
}

int hw_wakeup_taskname(char *task_name)
{
    return 0;
}

int hw_task_create(char *task_name)
{
    return 0; // the pid of created task name
}

int main()
{
	char command[1024];
	int a = 1;
	while(1){
		char * type;
		if(a==1){
			printf("$ ");
		}
		a = scanf("%s", command);
		if (a == 1)
		{
			type=strtok(command," ");
			if(strcmp(type, "add")==0){
				printf("added\n");
			}else if(strcmp(type, "remove")==0){
				printf("removed\n");
			}else if(strcmp(type, "start")==0){
				printf("simulating...\n");
				break;
			}
			else if (strcmp(type, "ps") == 0)
			{
				printf("information is...\n");
			}else{

			}
		}else{
			fflush(stdin);
		}
	}

	return 0;
}
