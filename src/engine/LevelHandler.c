#define CGLTF_IMPLEMENTATION

#include <cglm/affine.h> // IWYU pragma: keep
#include "def.h"
#include "Mesh.h"
#include "PathHandler.h"
#include "LevelHandler.h"

#define throwStructureFatal(fileName, message) throwFatal(fileName, "This level file has a weird structure!\n"message);

static SceneNode root;

static MeshID numMeshes;
static InstancePtrID numInstances;

static Mesh* meshes;

static MeshID* instancesMeshIds;
static mat4* transforms;

static char** meshNames;

static char* openLevelFile_alloc(const char fileName[const], size_t* const outFileSize) {
    const char firstStr[] = "levels\\";

    const PathStringSize relPathStrSize = sizeof(firstStr) + strlen(fileName);

    char relPathStr[relPathStrSize];
    
    strcpy(relPathStr, firstStr);
    strcat(relPathStr, fileName);

    return PH_OpenFile(relPathStr, relPathStrSize, outFileSize);
}
static int jsmnOpenLevelFile_alloc(
    const char fileName[const], char* outJS[const], jsmntok_t tokens[const], const size_t maxTokens
) {
    jsmn_parser parser;

    size_t fileSize;

    char* const js = openLevelFile_alloc(fileName, &fileSize);

    jsmn_init(&parser);

    *outJS = js;

    return jsmn_parse(&parser, js, fileSize - 1, tokens, maxTokens);
}
static int tokenStrCmp(const char js[const], const char str[const], const jsmntok_t* const token) {
    return strncmp(js + token->start, str, token->end - token->start);
}
static void addMesh(const char js[const], const jsmntok_t* const token, const MeshID meshId) {
    const ptrdiff_t start = token->start, meshNameLength = token->end - start;

    char tempName[meshNameLength + 1];

    memcpy(tempName, js + start, meshNameLength);

    tempName[meshNameLength] = '\0';

    const char* const found = strrchr(tempName, '.');
    const size_t shortNameLength = found ? (found - tempName ? found - tempName : meshNameLength) : meshNameLength;

    meshNames[meshId] = mallocd(shortNameLength + 1);

    memcpy(meshNames[meshId], js + start, shortNameLength);

    meshNames[meshId][shortNameLength] = '\0';

    Mesh_Init(meshes + meshId, GRAPHICS_PIPELINE_STATIC, tempName);
}
static int processNode(
    SceneNode* const node, const jsmntok_t tokens[const], const char js[const], const char fileName[const], int i
) {
    if (node != &root) {
	const ptrdiff_t length = tokens[i].end - tokens[i].start;

	node->name = mallocd(length + 1);

	memcpy(node->name, js + tokens[i].start, length);

	node->name[length] = '\0';

	node->unique = i;
    }

    if (tokens[++i].type != JSMN_OBJECT) throwStructureFatal(fileName, "Expected an JSON object after label");

    const int nodeSize = tokens[i].size;

    //Oh no... If this node has any instances (':' label), 24 bytes are wasted...
    node->children = callocd(nodeSize, sizeof(*node->children));

    for (node->numChildren = 0; node->numChildren < nodeSize; node->numChildren++) {
	if (tokens[++i].type != JSMN_STRING) throwStructureFatal(fileName, "Expected a string");

	if (!tokenStrCmp(js, ":", tokens + i)) {
	    if (tokens[++i].type != JSMN_ARRAY) throwStructureFatal(fileName, "Expected an array after ':' label");

	    if (tokens[i].size) {
		node->numInstances = tokens[i].size;

		node->instances = mallocd(root.numInstances * sizeof(*root.instances));
	    }

	    for (unsigned int j = 0; j < node->numInstances; j++) node->instances[j] = strtoul(js + tokens[++i].start, NULL, 0);
	}
	else i = processNode(node->children + node->numChildren, tokens, js, fileName, i);
    }

    return i;
}
static int parseLabel(const jsmntok_t tokens[const], const char js[const], const char fileName[const], int i) {
    if (tokens[i].type != JSMN_STRING) throwStructureFatal(fileName, "Level files must not have anonymous JSON objects");

    if (!tokenStrCmp(js, "static", tokens + i)) {
	if (tokens[++i].type != JSMN_OBJECT) throwStructureFatal(fileName, "Expected an JSON object after 'static' label");

	InstancePtrID nextInstance;

	const unsigned int staticSize = tokens[i].size;

	printfd("Number of meshes: %i\n", staticSize);

	numInstances = nextInstance = 0;

	meshes = mallocd(staticSize * sizeof(*meshes));
	meshNames = mallocd(staticSize * sizeof(*meshNames));

	for (; numMeshes < staticSize; numMeshes++) {
	    if (tokens[++i].type != JSMN_STRING) throwStructureFatal(fileName, "Expected a mesh file name");

	    const int mat4num = sizeof(mat4) / sizeof(float);

	    addMesh(js, tokens + i, numMeshes);

	    if (tokens[++i].type != JSMN_ARRAY) {
		throwStructureFatal(fileName, "Expected an array of floats after a mesh declaration");
	    }

	    const int arraySize = tokens[i].size;

	    if (!arraySize) {
		throwStructureFatal(fileName, "A mesh is declared, but it has no instances");
	    }
	    if (arraySize % mat4num != 0) {
		throwStructureFatal(fileName, "Expected 4x4 matrix for each instance");
	    }

	    numInstances += arraySize / mat4num;

	    instancesMeshIds = reallocd(instancesMeshIds, numInstances * sizeof(*instancesMeshIds));
	    transforms = reallocd(transforms, numInstances * sizeof(*transforms));

	    /*
	    float* data;

	    for (int j = 0; j < arraySize; j++) {
		if (j % mat4num == 0) {
		    const InstancePtrID id = IH_NewInstance(meshes + numMeshes);

		    data = IH_GetUploadPtr(id);

		    instancesMeshIds[nextInstance++] = numMeshes;
		}

		//is bro cooking?
		data[j % mat4num] = strtof(js + tokens[++i].start, NULL);
		transforms[nextInstance - 1][0][j % mat4num] = data[j % mat4num];
	    }
	    */
	}
    }
    else if (!tokenStrCmp(js, "root", tokens + i)) i = processNode(&root, tokens, js, fileName, i);
    else throwStructureFatal(fileName, "Unknown label");

    return i + 1;
}
static void checkParseResult(const int result, const char fileName[const]) {
    switch (result) {
    case JSMN_ERROR_INVAL:
	throwFatal(fileName, "This json file is corrupted!");
	break;
    case JSMN_ERROR_NOMEM:
	throwFatal(fileName, "This json file is too large!");
	break;
    case JSMN_ERROR_PART:
	throwFatal(fileName, "This json file is too short!");
	break;
    }
}

void LH_Load(const char fileName[const]) {
    printfd("Loading a level: %s\n", fileName);

    const size_t maxTokens = 128;

    int i;

    jsmntok_t tokens[maxTokens];

    char* js;

    const int numTokens = jsmnOpenLevelFile_alloc(fileName, &js, tokens, maxTokens);

    checkParseResult(numTokens, fileName);

    printfd("Number of tokens inside json: %i\n", numTokens);

    i = 0;

    while (i < numTokens) i = parseLabel(tokens, js, fileName, i);
   
    free(js);
}
InstancePtrID LH_GetInstancesCount() {
    return numInstances;
}
Mesh* LH_GetInstanceMesh(const InstancePtrID ptrId) {
    return meshes + instancesMeshIds[ptrId];
}
char* LH_GetInstanceName(const InstancePtrID ptrId) {
    return meshNames[instancesMeshIds[ptrId]];
}
vec4* LH_GetInstanceTransform(const InstancePtrID ptrId) {
    return transforms[ptrId];
}
const SceneNode* LH_GetRoot() {
    return &root;
}
void LH_Loop() {

}
