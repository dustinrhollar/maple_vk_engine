struct VS_IN
{
	float4 Position : POSITION;
	float4 Color    : COLOR;
};

struct VS_OUT
{
    float4 Position : SV_POSITION;
    float4 Color    : COLOR;
};

VS_OUT main(VS_IN Input)
{
    VS_OUT Output;

    Output.Position = Input.Position;
    Output.Color = Input.Color;

    return Output;
}


