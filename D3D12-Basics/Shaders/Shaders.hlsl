
cbuffer Constants0
{
	float4x4 mat;
};

SamplerState Sampler;
Texture2D Texture;


struct VS_COL_IN
{
	float3 pos : POSITION;
	float4 col : COLOR0;
};

struct VS_COL_OUT
{
	float4 pos : SV_POSITION;
	float4 col : COLOR0;
};

struct PS_OUT
{
	float4 col : SV_TARGET;
};

VS_COL_OUT HelloTriangleVS(VS_COL_IN I)
{
	VS_COL_OUT O;
	O.col = I.col;
	O.pos = mul(mat, float4(I.pos, 1.0f));
	return O;
}

PS_OUT HelloTrianglePS(VS_COL_OUT I)
{
	PS_OUT O;
	O.col = I.col * Texture.Sample(Sampler, float2(0.5f, 0.5f));
	return O;
}

struct VS_IN
{
	float3 pos : POSITION;
};

struct VS_OUT
{
	float4 pos : SV_POSITION;
};

VS_OUT VSMain(VS_IN I)
{
	VS_OUT O;
	O.pos = mul(mat, float4(I.pos, 1.0f));
	return O;
}

PS_OUT PSMain(VS_OUT I)
{
	PS_OUT O;
	O.col = float4(1.0f,1.0f,1.0f,1.0f);
	return O;
}

