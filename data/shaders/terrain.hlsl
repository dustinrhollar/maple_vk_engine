
#if 0

// TODO
struct MVP {
    mat4 View;
    mat4 Projection;
	mat4 Model;
};

layout (binding = 0, set = 0) uniform MvpBuffer {
    MVP Mvp;
};

layout(binding = 0, set = 1) uniform sampler2D heightmap;

#endif

struct VS_IN
{
	float2 Position : POSITION;
	float3 Normal   : NORMAL;
	float2 Tex0     : TEXCOORD;
};

struct VS_OUT
{
    float4 Position : SV_POSITION;
    float2 Tex0     : TEXCOORD;
};

//Texture2D    HeightmapTexture : register(t0);
//SamplerState SampleHeightmap

VS_OUT main(VS_IN Input)
{
	//vec3 hpos = vec3(5 * position.x, 5 * texture(heightmap, uv0).r * 60, 5 * position.y);
	//vec3 hpos = vec3(position.x, 0.0f, position.y);
	//gl_Position = Mvp.Projection * Mvp.View * Mvp.Model * vec4(hpos, 1.0f);
	
	VS_OUT Output;
	Output.Position = float4(Input.Position, 0.0f, 0.0f);
	Output.Tex0 = Input.Tex0;

	return Output;
}
