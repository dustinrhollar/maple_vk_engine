
struct PS_IN
{
	float4 Position : SV_POSITION; // interpolated vertex value
	float4 Color    : COLOR;       // interpolated diffuse color
	float2 Tex0     : TEXCOORD;
};

Texture2D DiffuseTexture : register(t0);
SamplerState SampleType;

float4 main(PS_IN Input) : SV_TARGET
{
	return DiffuseTexture.Sample(SampleType, Input.Tex0);
	//return float4(Input.Tex0, 0.0f, 1.0f);
    //return Input.Color;
}