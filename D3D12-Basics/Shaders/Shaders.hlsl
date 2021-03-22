
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
	float3 normal : NORMAL;
};

struct VS_OUT
{
	float4 pos : SV_POSITION;
	float4 col : COLOR0;
	float3 normal : NORMAL;
};

struct PS_OUT
{
	float4 col : SV_TARGET;
};

VS_OUT VSMain(VS_IN I)
{
	VS_OUT O;
	O.col = I.col;
	O.pos = mul(mat, float4(I.pos + float3(0,0,0), 1.0f));
	O.normal = mul(mat, float4(I.normal, 1.0f));
	return O;
}

PS_OUT PSMain(VS_OUT I)
{
	PS_OUT O;
	float4 col = float4((I.normal+1.0f) * 0.5f, 1.0f);
	O.col = col;// * float4(1,1,1,1);//I.col * Texture.Sample(Sampler, float2(0.5f, 0.5f));
	return O;
}
