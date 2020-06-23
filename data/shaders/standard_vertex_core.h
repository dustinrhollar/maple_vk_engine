layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normals;
layout (location = 2) in vec4 color;
layout (location = 3) in vec2 uv0;

layout (location = 0) out vec3 OutColor;
layout (location = 1) out vec3 OutNormal;
layout (location = 2) out vec2 OutTex;

struct global_data {
    mat4 View;
    mat4 Projection;
};

struct object_data {
    mat4 Model;
};

layout (binding = 0, set = 0) uniform global_data_buffer {
    float TestFloat;
    vec3 TestVec3;
    mat4 View;
    mat4 Projection;
};

layout (binding = 0, set = 1) uniform object_buffer {
    object_data ObjectData;
};