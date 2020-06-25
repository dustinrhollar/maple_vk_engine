
struct PS_IN
{
	float4 Position : SV_POSITION; // interpolated vertex value
	float4 Color    : COLOR;       // interpolated diffuse color
};

float4 main(PS_IN Input) : SV_TARGET
{
    return Input.Color;
}