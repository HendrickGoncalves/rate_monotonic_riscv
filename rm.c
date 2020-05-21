#include "rm.h"

struct list *l;
typedef void *(*state_func)();
task tasks[N_TASKS], auxTasks[N_TASKS];
state_func currentState;

static jmp_buf jmp[N_TASKS];

char mem_pool[8192]; //espaço de memória dedicada a alocação dinamica -- pode ser aumentado um pouco -- maximo 1M , usar 64k

static int32_t clock = 0;
task *currentTask;
taskType type;

static uint8_t it = 0;
static uint8_t started = 0;
static int curl;
static uint8_t finished = 1;
static uint8_t running = 0;
static uint8_t end = 0;
static int endClock = 0;
char *scheduleResult;

/* ----------------------------------------- STATES -------------------------------------------*/

void * getNextTask(void) {
	task *ptr;

	clock++;

	it = 0;
	finished = 0;

	ptr = currentTask;
    currentTask = getMostPriority();

	if(ptr->ind != currentTask->ind && clock > 1) {
		if(ptr->execTime > 0) 
			ptr->state = BLOCKED;
	}

    return checkExecTime;
}

void * checkExecTime(void) {
    if(checkExecutionTime()) {
        type = periodic;
        return run;
    }
    	
    type = idle;

    return run;
}

void * run(void) {
	task *ptr;

	switch (type) {
		case periodic:
			ptr = list_get(l, currentTask->ind);
			ptr->state = RUNNING;

			running = 1;

			break;
		case idle:
			currentTask->ind = N_TASKS-2;
			running = 1;

			break;
	}

	return waitNextIT;
}

void * waitNextIT(void) {
	return (it && finished) ? getNextTask : waitNextIT;
}

/* -----------------------FUNCTIONS ------------------------------------*/

uint8_t checkExecutionTime(void) {
    return (currentTask->execTime > 0) ? 1 : 0;
}

task * getMostPriority(void) {
	task *p;
	uint32_t period = MAX_PERIOD;
    task *auxTask;
   
	for(int i = 0; i < list_count(l); i++) {
		p = list_get(l, i);

		if(period > p->period && p->execTime > 0) {
			auxTask = p;
			period = p->period;
		}
	}

	return auxTask;
}

void timer1ctc_handler(void) {
	it = 1;
}

void initTIM1(void) {
    TIMER1PRE = TIMERPRE_DIV64; 

	/* unlock TIMER1 for reset */
	TIMER1 = TIMERSET;
	TIMER1 = 0;

	/* TIMER1 frequency: 5ms */
	TIMER1CTC = 50000; //counter compare

	/* enable interrupt mask for TIMER1 CTC events */
	TIMERMASK |= MASK_TIMER1CTC;
}

void scheduler(void) {
	volatile char cushion[1000];	/* reserve some stack space */
	cushion[0] = '@';		/* don't optimize my cushion away */
	curl = N_TASKS - 1;		/* the first thread to context switch is this one */

	currentState = getNextTask;

	setjmp(jmp[N_TASKS-1]);

	while(1) {
		if(clock <= endClock) {
			if(running)
				context_switch(currentTask->ind);
			running = 0;

			currentState = (state_func)(*currentState)();
		} else {
			printf("Schedule finished: %d\n", endClock);
			showResult(); 
			end = 1;
			return;
		}

	}
}

void idle_task(void) {
	volatile char cushion[1000];	/* reserve some stack space */
	cushion[0] = '@';		/* don't optimize my cushion away */

	if (!setjmp(jmp[3]))
		scheduler();

	while (1) {			/* thread body */
		
		if(!end) {
			printf("\nidle task...\n");

			it = 0;
			finished = 1;

			scheduleResult[strlen(scheduleResult)] = '.';
			updateCurrentTask();

			context_switch(N_TASKS-1); //pula pro scheduler
		} else 
			return;
	}
}

void task2(void) {
	task *ptr;
	volatile char cushion[1000];	/* reserve some stack space */
	cushion[0] = '@';		/* don't optimize my cushion away */

	if (!setjmp(jmp[2]))
		idle_task();

	while (1) {			/* thread body */

		if(!end) {
			ptr = list_get(l, 2);

			if(ptr->state == RUNNING) {
				printf("\ntask 2...\n");

				ptr->state = READY;

				it = 0;
				finished = 1;

				scheduleResult[strlen(scheduleResult)] = 'C';
				currentTask->execTime = currentTask->execTime > 0 ? currentTask->execTime - 1 : 0;
				updateCurrentTask();
			}

			context_switch(N_TASKS-1); //pula pro scheduler
		} else
			return;
	}
}

