#version 450
#extension GL_ARB_separate_shader_objects : enable

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
    global_data GlobalData;
};

layout (binding = 0, set = 1) uniform object_buffer {
    object_data ObjectData;
};
void main()
{
	gl_Position = GlobalData.Projection * GlobalData.View * ObjectData.Model * vec4(position, 1.0);
	
	OutColor  = color.xyz;
	OutNormal = normals;
	OutTex    = uv0;
}