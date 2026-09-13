#include <cglm/vec3.h>
#include "Bone.h"
#include "Array.h"
#include "PathHandler.h"
#include "TaskManager.h"
#include "TexturesHandler.h"

#define REDEF_PRINTFD
#include "ModelsHandler.h"

typedef unsigned int TaskID;
typedef vec3 AABB[2];

typedef struct {
    ModelLoadInfo baseInfo;

    cgltf_data* data;
} TaskLoadModel;

typedef TaskLoadModel* TaskHandle;

DECLARE_ARRAY_TYPEDEF(TaskHandle)
DECLARE_ARRAY_IMPL(TaskHandle)

static TaskHandleArray tasks;

static bool isMaterialWithTexture(const cgltf_material* const material) {
    return material && material->pbr_metallic_roughness.base_color_texture.texture;
}
static TextureID loadTextureCGLTFimage(const cgltf_image* const image) {
    return TexturesHandler_BeginLoadingTask(image->name, image->uri);
}
static const void* viewAccessor(cgltf_accessor* const accessor) {
    return cgltf_buffer_view_data(accessor->buffer_view) + accessor->offset;
}
static void handleCGLTFError(const cgltf_result errorCode, const char fileName[const]) {
    const char* const errorStrings[] = {
	"This mesh file's data is too short",
	"This mesh file has unknown format",
	"This mesh file has invalid json",
	"This gltf file is invalid",
	"This mesh file has invalid options",
	"No mesh file found with this name",
	"CGLTF: IO error",
	"CGLTF: out of memory",
	"This mesh file works only with legacy gltf readers"
    };

    throwFatal(
	fileName, errorCode > ARRAYSIZE(errorStrings) ? "CGLTF: unknown error" : errorStrings[errorCode - 1]
    );
}
static cgltf_data* initData(const char fileName[const]) {
    const char firstStr[] = "mesh\\";

    const PathStringSize relPathStrSize = sizeof(firstStr) + strlen(fileName);

    cgltf_data* data;

    char relPathStr[relPathStrSize], absPathStr[PH_GetAbsolutePathStrSize(relPathStrSize)];

    strcpy(relPathStr, firstStr);
    strcat(relPathStr, fileName);
    PH_GetAbsolutePathStr(absPathStr, relPathStr);

    cgltf_result result = cgltf_parse_file(&(cgltf_options){0}, absPathStr, &data);
    if (result != cgltf_result_success) {
	handleCGLTFError(result, fileName);
    }

    result = cgltf_load_buffers(&(cgltf_options){0}, data, absPathStr);
    if (result != cgltf_result_success) {
	handleCGLTFError(result, fileName);
    }

    if (!data->meshes_count) throwFatal(fileName, "Meshes not found in this file");

    return data;
}
static void countIndicesVerticesOfPrimitive(
    const cgltf_primitive* const primitive, RegionSize* const restrict numIndices, RegionSize* const restrict numVertices
) {
    *numIndices += primitive->indices->count;
    *numVertices += primitive->attributes[0].data->count;
}
static void offsetIndices(Index3D indices[const], const GLuint startIndex, const GLuint count, const Index3D offset) {
    for (GLuint i = startIndex; i < startIndex + count; i++) indices[i] += offset;
}
static void handlePositionAttribute(
    float vertices[const], AABB bounding, const cgltf_attribute* const attribute, const size_t numFloatsVertex
) {
    for (cgltf_size i = 0; i < attribute->data->count; i++) {
	const cgltf_size di = i * VERTEX_POSITIONS_SIZE;

	const float* const position = (const float*)viewAccessor(attribute->data) + di;

	//bounding box
	if (bounding) {
	    glm_vec3_minv(bounding[0], (float*)position, bounding[0]);
	    glm_vec3_maxv(bounding[1], (float*)position, bounding[1]);
	}

	memcpy(
	    vertices + (i * numFloatsVertex) + VERTEX_POSITIONS_OFFSET, 
	    position, VERTEX_POSITIONS_SIZE * sizeof(*vertices)
	);
    }
}
static void workerThrd(TaskLoadModel* const task) {
    cgltf_data* const data = initData(task->baseInfo.fileName);

    task->data = data;
}
static void handleAttribute(
    float vertices[const], AABB bounding, const cgltf_attribute* const attribute, 
    const size_t numFloatsVertex, const float texid
) {
    const float* const data = viewAccessor(attribute->data);

    switch (attribute->type) {
    case cgltf_attribute_type_position:
	handlePositionAttribute(vertices, bounding, attribute, numFloatsVertex);

	break;
    case cgltf_attribute_type_texcoord:
	for (cgltf_size i = 0; i < attribute->data->count; i++) {
	    const cgltf_size di = i * VERTEX_TEXCOORDS_SIZE;
	    const size_t off = (i * numFloatsVertex) + VERTEX_TEXCOORDS_OFFSET;

	    vertices[off] = data[di] + texid;
	    vertices[off + 1] = data[di + 1];
	}

	break;
    case cgltf_attribute_type_joints:
	for (cgltf_size i = 0; i < attribute->data->count; i++) {
	    const cgltf_size di = i * VERTEX_WEIGHTS_SIZE;
	    const size_t off = (i * numFloatsVertex) + VERTEX_WEIGHTS_OFFSET;

	    for (size_t j = 0; j < 4; j++) {
		uint32_t boneId;

		switch (attribute->data->stride) {
		case 2*2:
		    boneId = ((uint8_t*)data)[di + j];
		    break;
		case 2*2*2:
		    boneId = ((uint16_t*)data)[di + j];
		    break;
		case 2*2*2*2:
		    boneId = ((uint32_t*)data)[di + j];
		    break;
		default:
		    throwFatal("CGLTF error occurred!", "This mesh has unsupported buffer stride!");
		    return;
		}

		vertices[off + j] += (float)boneId;
	    }
	}

	break;
    case cgltf_attribute_type_weights:
	for (cgltf_size i = 0; i < attribute->data->count; i++) {
	    const cgltf_size di = i * VERTEX_WEIGHTS_SIZE;
	    const size_t off = (i * numFloatsVertex) + VERTEX_WEIGHTS_OFFSET;

	    //subtract epsilon from weights so shaders can properly identify the corresponding bone

	    for (size_t j = 0; j < 4; j++) {
		const float weight = data[di + j];

		vertices[off + j] += weight;

		if (weight >= 1) vertices[off + j] = nextafterf(vertices[off + j], -INFINITY);

		//vertices[off + j] = 0;

		/*
		if (weight == 1) {
		    puts("its one");
		    //vertices[off + j] = nextafterf(vertices[off + j], 0);
		    vertices[off + j] -= .0001f;
		}
		if (weight == 0) {
		    puts("its zero");
		    vertices[off + j] += .0001f;
		}*/
	    }
/*
	    vertices[off] = .75f;
	    vertices[off + 1] = 1.25f;
	    */
	}

	break;
    default:
	if (true) {}
    }
}
static void loadRig(const cgltf_data* const data, Mesh* const mesh) {
    if (data->skins_count) {
	const cgltf_skin* const skin = data->skins + 0;

	printfd("Number of joints: %llu\n", skin->joints_count);

	//mesh->numBones = skin->joints_count;

	if (skin->joints_count) {
	    //assign ids
	    for (cgltf_size i = 0; i < skin->joints_count; i++) {
		//we could just assign the value to this pointer, but in cgltf_free function the library frees this pointer
		skin->joints[i]->extras.data = mallocd(sizeof(BoneID));

		*(BoneID*)skin->joints[i]->extras.data = i;
	    }

	    mesh->numBones = 1;

	    mallocarr(mesh->bones, skin->joints_count);

	    Bone_Init(
		mesh->bones + 0, mesh->bones, skin->joints[0], 
		&mesh->numBones, (mat4*)viewAccessor(skin->inverse_bind_matrices)
	    );

	    mesh->numAnimations = data->animations_count;

	    printfd("Number of animations: %llu\n", gltf->data->animations_count);

	    mallocarr(mesh->animations, data->animations_count);

	    for (cgltf_size i = 0; i < data->animations_count; i++) {
		Animation_Init(mesh->animations + i, data->animations + i, skin->joints_count);
	    }
	}
    }
}
static void loadMesh(const TaskLoadModel* const task) {
    RegionSize numIndices, numVertices;

    Mesh* const mesh = task->baseInfo.mesh;

    numIndices = numVertices = mesh->numUsedTextures = 0;

    const cgltf_mesh* const cmesh = task->data->meshes + 0;
    const ModelLoadInfo* const info = &task->baseInfo;
    
    foreach (const cgltf_primitive* const primitive, cmesh->primitives, cmesh->primitives_count)
	countIndicesVerticesOfPrimitive(primitive, &numIndices, &numVertices);

	mesh->numUsedTextures += isMaterialWithTexture(primitive->material);
    forend

    if (!numIndices) throwFatal(info->fileName, "This mesh doesn't contain any indices");
    if (!numVertices) throwFatal(info->fileName, "This mesh doesn't contain any vertices");

    printfd(
	"Number of indices: %u ; of vertices: %u ; of primitives: %llu\n", numIndices, numVertices, cmesh->primitives_count
    );

    Index3D* const indices = mallocd(numIndices * sizeof(*indices));
    float* const vertices = callocd(numVertices, R_GetVertexSizeByPipelineId(info->pipId));

    if (mesh->numUsedTextures) mallocarr(mesh->usedTextures, mesh->numUsedTextures);
    else mesh->usedTextures = NULL;

    numIndices = numVertices = mesh->numUsedTextures = 0;

    for (cgltf_size i = 0; i < cmesh->primitives_count; i++) {
	const cgltf_primitive* const primitive = cmesh->primitives + i;

	const TextureID texid = isMaterialWithTexture(primitive->material) ? loadTextureCGLTFimage(
	    primitive->material->pbr_metallic_roughness.base_color_texture.texture->image
	) : 0;

	//if (primitive->material) printfd("TEX ID: %u\n", texid);
	if (isMaterialWithTexture(primitive->material)) mesh->usedTextures[mesh->numUsedTextures++] = texid;

	cgltf_accessor_unpack_indices(primitive->indices, indices + numIndices, sizeof(Index3D), MAXT(cgltf_size));

	if (i) offsetIndices(indices, numIndices, primitive->indices->count, numVertices);

	for (cgltf_size j = 0; j < primitive->attributes_count; j++) {
	    const size_t numFloatsVertex = R_GetVertexSizeByPipelineId(info->pipId) / sizeof(float);

	    if (
		(primitive->attributes[j].type != cgltf_attribute_type_weights
		&& primitive->attributes[j].type != cgltf_attribute_type_joints)
		|| info->pipId == GRAPHICS_PIPELINE_SKINNED) {
		//pointer math is beautiful
		handleAttribute(
		    vertices + (numVertices * numFloatsVertex), NULL, 
		    primitive->attributes + j, numFloatsVertex, texid
		);
	    }
	}

	countIndicesVerticesOfPrimitive(primitive, &numIndices, &numVertices);
    }

    R_UploadIndices(&mesh->indicesRegion, numIndices, indices);
    R_UploadVertices(
	info->pipId, 
	(UploadVerticesInfo){.count = numVertices, .outRegion = &mesh->verticesRegion, .data = vertices}
    );

    free(indices);
    free(vertices);
}

