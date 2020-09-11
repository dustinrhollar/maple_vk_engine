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

#if 1
layout(location = 0) out VS_OUT {
	vec3 FragColor;
	vec3 Normal;
} vs_out;
#else

layout(location = 0) out vec3 FragColor;

#endif

const float ScaleY = 150.0f;

void main() {

#if 0

	float Height = texture(Heightmap, Uvs).r * ScaleY;   
	gl_Position = GlobalData.Projection * GlobalData.View * ObjectData.Model * vec4(Position.x, Height, Position.y, 1.0);

#else

	gl_Position = GlobalData.Projection * GlobalData.View * ObjectData.Model * vec4(Position, 1.0f);

#endif
	
	vs_out.Normal    = Normal;

	float Height = texture(Heightmap, Uvs).r;
	//vs_out.FragColor = vec3(Height, Height, Height);
	vs_out.FragColor = Color;
	//vs_out.FragColor = Normal;

}
