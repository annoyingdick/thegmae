#ifndef LevelHandler_h_
#define LevelHandler_h_

#include "SceneNode.h"

void LH_Load(const char fileName[]);
InstancePtrID LH_GetInstancesCount();
Mesh* LH_GetInstanceMesh(InstancePtrID ptrId);
char* LH_GetInstanceName(InstancePtrID ptrId);
vec4* LH_GetInstanceTransform(InstancePtrID ptrId);
const SceneNode* LH_GetRoot();
void LH_Loop();

#endif
