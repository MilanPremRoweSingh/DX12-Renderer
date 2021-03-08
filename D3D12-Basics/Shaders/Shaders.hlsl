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
	float4 col : SV_TARGET0;
};

VS_OUT HelloTriangleVS(VS_IN I)
{
	VS_OUT O;
	O.col = I.col;
	O.pos = float4(I.pos, 1.0f);
	return O;
}

PS_OUT HelloTrianglePS(VS_OUT I)
{
	PS_OUT O;
	O.col = I.col;	
	return O;
}