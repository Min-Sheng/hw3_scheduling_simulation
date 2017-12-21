#include "scheduling_simulator.h"

#define _XOPEN_SOURCE_EXTENDED 1

#ifdef _LP64
	#define STACK_SIZE 2097152+16384	/* Large enough value for AMODE 64 */
#else
	#define STACK_SIZE 16384			/* AMODE 31 addressing */
#endif

/* Task queue data structure */
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

static ucontext_t mcontext;				/* Main function context */
static ucontext_t signal_context;		/* Signal function context */
static ucontext_t scheduler_context;	/* Scheduler function context */
static ucontext_t newcontext;			/* New context for new task */
static void *signal_stack;				/* Stack pointer for signal function */
static void *scheduler_stack;			/* Stack pointer for scheduler function*/
static struct itimerval t;				/* Timer interval */

static struct Node* head = NULL;		/* Node pointer for head node */
static struct Node *current_node;		/* Node pointer for current node */
static int pid_counter = 1;


void scheduler(void);
void signal_function(void);
void timer_handler(int sig);
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
	/* Activate the pause handler */
	signal(SIGTSTP, pause_handler);

	/* Allocate the global signal function stack */
	signal_stack = malloc(STACK_SIZE);
	if (signal_stack == NULL) {
        perror("malloc");
        exit(1);
    }

	/* Allocate the global scheduler function stack */
	scheduler_stack = malloc(STACK_SIZE);
    if (scheduler_stack == NULL) {
        perror("malloc");
        exit(1);
    }

	char command[1024];
	while (1) {
		/* Get the main function context */
		getcontext(&mcontext);

		/* Make the scheduler function context for the fist time */
		getcontext(&scheduler_context);
		scheduler_context.uc_stack.ss_sp = scheduler_stack;
		scheduler_context.uc_stack.ss_size = STACK_SIZE;
		scheduler_context.uc_stack.ss_flags = 0;
		scheduler_context.uc_link = &mcontext;
		makecontext(&scheduler_context, scheduler, 0);

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
			printf("simulating...\n");
			/* Swap the context from main function to scheduler function */
			swapcontext(&mcontext, &scheduler_context);
		} else if (strcmp(type, "ps") == 0) {
			printf("information is...\n");
			process_status();
		} else {

		}
	}
	free_all();
	return 0;
}
/* The RR scheduling algorithm; selects the next ready task to run and swaps to it's context to start it; if the task terminates, it will swap back and the scheduler will reschedule */
void scheduler(void)
{
	ucontext_t recontext;
	getcontext(&recontext);

	t.it_interval.tv_sec = 0;
	t.it_interval.tv_usec = 0;
	t.it_value = t.it_interval;
	if (setitimer(ITIMER_REAL, &t, NULL) < 0)
	{
		printf("settimer error.\n");
		exit(1);
	}
	signal(SIGALRM, timer_handler);

	if(current_node==NULL){
		printf("No task in ready queue.\n");
		return;
	}

	struct Node *original_node = current_node;
	while (current_node->data.task_state != TASK_READY&&current_node->data.task_state !=TASK_RUNNING)
	{
		if(current_node->next==NULL) {
			current_node = head;
		}else{
			current_node = current_node->next;
		}
		if(original_node==current_node){
			printf("No task in ready queue.\n");
			t.it_interval.tv_usec = 0;
			t.it_interval.tv_sec = 0;
			t.it_value = t.it_interval;
			if( setitimer( ITIMER_REAL, &t, NULL) < 0 ){
				printf("settimer error.\n");
				exit(1);
			}
			signal(SIGALRM, timer_handler);
			return;
		}
	}

	struct Node *current = head;
	while (current!=NULL){
	if(current!=current_node){
		current->data.queueing_time += current_node->data.time_quantum;
	}
		current = current->next;
	}

	t.it_interval.tv_sec = 1;
	//t.it_interval.tv_usec = current_node->data.time_quantum * 1000;
	t.it_interval.tv_usec = 0;
	t.it_value = t.it_interval;
	if( setitimer( ITIMER_REAL, &t, NULL) < 0 ){
		printf("settimer error.\n");
		exit(1);
	}
	signal(SIGALRM, timer_handler);

	printf("Schedule in task's PID\t:\t%d\n", current_node->data.pid);
	current_node->data.task_state=TASK_RUNNING;
	swapcontext(&scheduler_context, &current_node->data.context);
	current_node->data.task_state=TASK_TERMINATED;
	printf("Terminated task's PID\t:\t%d\n", current_node->data.pid);
	setcontext(&recontext);
}

