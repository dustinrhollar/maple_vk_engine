#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

struct global_data 
{
    mat4 View;
    mat4 Projection;
};

layout (binding = 0, set = 0) uniform global_data_buffer {
    global_data GlobalData;
};

layout(location = 0) in GS_IN {
	vec3 FragColor;
    vec3 Normal;
} gs_in[];

const float MAGNITUDE = 10.2;

void GenerateLine(int index)
{
	gl_Position = GlobalData.Projection * gl_in[index].gl_Position;
    EmitVertex();
    gl_Position = GlobalData.Projection * (gl_in[index].gl_Position + vec4(gs_in[index].Normal, 0.0) * MAGNITUDE);
    EmitVertex();
    EndPrimitive();
}

void main()
{
    GenerateLine(0); // first vertex normal
    GenerateLine(1); // second vertex normal
    GenerateLine(2); // third vertex normal
}