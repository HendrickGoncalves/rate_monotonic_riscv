#include <hf-risc.h>
#include <list.h>

#ifndef RM_H
#define RM_H

#define N_TASKS 5
#define MAX_PERIOD 4000000

typedef enum task_state{
    BLOCKED = 0,
    READY,
    RUNNING
} taskState;

typedef enum task_type {
    idle = 0,
    periodic
} taskType;

typedef struct task_info {
    int ind;
    uint32_t execTime;
    uint32_t period;
    uint32_t deadline;
    uint8_t prempFlag;
    taskState state;
} task;

typedef uint32_t jmp_buf[20];
int32_t setjmp(jmp_buf env);
void longjmp(jmp_buf env, int32_t val);

/* -----------------------------------SCHEDULER STATES -----------------------------*/

void * getNextTask(void);
void * checkExecTime(void);
void * waitNextIT(void);
void * run(void);

/* -----------------------------------Functions -----------------------------*/

void *memcpy(void *dst, const void *src, uint32_t n);
uint8_t checkExecutionTime(void);
void initTIM1(void);
uint8_t checkExecutionTime(void);
task * getMostPriority(void);
void updateCurrentTask(void);
void context_switch(int ind);
int mdc(int a, int b);
int mmc(int a, int b);
void showResult(void);

/* ----------------------------------TASKS---------------------------------------*/

void scheduler(void);

void task0(void);
void task1(void);
void task2(void);
void idle_task(void);

#endif