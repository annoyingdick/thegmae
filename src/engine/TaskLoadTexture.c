#include "TaskLoadTexture.h"

bool TaskLoadTexture_IsLoaded(const TaskLoadTexture* const task) {
    return task->numBatches;
}
bool TaskLoadTexture_IsUploaded(const TaskLoadTexture* const task) {
    return task->nextBatchId < 2;
}
bool TaskLoadTexture_IsValid(const TaskLoadTexture* const task) {
    return task->path;
}
