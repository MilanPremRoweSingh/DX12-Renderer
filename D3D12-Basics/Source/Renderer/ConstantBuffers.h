#pragma once
#include <stddef.h> 
#include <unordered_map>

#define CBSTATIC_ENTRY(name) {sizeof((*((CBStatic*)0)).name), offsetof(CBStatic, name), CBIDStatic }
#define CBCOMMON_ENTRY(name) {sizeof((*((CBCommon*)0)).name), offsetof(CBCommon, name), CBIDCommon }

enum ConstantBufferID : int32
{
    CBIDStart = 0,
    CBIDStatic = CBIDStart,
    CBIDCommon,
    CBIDCount,
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
};

struct CBCommon : ConstantData
{
    Vector3 diffuse;
    float specular;
    float specularHardness;
};

extern size_t g_cbSizes[CBIDCount];