void task1(void) {
	task *ptr;
	volatile char cushion[1000];	/* reserve some stack space */
	cushion[0] = '@';		/* don't optimize my cushion away */

	if (!setjmp(jmp[1])) 
		task2();

	while (1) {			/* thread body */
		ptr = list_get(l, 1);

		if(!end) {
			if(ptr->state == RUNNING) {
				ptr->state = READY;
				
				it = 0;
				finished = 1;
				printf("task 1...\n");

				scheduleResult[strlen(scheduleResult)] = 'B';
				currentTask->execTime = currentTask->execTime > 0 ? currentTask->execTime - 1 : 0;		
				updateCurrentTask();
			}
			context_switch(N_TASKS-1); //pula pro scheduler
		} else 
			return;
	}
}

void task0(void) {
	task *ptr;
	volatile char cushion[1000];	/* reserve some stack space */
	cushion[0] = '@';		/* don't optimize my cushion away */

	if (!setjmp(jmp[0])) 
		task1();

	while (1) {			/* thread body */
		ptr = list_get(l, 0);
		
		if(!end) {
			if(ptr->state == RUNNING) {
				scheduleResult[strlen(scheduleResult)] = 'A';

				ptr->state = READY;
				
				it = 0;
				finished = 1;
				printf("task 0...\n");

				currentTask->execTime = currentTask->execTime > 0 ? currentTask->execTime - 1 : 0;
				
				updateCurrentTask();
			}

			context_switch(N_TASKS-1); //pula pro scheduler	
		} else
			return;
	}
}

void context_switch(int ind) {
	it = 0;

	if (!setjmp(jmp[curl])) { //setjump retorna 0 na primeira vez
		curl = ind;
		longjmp(jmp[curl], 1);
	}
}

void updateCurrentTask(void) {
	task *ptr;

	for (int i = 0; i < list_count(l); i++) {
		ptr = list_get(l, i);

		if(ptr->deadline == clock) {
			ptr->deadline += auxTasks[ptr->ind].deadline;

			if(ptr->execTime == 0) 
				ptr->execTime = auxTasks[ptr->ind].execTime;
			else 
				ptr->prempFlag = 1;
		} else if(ptr->prempFlag == 1 && ptr->execTime == 0) {
			ptr->execTime = auxTasks[ptr->ind].execTime;
			ptr->prempFlag = 0;
		}
		
	}
}

void initList(void) {
    for (int i = 0; i < N_TASKS-2; i++)
		if(list_append(l, &tasks[i])) printf("FAIL!");
}

void showResult(void) {
	scheduleResult[endClock] = '\0';
	printf("%s\n", scheduleResult);
}

int mdc(int a, int b){
    while(b != 0){
        int r = a % b;
        a = b;
        b = r;
    }
    return a;
}

int mmc(int a, int b){
    return a * (b / mdc(a, b));
}

void initStruct(void) {
	uint8_t periods[N_TASKS-2];

	tasks[0].ind = 0;
	tasks[0].state = READY;
	tasks[0].execTime = 3;
	tasks[0].period = 9;
	tasks[0].deadline = 9;
	periods[0] = tasks[0].period;

	tasks[1].ind = 1;
	tasks[1].state = READY;
	tasks[1].execTime = 4;
	tasks[1].period = 12;
	tasks[1].deadline = 12;
	periods[1] = tasks[1].period;

	tasks[2].ind = 2;
	tasks[2].state = READY;
	tasks[2].execTime = 4;
	tasks[2].period = 14;
	tasks[2].deadline = 14;
	periods[2] = tasks[2].period;
	
	memcpy(auxTasks, tasks, sizeof(tasks));

	endClock = mmc(periods[0],mmc(periods[1],periods[2]));
	printf("MMC: %d\n", endClock);
}

void show_list(void) {
	int32_t i;
	task *p;
	
	printf("\nshowing the list...\n");
	for (i = 0; i < list_count(l); i++){
		p = list_get(l, i);
		printf("%d: %d -- %d -- %d\n", i, p->execTime, p->period, p->deadline);
	}
}

int main(void) {

	heap_init((uint32_t *)&mem_pool, sizeof(mem_pool));
	l = list_init();

	initStruct();

	scheduleResult = (char *)malloc(endClock * sizeof(char));

	initList();
    initTIM1();

	task0();

	return 0;
}