
cbuffer Constants0
{
	float4x4 mat;
};

SamplerState Sampler;
Texture2D Texture;

struct VS_IN
{
	float3 pos : POSITION;
	float4 col : COLOR0;
};

struct VS_OUT
{
	float4 pos : SV_POSITION;
	float4 col : COLOR0;
};

struct PS_OUT
{
	float4 col : SV_TARGET;
};

VS_OUT HelloTriangleVS(VS_IN I)
{
	VS_OUT O;
	O.col = I.col;
	O.pos = mul(mat, float4(I.pos, 1.0f));
	return O;
}

PS_OUT HelloTrianglePS(VS_OUT I)
{
	PS_OUT O;
	O.col = I.col * Texture.Sample(Sampler, float2(0.5f, 0.5f));
	return O;
}