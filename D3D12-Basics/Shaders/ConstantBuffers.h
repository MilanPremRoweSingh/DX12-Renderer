
cbuffer CBStatic : register(b0)
{
    float4x4 matView;
    float4x4 matProj;
    float3 directionalLight;
};

cbuffer CBCommon : register(b1)
{
    float3 diffuse;
    float specular;
    float specularHardness;
};