void ModelsHandler_Init() {
    TaskHandleArray_Init(&tasks);
}
void ModelsHandler_LoadGeometry(const char fileName[const], ModelGeometry* const outGeometry) {
    cgltf_data* const data = initData(fileName);

    const cgltf_mesh* const mesh = data->meshes + 0;

    outGeometry->numIndices = outGeometry->numVertices = 0;

    foreach (const cgltf_primitive* const primitive, mesh->primitives, mesh->primitives_count)
	countIndicesVerticesOfPrimitive(primitive, &outGeometry->numIndices, &outGeometry->numVertices);
    forend

    mallocarr(outGeometry->indices, outGeometry->numIndices);
    mallocarr(outGeometry->vertices, outGeometry->numVertices);

    outGeometry->numIndices = outGeometry->numVertices = 0;

    for (cgltf_size i = 0; i < mesh->primitives_count; i++) {
	const cgltf_primitive* const primitive = mesh->primitives + i;

	cgltf_accessor_unpack_indices(
	    primitive->indices, outGeometry->indices + outGeometry->numIndices, sizeof(Index3D), MAXT(cgltf_size)
	);

	if (i) {
	    offsetIndices(outGeometry->indices, outGeometry->numIndices, primitive->indices->count, outGeometry->numVertices);
	}

	for (cgltf_size j = 0; j < primitive->attributes_count; j++) {
	    if (primitive->attributes[i].type == cgltf_attribute_type_position) {
		handlePositionAttribute(outGeometry->vertices[0], NULL, primitive->attributes + i, 3);

		break;
	    }
	}

	countIndicesVerticesOfPrimitive(primitive, &outGeometry->numIndices, &outGeometry->numVertices);
    }

    cgltf_free(data);
}
Mesh** ModelsHandler_BeginLoadingTask(const ModelLoadInfo* const info) {
    /*
    aforeach (TaskLoadModel* const task, &tasks)
	if (!strcmp(task->path, info->fileName)) {

	}
    forend
    */

    TaskLoadModel* const task = mallocd(sizeof(*task));

    TaskHandleArray_AppendElement(&tasks, &task);

    task->data = NULL;
    task->baseInfo = *info;

    TM_AddTask(&(Task){.function = (void*)workerThrd, .argument = task});

    return &task->baseInfo.mesh;
}
void ModelsHandler_Loop() {
    //printf("%llu\n", tasks.numElements);

    for (ArrayIndex i = 0; i < tasks.numElements; i++) {
	TaskLoadModel* const task = tasks.elements[i];

	if (task->data) {
	    if (task->baseInfo.mesh) {
		loadRig(task->data, task->baseInfo.mesh);
		loadMesh(task);

		R_UploadMesh(task->baseInfo.pipId, (UploadMeshInfo){
		    .meshId = task->baseInfo.meshId,
		    .firstIndex = task->baseInfo.mesh->indicesRegion.position,
		    .firstVertex = task->baseInfo.mesh->verticesRegion.position,
		    //for explanation visit this struct's declaration line
		    .numIndices = task->baseInfo.mesh->numInstances ? task->baseInfo.mesh->indicesRegion.size : 0
		});
	    }

	    cgltf_free(task->data);

	    free(task->baseInfo.fileName);
	    free(task);

	    TaskHandleArray_RemoveElement(&tasks, i);
	}
    }
}
