#include "scheduling_simulator.h"

struct Data {
	int pid;
    char task_name[10];
	enum TASK_STATE task_state;
	int time_quantum;
	int queueing_time;
};
struct Node
{
	struct Data data;
	struct Node *next;
};

struct Node* head = NULL;

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
	while (current != NULL && current->data.pid != pid)
    {
        prev = current;
        current = current->next;
    }
	return 0; // the pid of created task name
}

void add_task(char *task_name, int time_quantum);
void remove_task(int pid);
void start_simulation();
void process_status();
void free_all();

int main()
{
	char command[1024];
	while (1) {
		char * type;
		printf("$ ");
		fgets(command,sizeof(command),stdin);
		if(strcmp(command,"\n")==0)
			continue;
		command[strlen(command) - 1] = 0;
		type=strtok(command," ");
		if (strcmp(type, "add") == 0) {
			char *task_name;
			int time_quantum = 10;
			char *c;
			char *t;
			task_name = strtok(NULL, " ");
			c=strtok(NULL, " ");
			t=strtok(NULL, " ");
			if(task_name!=NULL) {
				//printf("task name: %s\n", task_name);
				if(c!=NULL) {
					if(strcmp(c,"-t")==0) {
						if(t!=NULL) {
							if(strcmp(t,"L")==0) {
								time_quantum = 20;
							} else if (strcmp(t, "S") == 0) {
								time_quantum = 10;
							} else {
								printf("The correct time quantum code should be entered.\n");
								continue;
							}
						} else {
							printf("Use default time quantum.\n");
						}
					} else {
						printf("Correct parameter should be topped with '-t'.\n");
						continue;
					}
				} else {
					//printf("time quantum (ms): %d\n", time_quantum);
				}
				add_task(task_name, time_quantum);
			}
			else
			{
				printf("The task name should be entered.\n");
			}
		} else if(strcmp(type, "remove")==0) {
			char *PID;
			PID = strtok(NULL, " ");
			if (PID != NULL) {
				int pid;
				pid = atoi(PID);
				//printf("Removed task's PID: %d\n", pid);
				remove_task(pid);
			} else {
				printf("The PID should be entered.\n");
			}
		} else if(strcmp(type, "start")==0) {
			printf("simulating...\n");
			break;
		} else if (strcmp(type, "ps") == 0) {
			printf("information is...\n");
			process_status();
		} else {
			free_all();
		}
	}
	return 0;
}
void add_task(char *task_name, int time_quantum){
	//printf("Added task:\n");
	//printf("task name: %s\n", task_name);
	//printf("time quantum (ms): %d\n", time_quantum);
	struct Node *last = head;
	struct Node *newNode;
	newNode = malloc(sizeof(struct Node));
	int pid = hw_task_create(task_name);
	strcpy(newNode->data.task_name, task_name);
	newNode->data.pid=pid;
	newNode->data.task_state=TASK_READY;
	newNode->data.time_quantum=time_quantum;
	newNode->next = NULL;
    if (head == NULL)
    {
		head = newNode;
		return;
	}
	while (last->next != NULL)
	{
		last = last->next;
	}
    last->next = newNode;
    return;
}
void remove_task(int pid){
	struct Node *current = head;
	struct Node *prev;
	// If head node itself holds the pid to be deleted
    if (current != NULL && current->data.pid == pid)
    {
        head = current->next;
        free(current);
        return;
    }

    // Search for the pid to be deleted, keep track of the
    // previous node as we need to change 'prev->next'
    while (current != NULL && current->data.pid != pid)
    {
        prev = current;
        current = current->next;
    }

    // If pid was not present in linked list
    if (current == NULL){
		printf("No such pid in the queue.\n");
		return;
	}

    // Unlink the node from linked list
    prev->next = current->next;

    free(current);
	return;
}
void process_status(){
	struct Node *current = head;
	if(current==NULL){
		printf("No task in the queue.\n");
		return;
	}
	while(current != NULL){
		printf("%d\t%s\t%d\t%d\n", current->data.pid, current->data.task_name,current->data.task_state, current->data.time_quantum);
        current = current->next;
    }
}

void free_all(){
	struct Node* current = head;
   	struct Node* next;
	while (current != NULL)
	{
		next = current->next;
		free(current);
		current = next;
	}
	head = NULL;
}
