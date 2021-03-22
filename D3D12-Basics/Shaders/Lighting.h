static const float PI = 3.14159265f;

float Lambertian(float3 n, float3 l)
{
	return dot(n,l);	
}