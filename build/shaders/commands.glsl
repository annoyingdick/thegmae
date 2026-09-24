struct DrawElementsIndirectCommand {
    uint count;
    uint instanceCount;
    uint firstIndex;
    int baseVertex;
    uint baseInstance;
};

#define DECLARE_COMMANDS(x) layout (binding = 2, std430) x buffer commandsBuffer { DrawElementsIndirectCommand commands[]; }
