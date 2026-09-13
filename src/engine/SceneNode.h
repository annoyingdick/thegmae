#ifndef SceneNode_h_
#define SceneNode_h_

#include "InstancesHandler.h"

typedef struct SceneNode SceneNode;

typedef uint8_t SceneNodeID;
typedef uint8_t SceneNodeNameSize;

struct SceneNode {
    int unique;
    InstancePtrID numInstances;
    SceneNodeID numChildren;

    SceneNode* children;
    InstancePtrID* instances;
    char* name;
};

#endif
