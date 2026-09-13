#include <pthread.h>
#include "Array.h"
#include "TaskManager.h"

#define NUM_WORKERS 1
#define INIT_NUM_TASKS 2

#define FORBID_SLEEPING

typedef struct {
    Task* elements;
    volatile ArrayIndex numElements;
    ArrayIndex sizeInElements;
} TaskArray;

DECLARE_ARRAY_IMPL(Task)

static pthread_mutex_t mutex;
static pthread_cond_t notify;

static TaskArray tasks;

static void workerThrd() {
    for (;;) {
#ifdef FORBID_SLEEPING
	if (!tasks.numElements) continue;
#endif
	pthread_mutex_lock(&mutex);

#ifndef FORBID_SLEEPING
	if (!tasks.numElements) pthread_cond_wait(&notify, &mutex);
#endif

	const Task* const task = TaskArray_GetLastElement(&tasks);
	void (*const func)(void*) = task->function;

	void* const arg = task->argument;

	tasks.numElements--;

	pthread_mutex_unlock(&mutex);

	func(arg);
    }
}

void TM_Init() {
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&notify, NULL);

    TaskArray_Init(&tasks);

    for (unsigned int i = 0; i < NUM_WORKERS; i++) pthread_create(NULL, NULL, (void*)workerThrd, NULL);
}
void TM_AddTask(const Task* const task) {
    pthread_mutex_lock(&mutex);

    //lock mutex because of the dynamic array growth and the risk of dangling pointer situation
    TaskArray_AppendElement(&tasks, task);

#ifndef FORBID_SLEEPING
    if (tasks.numElements == 1) pthread_cond_signal(&notify);
#endif

    pthread_mutex_unlock(&mutex);
}
