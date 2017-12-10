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
	while (1)
	{
		char * type;
		printf("$ ");
		fgets(command,sizeof(command),stdin);
		command[strlen(command) - 1] = 0;
		type=strtok(command," ");
		if (strcmp(type, "add") == 0)
		{
			char *task_name;
			int time_quantum = 10;
			char *c;
			char *t;
			task_name = strtok(NULL, " ");
			c=strtok(NULL, " ");
			t=strtok(NULL, " ");
			if(task_name!=NULL){
				printf("task name: %s\n", task_name);
				if(c!=NULL){
					if(strcmp(c,"-t")==0){
						if(t!=NULL){
							if(strcmp(t,"L")==0){
								time_quantum = 20;
							}else if (strcmp(t, "S") == 0){
								time_quantum = 10;
							}else{
								printf("The correct time quantum code should be entered.\n");
							}
							printf("time quantum (ms): %d\n", time_quantum);
						}else{
							printf("Use default time quantum.\n");
							printf("time quantum (ms): %d\n", time_quantum);
						}
					}else{
						printf("Correct parameter should be topped with '-t'.\n");
						printf("time quantum (ms): %d\n", time_quantum);
					}
				}else{
					printf("time quantum (ms): %d\n", time_quantum);
				}
			}else{
				printf("The task name should be entered.\n");
			}
		} else if(strcmp(type, "remove")==0) {
			char *PID;
			PID = strtok(NULL, " ");
			if (PID != NULL)
			{
				pid_t pID;
				pID = atoi(PID);
				printf("Removed task's PID: %d\n", (int)pID);
			}else{
				printf("The PID should be entered.\n");
			}
		} else if(strcmp(type, "start")==0) {
			printf("simulating...\n");
			break;
		} else if (strcmp(type, "ps") == 0) {
			printf("information is...\n");
		} else {

		}
	}

	return 0;
}
