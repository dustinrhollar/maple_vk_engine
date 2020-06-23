#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "standard_vertex_core.h"

void main()
{
	gl_Position = Projection * View * ObjectData.Model * vec4(position, 1.0);
	
    gl_Position *= TestFloat;
    gl_Position *= TestVec3[0];
    
    
	OutColor  = color.xyz;
	OutNormal = normals;
	OutTex    = uv0;
}