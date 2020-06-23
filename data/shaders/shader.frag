#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "pbr_core_metallic.h"

layout (push_constant) uniform push_constants
{
    default_pbr_metallic_textures Textures;
    
    // Push constant blocks get 128 bytes so
    // pad out the remaining 108 byes
    int Padding[27];
} PushConstants;


void main()
{
	FragColor = texture(BaseColorTexture, InTex);
}