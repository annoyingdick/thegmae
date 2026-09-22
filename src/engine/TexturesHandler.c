#include <GL/glew.h>
#include "def.h"
#include "PathHandler.h"
#include "Pip.h"
#include "TexturesHandler.h"
#include "TaskManager.h"
#include "Arena.h"

#define STBI_MALLOC mallocd
#define STBI_REALLOC reallocd
#define STBI_FREE free
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define CHECKINIT(x) if (!texturesArena.size) throwFatal("R and TexturesHandler are not initialized!", x)

#define INIT_NUM_TASKS 2

#define MAX_TEXTURE_SIZE 4096
#define LOAD_SIZE (GLsizeiptr)(4 * MAX_TEXTURE_SIZE * MAX_TEXTURE_SIZE)
#define UPLOADBATCH_SIZE (512 * 512)

static Arena texturesArena, tasksArena;

static TextureID nextTextureId;
static TaskLoadTextureID nextTaskId;

static GLuint* textures;
static TextureID* numUsages;
static TaskLoadTexture** bindedTasks;
static char** names;

static TaskLoadTexture** tasks;
static GLuint* buffers;
static void** bufferPointers;

static UploadBatchID getNumBatchesForSize(const int w, const int h) {
    return (w * h / UPLOADBATCH_SIZE) + (w * h % UPLOADBATCH_SIZE > 0) + 1;
}
static TextureID createTexture(const char name[const]) {
    Region region;

    RegionSize newSize;

    const RegionSize oldSize = texturesArena.size;

    if (Arena_RequestRegion(&texturesArena, &region, &newSize, 1)) ++nextTextureId;
    if (newSize) {
	reallocarr(textures, newSize);
	reallocarr(numUsages, newSize);
	reallocarr(bindedTasks, newSize);
	reallocarr(names, newSize);

	R_ResizeTextureHandlesBuffer(oldSize, newSize);
    }

    numUsages[region.position] = 1;

    names[region.position] = strdup(name);

    glCreateTextures(GL_TEXTURE_2D, 1, textures + region.position);

    return region.position;
}
static bool areNamesSame(const TextureID id, const char name[]) {
    if (!strcmp(name, names[id])) {
	++numUsages[id];

	return true;
    }

    return false;
}
static bool isFenceSignaled(GLsync fence) {
    GLint buf;

    glGetSynciv(fence, GL_SYNC_STATUS, 1, NULL, &buf);

    return buf == GL_SIGNALED;
}
static void allocateTextureGL(const GLuint texture, const GLenum format, const int width, const int height) {
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureStorage2D(texture, 1, format, width, height);
}
//returns true if you won't need to upload batches anymore
static bool uploadBatch(TaskLoadTexture* const task) {
    const GLuint glTex = textures[task->textureId];
	
    if (!task->uploadSync) {
	GL_CHECK(glFlushMappedNamedBufferRange(buffers[task->loadId], 0, (GLsizeiptr)task->width * task->height * 4));

	allocateTextureGL(glTex, GL_RGBA8, task->width, task->height);

	task->uploadSync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    }
    else if (isFenceSignaled(task->uploadSync)) {
	//printf("TxID: %u, UPLOAD BATCH: %u\n", id, task->nextBatchId);

	const GLsizei height = UPLOADBATCH_SIZE / task->width;
	const GLint yOff = (--task->nextBatchId - 1) * height;

	glDeleteSync(task->uploadSync);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, buffers[task->loadId]);
	glTextureSubImage2D(
	    glTex, 0, 0, yOff, task->width, task->nextBatchId == task->numBatches ? task->height - yOff : height, 
	    GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, (void*)(GLsizeiptr)(yOff * task->width * 4)
	    //(void*)((task->loadId * LOAD_SIZE) + ((GLsizeiptr)yOff * task->width * 4))
	);

	if (TaskLoadTexture_IsUploaded(task)) {
	    task->uploadSync = GL_ZERO;

	    return true;
	}

	task->uploadSync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    }

    return false;
}
static bool isTextureValid(const TextureID id) {
    return names[id];
}
static stbi_uc* openImage(TaskLoadTexture* const task) {
    const char relPathPreStr[] = "mesh\\";

    const PathStringSize relPathStrSize = sizeof(relPathPreStr) + strlen(task->path);

    char relPathStr[relPathStrSize], absPathStr[PH_GetAbsolutePathStrSize(relPathStrSize)];

    strcpy_s(relPathStr, relPathStrSize, relPathPreStr);
    strcat_s(relPathStr, relPathStrSize, task->path);
    PH_GetAbsolutePathStr(absPathStr, relPathStr);

    return stbi_load(absPathStr, &task->width, &task->height, NULL, 4);
}
static void workerThrd(TaskLoadTexture* const task) {
    stbi_uc* const image = openImage(task);

    if (image) {
	if (task->width <= MAX_TEXTURE_SIZE && task->height <= MAX_TEXTURE_SIZE) {
	    memcpy(task->bufferPointer, image, (size_t)task->width * task->height * 4);

	    //SDL_Surface* const main = temp;

	    task->nextBatchId = getNumBatchesForSize(task->width, task->height);
	    task->numBatches = task->nextBatchId - 1;
	}
	else throwFatal(task->path, "Max texture size is 4096 by 4096 pixels");
    }
    else throwFatal(task->path, "Image loading error occurred!");

    stbi_image_free(image);
}
static void createLoadTask(TaskLoadTexture* const task, const char path[const]) {
    //pointer argument to task object that is located on stack
    task->uploadSync = GL_ZERO;

    task->nextBatchId = task->numBatches = 0;

    task->path = strdup(path);

    TM_AddTask(&(Task){.function = (void*)workerThrd, .argument = task});
}
static void createBuffers(const TaskLoadTextureID startId, const TaskLoadTextureID count) {
    glCreateBuffers(count, buffers + startId);

    for (TaskLoadTextureID i = startId; i < startId + count; i++) {
	const GLbitfield storageFlags = GL_MAP_WRITE_BIT + GL_MAP_PERSISTENT_BIT;

	glNamedBufferStorage(buffers[i], LOAD_SIZE, NULL, storageFlags);

	bufferPointers[i] = glMapNamedBufferRange(
	    buffers[i], 0, LOAD_SIZE, 
	    storageFlags + GL_MAP_INVALIDATE_BUFFER_BIT + GL_MAP_FLUSH_EXPLICIT_BIT + GL_MAP_UNSYNCHRONIZED_BIT
	);
    }
}
static void removeTask(TaskLoadTexture* const task) {
    if (task->uploadSync) glDeleteSync(task->uploadSync);

    nextTaskId -= Arena_ReturnRegion(&tasksArena, &(Region){.position = task->loadId, .size = 1});

    free(task->path);
    free(task);
}

