#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (push_constant) uniform push_constants
{
	vec3 Min;
	vec3 Max;
} Bounds;

layout (points) in;
layout (line_strip, max_vertices = 5) out;

void main()
{
	vec2 Min = Bounds.Min.xy;
	vec2 Max = Bounds.Max.xy;
	
	gl_Position = vec4(-0.5, 0.5, 0.0f, 1.0f);
	EmitVertex();
	
	gl_Position = vec4(0.5, 0.5, 0.0f, 1.0f);
	EmitVertex();
	
	gl_Position = vec4(0.5, -0.5, 0.0f, 1.0f);
	EmitVertex();
	
	gl_Position = vec4(-0.5, -0.5, 0.0f, 1.0f);
	EmitVertex();
	
	gl_Position = vec4(-0.5, 0.5, 0.0f, 1.0f);
	EmitVertex();
	
	EndPrimitive();
}