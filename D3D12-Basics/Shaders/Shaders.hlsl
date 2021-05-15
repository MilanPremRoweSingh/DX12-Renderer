#include "ConstantBuffers.h"
#include "Lighting.h"

SamplerState Sampler;
Texture2D Texture;


struct VS_IN
{
	float3 pos : POSITION;
	float4 col : COLOR0;
	float3 normal : NORMAL;
	float2 uv : UV;
};

struct VS_OUT
{
	float4 hpos : SV_POSITION;
	float4 col : COLOR0;
	float3 normal : NORMAL;
	float4 viewPos : TEXCOORD0;
	float2 uv : TEXCOORD1;
};

struct PS_OUT
{
	float4 col : SV_TARGET;
};

VS_OUT VSMain(VS_IN I)
{
	VS_OUT O;
	O.col = I.col;
	O.viewPos = mul(matView, float4(I.pos, 1.0f));
	O.hpos = mul(matProj, O.viewPos);
	O.normal = mul(matView, float4(I.normal, 0.0f)).xyz;
	O.uv = I.uv;
	return O;
}

PS_OUT PSMain(VS_OUT I)
{
	PS_OUT O;

	float3 v = normalize(-I.viewPos.xyz);
	float3 n = normalize(I.normal);
	float3 l = directionalLight;

	float3 albedoColour = Texture.Sample(Sampler, I.uv);

	O.col.xyz = albedoColour * diffuse * Lambertian(n, l) + specular * BlinnPhongSpecular(l, v, n, specularHardness);
	return O;
}