void TexturesHandler_Init() {
    Arena_Init(&texturesArena, INIT_NUM_TEXTURES);

    mallocarr(textures, INIT_NUM_TEXTURES);
    mallocarr(numUsages, INIT_NUM_TEXTURES);
    mallocarr(bindedTasks, INIT_NUM_TEXTURES);
    mallocarr(names, INIT_NUM_TEXTURES);

    Arena_Init(&tasksArena, INIT_NUM_TASKS);

    mallocarr(tasks, INIT_NUM_TASKS);
    mallocarr(buffers, INIT_NUM_TASKS);
    mallocarr(bufferPointers, INIT_NUM_TASKS);

    createBuffers(0, INIT_NUM_TASKS);
}
GLuint TexturesHandler_GetGLTexture(const TextureID id) {
    return textures[id];
}
TextureID TexturesHandler_BeginLoadingTask(const char name[], const char path[const restrict]) {
    CHECKINIT("Tried to begin texture loading task");

    name = name ? name : path;

    //does this texture already exist?
    for (TextureID i = 0; i < nextTextureId; i++) {
	if (isTextureValid(i) && areNamesSame(i, name)) return i;
    }

    Region taskRegion;
    RegionSize newSize;

    const RegionSize oldTasksSize = tasksArena.size;

    const TextureID id = createTexture(name);

    if (Arena_RequestRegion(&tasksArena, &taskRegion, &newSize, 1)) ++nextTaskId;
    if (newSize) {
	reallocarr(tasks, newSize);
	reallocarr(buffers, newSize);
	reallocarr(bufferPointers, newSize);

	createBuffers(oldTasksSize, newSize - oldTasksSize);
    }

    TaskLoadTexture* const task = mallocd(sizeof(*task));

    bindedTasks[id] = task;

    tasks[taskRegion.position] = task;

    task->textureId = id;
    task->loadId = taskRegion.position;
    task->bufferPointer = bufferPointers[taskRegion.position];

    createLoadTask(task, path);

    return id;
}
TextureID TexturesHandler_LoadTextureRGB888(
    const unsigned char data[const], const int width, const int height, const char name[const]
) {
    const TextureID id = createTexture(name);

    allocateTextureGL(textures[id], GL_RGB8, width, height);

    glTextureSubImage2D(textures[id], 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);

    R_ShowTexture(id);

    return id;
}
void TexturesHandler_UnloadTexture(const TextureID id) {
    numUsages[id]--;
}
void TexturesHandler_Quit() {
    Arena_Destroy(&texturesArena);
    Arena_Destroy(&tasksArena);
}
void TexturesHandler_Loop() {
    //BENCHMARK_BEGIN
    //textures removal
    for (TextureID i = 0; i < nextTextureId; i++) {
	if (isTextureValid(i) && !numUsages[i]) {
	    if (!bindedTasks[i] || TaskLoadTexture_IsLoaded(bindedTasks[i])) {
		free(names[i]);

		//invalidate texture
		names[i] = NULL;

		nextTextureId -= Arena_ReturnRegion(&texturesArena, &(Region){.position = i, .size = 1});

		if (!bindedTasks[i]) glMakeTextureHandleNonResidentARB(glGetTextureHandleARB(textures[i]));
		else {
		    tasks[bindedTasks[i]->loadId] = NULL;

		    removeTask(bindedTasks[i]);

		    bindedTasks[i] = NULL;
		}

		glDeleteTextures(1, textures + i);

		R_DeleteTexture(i);
	    }
	}
    }
    //BENCHMARK_END

    for (TaskLoadTextureID i = 0; i < nextTaskId; i++) {
	TaskLoadTexture* const task = tasks[i];

	//printf("%u\n", i);

	if (task && TaskLoadTexture_IsLoaded(task) && uploadBatch(task)) {
	    //this texture is on gpu now, we are so bacc fuckers!
	    R_ShowTexture(task->textureId);

	    bindedTasks[task->textureId] = NULL;

	    removeTask(task);

	    tasks[i] = NULL;
	}
    }
}
