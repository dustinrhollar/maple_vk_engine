#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 0, set=2) uniform sampler2D BaseColorTexture;
layout (binding = 1, set=2) uniform sampler2D MetallicRoughnessTexture;
layout (binding = 2, set=2) uniform sampler2D NormalTexture;
layout (binding = 3, set=2) uniform sampler2D OcclusionTexture;
layout (binding = 4, set=2) uniform sampler2D EmissiveTexture;

layout (location = 0) in vec3 OutColor;
layout (location = 1) in vec3 OutNormal;
layout (location = 2) in vec2 OutTex;

layout (location = 0) out vec4 FragColor;


void main()
{
	// FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
	FragColor = texture(BaseColorTexture, OutTex);
}