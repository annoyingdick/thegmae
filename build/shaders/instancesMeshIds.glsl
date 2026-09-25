#define DECLARE_IMI(x) layout (binding = $BUFFER_BINDING_INSTANCES_MESH_IDS$, std430) x buffer instancesMeshIdsBuffer { \
    uint instancesMeshIds[]; }
