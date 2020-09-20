#version 450
#extension GL_ARB_separate_shader_objects : enable

#if 1
layout(location = 0) in FS_IN {
	vec3 FragColor;
	vec3 Normal;
} fs_in;
#else
layout(location = 0) in vec3 FragColor;
#endif

layout(location = 0) out vec4 FinalColor;

struct directional_light
{
	vec3 Direction;
	vec3 Ambient, Diffuse, Specular;
};

const directional_light Light = {
	{ 0.0f, -1.0f, 0.0f },
	{ 1.0f, 1.0f, 1.0f },
	{ 1.0f, 1.0f, 1.0f },
	{ 1.0f, 1.0f, 1.0f }
};

void main() 
{	
	vec3 Ambient = Light.Ambient * fs_in.FragColor;
	
	// Diffuse
	vec3 Normal = normalize(fs_in.Normal);
	vec3 LightDir = -Light.Direction;
	float Diff = max(0.0f, dot(Normal, LightDir));
	vec3 Diffuse = Light.Diffuse * Diff * fs_in.FragColor;

	// Specular
	

    FinalColor = vec4(0.4f * Ambient + Diffuse, 1.0);
    //FinalColor = vec4(fs_in.FragColor * 0.8, 1.0);
}
