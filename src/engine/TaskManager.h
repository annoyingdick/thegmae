#ifndef TaskManager_h_
#define TaskManager_h_

typedef struct {
    void (*function)(void*);
    void* argument;
} Task;

void TM_Init();
void TM_AddTask(const Task* task);

#endif
