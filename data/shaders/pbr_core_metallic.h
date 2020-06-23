
//layout (binding = 0, set=2) uniform sampler2D BaseColorTexture;
//layout (binding = 1, set=2) uniform sampler2D MetallicRoughnessTexture;
//layout (binding = 2, set=2) uniform sampler2D NormalTexture;
//layout (binding = 3, set=2) uniform sampler2D OcclusionTexture;
//layout (binding = 4, set=2) uniform sampler2D EmissiveTexture;

layout (location = 0) in vec3 InColor;
layout (location = 1) in vec3 InNormal;
layout (location = 2) in vec2 InTex;

layout (location = 0) out vec4 FragColor;

struct default_pbr_metallic_textures
{
    int BaseColorIdx;
    int MetallicRoughnessIdx;
    int NormalIndex;
    int OcclusionIdx;
    int EmissiveIdx;
};

layout (binding = 0, set = 2) uniform sampler2D Textures;
