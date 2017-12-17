#include "scheduling_simulator.h"

#define _XOPEN_SOURCE_EXTENDED 1

#ifdef _LP64
#define STACK_SIZE 2097152+16384 /* large enough value for AMODE 64 */
#else
#define STACK_SIZE 16384  /* AMODE 31 addressing*/
#endif

struct Data {
	int pid;
	char task_name[10];
	ucontext_t context;
	enum TASK_STATE task_state;
	int time_quantum;
	int queueing_time;
};
struct Node {
	struct Data data;
	struct Node *next;
};

static ucontext_t mcontext;
static ucontext_t timer_context;
static ucontext_t newcontext;

static struct Node* head = NULL;
static struct Node *current_node;
static int pid_counter = 1;
static void *timer_stack;
static struct itimerval t;

void scheduler(void);
void timer_handler(int j);
void pause_handler(int sig);
void hw_suspend(int msec_10);
void hw_wakeup_pid(int pid);
int hw_wakeup_taskname(char *task_name);
int hw_task_create(char *task_name);
void task1(void);
void task2(void);
void task3(void);
void task4(void);
void task5(void);
void task6(void);
void add_task(char *task_name, int time_quantum);
void remove_task(int pid);
void start_simulation(void);
void process_status(void);
void free_all(void);


int main()
{
	signal(SIGTSTP, pause_handler);
	/* allocate the global timer interrupt stack */
	timer_stack = malloc(STACK_SIZE);
	if (timer_stack == NULL) {
		perror("malloc");
		exit(1);
	}
	char command[1024];
	while (1) {
		getcontext(&mcontext);
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
			} else {
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
			/* setup our timer */
			current_node=head;
			if(current_node==NULL) {
				printf("No task in the queue.\n");
				continue;
			} else {
				printf("simulating...\n");
				t.it_interval.tv_sec = 0;
				t.it_interval.tv_usec = 500*1000;
				t.it_value = t.it_interval;
				if (setitimer(ITIMER_REAL, &t, NULL) ) perror("setitiimer");
				signal(SIGALRM, timer_handler);
				/* force a swap to the first context */
				setcontext(&current_node->data.context);
			}
		} else if (strcmp(type, "ps") == 0) {
			printf("information is...\n");
			process_status();
		} else {

		}
	}
	free_all();
	return 0;
}
/* The scheduling algorithm; selects the next context to run, then starts it. */
void scheduler(void)
{
	printf("Schedule in task's PID\t:\t%d\n", current_node->data.pid);
	if(current_node->next==NULL) {
		current_node = head;
	} else {
		current_node = current_node->next;
	}
	printf("Schedule out task's PID\t:\t%d\n\n", current_node->data.pid);
	setcontext(&current_node->data.context);
}

/*
  Timer interrupt handler.
  Creates a new context to run the scheduler in, and swaps
  contexts saving the previously executing task and jumping to the
  scheduler.
*/
void timer_handler(int j)
{
	/* Create new scheduler context */
	getcontext(&timer_context);
	timer_context.uc_stack.ss_sp = timer_stack;
	timer_context.uc_stack.ss_size = STACK_SIZE;
	timer_context.uc_stack.ss_flags = 0;
	makecontext(&timer_context, scheduler,0);
	/* save running task, jump to scheduler */
	swapcontext(&current_node->data.context,&timer_context);
}
void pause_handler(int sig)
{
	t.it_interval.tv_usec = 0;
	t.it_interval.tv_sec = 0;
	t.it_value = t.it_interval;

	if( setitimer( ITIMER_REAL, &t, NULL) < 0 ) {
		printf("settimer error.\n");
		exit(1);
	}
	signal(SIGALRM, timer_handler);
	//printf(" Your input is Ctrl + Z\n");
	printf("\n");
	setcontext(&mcontext);
}

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
	void * stack;

	getcontext(&newcontext);

	stack = malloc(STACK_SIZE);
	if (stack == NULL) {
		perror("malloc");
		exit(1);
	}
	/* we need to initialize the ucontext structure, give it a stack,
	    flags, and a sigmask */
	newcontext.uc_stack.ss_sp = stack;
	newcontext.uc_stack.ss_size = STACK_SIZE;
	newcontext.uc_stack.ss_flags = 0;

	/* setup the function we're going to. */
	if(strcmp(task_name,"task1")==0) {
		makecontext(&newcontext, task1, 0);
		printf("context is %p\n", &newcontext);
		return pid_counter++;
	} else if(strcmp(task_name,"task2")==0) {
		makecontext(&newcontext, task2, 0);
		printf("context is %p\n", &newcontext);
		return pid_counter++;
	} else if(strcmp(task_name,"task3")==0) {
		makecontext(&newcontext, task3, 0);
		printf("context is %p\n", &newcontext);
		return pid_counter++;
	} else if(strcmp(task_name,"task4")==0) {
		makecontext(&newcontext, task4, 0);
		printf("context is %p\n", &newcontext);
		return pid_counter++;
	} else if(strcmp(task_name,"task5")==0) {
		makecontext(&newcontext, task5, 0);
		printf("context is %p\n", &newcontext);
		return pid_counter++;
	} else if(strcmp(task_name,"task6")==0) {
		makecontext(&newcontext, task6, 0);
		printf("context is %p\n", &newcontext);
		return pid_counter++;
	} else {
		return -1;
	}
}

void add_task(char *task_name, int time_quantum)
{
	//printf("Added task:\n");
	//printf("task name: %s\n", task_name);
	//printf("time quantum (ms): %d\n", time_quantum);
	struct Node *last = head;
	struct Node *newNode;
	newNode = malloc(sizeof(struct Node));
	int pid = hw_task_create(task_name);
	if(pid==-1) {
		printf("No such task name to create.\n");
		return;
	}
	strcpy(newNode->data.task_name, task_name);
	newNode->data.context = newcontext;
	newNode->data.pid=pid;
	newNode->data.task_state=TASK_READY;
	newNode->data.time_quantum=time_quantum;
	newNode->next = NULL;
	if (head == NULL) {
		head = newNode;
		return;
	}
	while (last->next != NULL) {
		last = last->next;
	}
	last->next = newNode;
	return;
}
void remove_task(int pid)
{
	struct Node *current = head;
	struct Node *prev;
	// If head node itself holds the pid to be deleted
	if (current != NULL && current->data.pid == pid) {
		head = current->next;
		free(current);
		return;
	}

	// Search for the pid to be deleted, keep track of the
	// previous node as we need to change 'prev->next'
	while (current != NULL && current->data.pid != pid) {
		prev = current;
		current = current->next;
	}

	// If pid was not present in linked list
	if (current == NULL) {
		printf("No such pid in the queue.\n");
		return;
	}

	// Unlink the node from linked list
	prev->next = current->next;

	free(current);
	return;
}
void process_status()
{
	struct Node *current = head;
	if(current==NULL) {
		printf("No task in the queue.\n");
		return;
	}
	while(current != NULL) {
		printf("%d\t%s\t%d\t%d\n", current->data.pid, current->data.task_name,
		       current->data.task_state, current->data.time_quantum);
		current = current->next;
	}
}

void free_all()
{
	struct Node* current = head;
	struct Node* next;
	while (current != NULL) {
		next = current->next;
		free(current);
		current = next;
	}
	head = NULL;
}
