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
static int endClock = 0;
char *scheduleResult;

/* ----------------------------------------- STATES -------------------------------------------*/

void * getNextTask(void) {
	printf("Getting next task...\n");
	int ind;

	clock++;
	printf("\nClock: %d\n", clock);

	it = 0;
	finished = 0;

	ind = currentTask->ind;
    currentTask = getMostPriority();

	if(ind != currentTask->ind) {
		if(currentTask->execTime > 0)
			currentTask->state = BLOCKED;
	}

    return checkExecTime;
}

void * checkExecTime(void) {
	printf("Checking execution time...\n");
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

			printf("Running task: %d\n", currentTask->ind);

			running = 1;

			break;
		case idle:

			printf("Running idle task...\n");
			currentTask->ind = N_TASKS-2;
			running = 1;

			break;
	}

	return waitNextIT;
}

void * waitNextIT(void) {
	//printf("Waiting next IT: %d -- %d -- Time: %d\n", it, finished, TIMER1);
	delay_ms(100);

	return (it && finished) ? getNextTask : waitNextIT;
}

/* -----------------------FUNCTIONS ------------------------------------*/

uint8_t checkExecutionTime(void) {
    return (currentTask->execTime > 0) ? 1 : 0;
}

task * getMostPriority(void) {
	task *p;
    task *auxTask;
   
	auxTask->period = MAX_PERIOD;

	for(int i = 0; i < list_count(l); i++) {
		p = list_get(l, i);

		/* 
		
		
		DEBUGAR TEMPO 11!! E VER POR QUE SELECIONOU A TAREFA ERRADA!
		
		*/
		if(auxTask->period > p->period && p->execTime > 0) {
			auxTask = p;
		}
	}
	
	auxTask->execTime = (auxTask->period == MAX_PERIOD) ? 0 : auxTask->execTime;

	return auxTask;
}

void updateFSM(void) {
	printf("Retornando... %d", N_TASKS-1);
	currentState = (state_func)(*currentState)();
}

void initTaskValue(void) {
	uint8_t periods[N_TASKS-2];

    for (int i = 0; i < N_TASKS-2; i++) {
		tasks[i].ind = i;
        tasks[i].execTime = (random() % 5) + 1;
        tasks[i].period = tasks[i].execTime + (random() % 5) + 1;
		periods[i] = tasks[i].period;
        tasks[i].deadline = tasks[i].period;
		printf("%d: %d -- %d -- %d\n", i, tasks[i].execTime, tasks[i].period, tasks[i].deadline);
    }

	memcpy(auxTasks, tasks, sizeof(tasks));

	endClock = mmc(periods[0],mmc(periods[1],periods[2]));
}

void timer1ctc_handler(void) {
	printf("Interruption!\n");
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
			delay_ms(500);
		}

	}
}

void idle_task(void) {
	volatile char cushion[1000];	/* reserve some stack space */
	cushion[0] = '@';		/* don't optimize my cushion away */

	//printf("Saving idle task context: %X\n", jmp[3]);
	if (!setjmp(jmp[3]))
		scheduler();

	while (1) {			/* thread body */
		printf("\nidle task...\n");
		delay_ms(100);
		it = 0;
		finished = 1;

		scheduleResult[strlen(scheduleResult)] = '.';
		context_switch(N_TASKS-1); //pula pro scheduler
	}
}

void task2(void) {
	task *ptr;
	volatile char cushion[1000];	/* reserve some stack space */
	cushion[0] = '@';		/* don't optimize my cushion away */

	//printf("Saving task2 context: %X\n", jmp[2]);
	if (!setjmp(jmp[2]))
		idle_task();

	while (1) {			/* thread body */
		delay_ms(100);

		ptr = list_get(l, 2);

		if(ptr->state == RUNNING) {
			printf("\ntask 2...\n");
			ptr->state = READY;
			it = 0;
			finished = 1;

			scheduleResult[strlen(scheduleResult)] = '2';
			currentTask->execTime = currentTask->execTime > 0 ? currentTask->execTime - 1 : 0;
			updateCurrentTask();
		}

		context_switch(N_TASKS-1); //pula pro scheduler
	}
}

void task1(void) {
	task *ptr;
	volatile char cushion[1000];	/* reserve some stack space */
	cushion[0] = '@';		/* don't optimize my cushion away */

	//printf("Saving task1 context: %X\n", jmp[1]);
	if (!setjmp(jmp[1])) 
		task2();

	while (1) {			/* thread body */
		ptr = list_get(l, 1);

		if(ptr->state == RUNNING) {
			ptr->state = READY;
			
			it = 0;
			finished = 1;
			printf("task 1...\n");
			delay_ms(100);
			
			scheduleResult[strlen(scheduleResult)] = '1';
			currentTask->execTime = currentTask->execTime > 0 ? currentTask->execTime - 1 : 0;		
			updateCurrentTask();
		}
		context_switch(N_TASKS-1); //pula pro scheduler
	}
}

void task0(void) {
	task *ptr;
	volatile char cushion[1000];	/* reserve some stack space */
	cushion[0] = '@';		/* don't optimize my cushion away */

	//printf("Saving task0 context: %X\n", jmp[0]);
	if (!setjmp(jmp[0])) 
		task1();

	while (1) {			/* thread body */
		ptr = list_get(l, 0);
		
		delay_ms(100);

		if(ptr->state == RUNNING) {
			scheduleResult[strlen(scheduleResult)] = '0';

			ptr->state = READY;
			
			it = 0;
			finished = 1;
			printf("task 0...\n");
			currentTask->execTime = currentTask->execTime > 0 ? currentTask->execTime - 1 : 0;
			updateCurrentTask();
		}

		context_switch(N_TASKS-1); //pula pro scheduler
	}
}

void context_switch(int ind) {
	it = 0;

	if (!setjmp(jmp[curl])) { //setjump retorna 0 na primeira vez
		// if (N_TASKS == ++curl)
		// 	curl = 0;
		curl = ind;
		printf("Jumping to %d...\n", curl);
		longjmp(jmp[curl], 1);
	}
}

void updateCurrentTask(void) {
	task *ptr;

	for (int i = 0; i < list_count(l); i++) {
		ptr = list_get(l, i);

		//printf("Task %d -- Period %d -- Deadline %d -- ExecTime %d\n", ptr->ind, ptr->period, ptr->deadline, ptr->execTime);
		if(ptr->deadline == clock) {
			ptr->deadline *= 2;

			if(ptr->execTime == 0) 
				ptr->execTime = auxTasks[ptr->ind].execTime;
		}
	}
}

void initList(void) {
    for (int i = 0; i < N_TASKS-2; i++)
		if(list_append(l, &tasks[i])) printf("FAIL!");
}

void showResult(void) {
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

int main(void) {

	printf("Starting...\n");
	heap_init((uint32_t *)&mem_pool, sizeof(mem_pool));
	l = list_init();
	
	initTaskValue();

	scheduleResult = (char *)malloc(endClock * sizeof(char));

	initList();
    initTIM1();

	task0();

	while(1);

	return 0;
}