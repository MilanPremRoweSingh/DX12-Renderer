#pragma once
#include <stddef.h> 
#include <unordered_map>

#define CBSTATIC_ENTRY(name) {sizeof((*((CBStatic*)0)).name), offsetof(CBStatic, name), CBStaticID }

enum ConstantBufferID : int32
{
    CBStart = 0,
    CBStaticID = CBStart,
    CBCount,
};

struct ConstantDataEntry
{
    size_t size;
    size_t offset;
    ConstantBufferID id;
};

struct ConstantData
{};

struct CBStatic : ConstantData
{
    Matrix4x4 matView;
    Matrix4x4 matProj;
    Vector3 directionalLight;
    float diffuse;
    float specular;
    float specularHardness;
};

extern size_t g_cbSizes[CBCount];