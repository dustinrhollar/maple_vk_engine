
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

cbuffer cbPerObject
{
    float4x4 Projection;
    float4x4 View;
    float4x4 Model;
};

struct VS_OUT
{
    float4 Position : SV_POSITION;
    float4 Color    : COLOR;
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
	
#if 1
	float4 TmpPos = float4(Input.Position.x, 0.0f, Input.Position.y, 1.0f);
#else
	float4 TmpPos = float4(Input.Position.x, Input.Position.y, 0.5f, 1.0f);
#endif

	Output.Position = mul(Projection, mul(View, mul(Model, TmpPos)));
	//Output.Position = mul(TmpPos, mul(Model, mul(View, Projection)));
	//Output.Position = mul(Model, TmpPos);
	//Output.Position = TmpPos;
	
	Output.Tex0  = Input.Tex0;
	Output.Color = float4(0.0f, 1.0f, 0.0f, 1.0f);
	
	return Output;
}
