
cbuffer CBCommon : register(b0)
{
    float3 diffuse;
    float specular;
    float specularHardness;
};

cbuffer CBStatic : register(b1)
{
    float4x4 matView;
    float4x4 matProj;
    float3 directionalLight;
};

