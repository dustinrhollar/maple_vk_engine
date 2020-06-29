struct VS_IN
{
	float3 Position : POSITION;
	float4 Color    : COLOR;
	float2 Tex0     : TEXCOORD;
};

struct VS_OUT
{
    float4 Position : SV_POSITION;
    float4 Color    : COLOR;
	float2 Tex0     : TEXCOORD;
};

VS_OUT main(VS_IN Input)
{
    VS_OUT Output;

    Output.Position = float4(Input.Position, 1.0f);
    //Output.Position = Input.Position;
    Output.Color = Input.Color;
	Output.Tex0 = Input.Tex0;

    return Output;
}