/* The signal function; updates the current node, and
  makes and sets to new scheduler context to run the scheduler in */
void signal_function(void){
	printf("Schedule out task's PID\t:\t%d\n", current_node->data.pid);
	current_node->data.task_state = TASK_READY;

	if(current_node->next==NULL) {
		current_node = head;
	}else{
		current_node = current_node->next;
	}

	getcontext(&scheduler_context);
    scheduler_context.uc_stack.ss_sp = scheduler_stack;
    scheduler_context.uc_stack.ss_size = STACK_SIZE;
    scheduler_context.uc_stack.ss_flags = 0;
	scheduler_context.uc_link = &mcontext;
	makecontext(&scheduler_context, scheduler, 0);
	setcontext(&scheduler_context);
}

/* Timer interrupt handler; makes the new signal function context, saves the running task and swaps to signal function */
void timer_handler(int j)
{
	getcontext(&signal_context);
    signal_context.uc_stack.ss_sp = signal_stack;
    signal_context.uc_stack.ss_size = STACK_SIZE;
    signal_context.uc_stack.ss_flags = 0;
    makecontext(&signal_context, signal_function, 0);
	swapcontext(&current_node->data.context, &signal_context);
}

void pause_handler(int sig)
{
    t.it_interval.tv_usec = 0;
    t.it_interval.tv_sec = 0;
    t.it_value = t.it_interval;
    if( setitimer( ITIMER_REAL, &t, NULL) < 0 ){
        printf("settimer error.\n");
        exit(1);
    }
    signal(SIGALRM, timer_handler);

    //printf(" Your input is Ctrl + Z\n");
    printf("\n");

	swapcontext(&scheduler_context, &mcontext);
	getcontext(&scheduler_context);
    scheduler_context.uc_stack.ss_sp = scheduler_stack;
    scheduler_context.uc_stack.ss_size = STACK_SIZE;
    scheduler_context.uc_stack.ss_flags = 0;
	scheduler_context.uc_link = &mcontext;
	makecontext(&scheduler_context, scheduler, 0);
	setcontext(&scheduler_context);
	swapcontext(&mcontext,&scheduler_context);
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
	newcontext.uc_stack.ss_sp = stack;
	newcontext.uc_stack.ss_size = STACK_SIZE;
	newcontext.uc_stack.ss_flags = 0;
	newcontext.uc_link = &scheduler_context;

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
	newNode->data.queueing_time=0;
	newNode->next = NULL;
	if (head == NULL) {
		head = newNode;
		current_node=head; /* Allocate the head to current node when head is builded */
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
	/* If head node itself holds the pid to be deleted */
	if (current != NULL && current->data.pid == pid) {
		if(current_node==head){
			head = current->next;
			current_node = head;
		}else{
			head = current->next;
		}
		free(current);
		return;
	}

	/* Search for the pid to be deleted, keep track of the previous node as we need to change 'prev->next' */
	while (current != NULL && current->data.pid != pid) {
		prev = current;
		current = current->next;
	}

	/* If pid was not present in linked list */
	if (current == NULL) {
		printf("No such pid in the queue.\n");
		return;
	}

	/* Unlink the node from linked list */
	if(current_node==current){
		prev->next = current->next;
		if(current->next==NULL){
			current_node = head;
		}else{
			current_node = current->next;
		}
	}else{
		prev->next = current->next;
	}
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
		//printf("%d\t%s\t%d\t%d\n", current->data.pid, current->data.task_name,
		//       current->data.task_state, current->data.time_quantum);
		char *state="";
		switch(current->data.task_state){
			case TASK_RUNNING:
				state = "TASK_RUNNING";
				break;
			case TASK_READY:
				state = "TASK_READY";
				break;
			case TASK_WAITING:
				state = "TASK_WAITING";
				break;
			case TASK_TERMINATED:
				state = "TASK_TERMINATED";
				break;
			default:;
		}
			printf("%d\t%s\t%s\t%d\n", current->data.pid, current->data.task_name,
				   state, current->data.queueing_time);
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
