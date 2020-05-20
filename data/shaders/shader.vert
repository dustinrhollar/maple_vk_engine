#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normals;
layout (location = 2) in vec4 color;
layout (location = 3) in vec2 uv0;

struct MVP {
    mat4 View;
    mat4 Projection;
	mat4 Model;
};

layout (binding = 0, set = 0) uniform MvpBuffer {
    MVP Mvp;
};

layout(binding = 0, set = 1) uniform sampler2D heightmap;

void main()
{
	gl_Position = Mvp.Projection * Mvp.View * Mvp.Model * vec4(position, 1.0f);
}