#include "Lighting.h"

cbuffer Constants0 : register(b0)
{
	float4x4 matMVP;
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
	O.pos = mul(matMVP, float4(I.pos + float3(0,0,0), 1.0f));
	O.normal = mul(matMVP, float4(I.normal, 0.0f)).xyz;
	return O;
}

PS_OUT PSMain(VS_OUT I)
{
	PS_OUT O;

	float3 n = normalize(I.normal);
	float3 l = normalize(float3(1, 1, 0));

	O.col = I.col * Lambertian(n, l);
	return O;
}
