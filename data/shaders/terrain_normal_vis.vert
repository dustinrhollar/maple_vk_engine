#version 450
#extension GL_ARB_separate_shader_objects : enable

struct global_data 
{
    mat4 View;
    mat4 Projection;
};

struct object_data 
{
	mat4 Model;
};

layout (binding = 0, set = 0) uniform global_data_buffer {
    global_data GlobalData;
};

layout (binding = 0, set = 1) uniform object_data_buffer {
    object_data ObjectData;
};

layout(binding = 0, set = 2) uniform sampler2D Heightmap;

layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec2 Uvs;
layout(location = 3) in vec3 Color;

layout(location = 0) out VS_OUT {
	vec3 FragColor;
	vec3 Normal;
} vs_out;

const float ScaleY = 100.0f;

void main() {
	//float Height = texture(Heightmap, Uvs).r * ScaleY;
       
	gl_Position = GlobalData.View * ObjectData.Model * vec4(Position, 1.0);
	
	mat3 NormalMatrix = mat3(transpose(inverse(GlobalData.View * ObjectData.Model)));
	vs_out.Normal    = vec3(vec4(NormalMatrix * Normal, 0.0));;
	vs_out.FragColor = vs_out.Normal;
}
