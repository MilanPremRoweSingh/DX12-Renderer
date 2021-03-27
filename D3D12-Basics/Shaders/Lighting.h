static const float PI = 3.14159265f;

float Lambertian(float3 n, float3 l)
{
	return saturate(dot(n,l));	
}

float BlinnPhongSpecular(float3 l, float3 v, float3 n, float hardness)
{
	float3 h = normalize(v+l);
	float nDotH = saturate(dot(h,n));
	return pow(nDotH, hardness);
}