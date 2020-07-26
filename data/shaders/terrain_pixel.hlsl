
struct PS_IN
{
	float4 Position : SV_POSITION; // interpolated vertex value
	float4 Color    : COLOR;       // interpolated diffuse color
	float2 Tex0     : TEXCOORD;
};


float4 main(PS_IN Input) : SV_TARGET
{
    return float4(1.0f, 0.0f, 0.0f, 1.0f);
}