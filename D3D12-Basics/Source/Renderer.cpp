#include "Renderer.h"

#include "Engine.h"
#include "Camera.h"
#include "Shell.h"
#include "ConstantBuffers.h"
#include "D3D12Core.h"

size_t g_cbSizes[CBCount] = {
   sizeof(CBStatic),
};

struct RenderContext
{    
    ConstantData* constantData[CBCount];
    uint32 dirtyCBFlags;
};

void Renderer::ConstantDataInitialise()
{
    int32 id = CBStart;
    m_context->constantData[id++] = new CBStatic();

    ASSERT(id == CBCount);

    m_context->dirtyCBFlags = 0;
}

void Renderer::ConstantDataDispose()
{
    for (int32 id = CBStart; id < CBCount; id++)
    {
        delete m_context->constantData[id];
    }
}

void Renderer::ConstantDataSetEntry(
    const ConstantDataEntry& entry,
    void* data)
{
    Utils::SetBit32(entry.id, m_context->dirtyCBFlags);
    memcpy(m_context->constantData[entry.id] + entry.offset, data, entry.size);
}

void Renderer::ConstantDataFlush(
    void)
{
    if (!m_context->dirtyCBFlags)
    {
        return;
    }

    for (int32 id = CBStart; id < CBCount; id++)
    {
        if (Utils::TestBit32(id, m_context->dirtyCBFlags))
        {
            m_core->ConstantBufferSetData((ConstantBufferID)id, g_cbSizes[id], m_context->constantData[id]);
        }
    }
}

Renderer::Renderer()
{
    m_core = new D3D12Core();
    m_context = new RenderContext();

    ConstantDataInitialise();
}

Renderer::~Renderer()
{
    ConstantDataDispose();

    delete m_context;
    delete m_core;
}

void Renderer::Render()
{
    // This is terrible, but fine for now
    float radius = 10.0f;
    float time = GetCurrentFrameTime();
    Vector3 eyePos(radius * cosf(time), 0.0f, radius * sinf(time));
    Vector3 targetPos;
    Vector3 camUp(0.0f, 1.0f, 0.0f);
    camUp.Normalize();
    Camera cam(eyePos, targetPos, camUp, 0.1f, 100.0f, 90.0f, GetWindowAspectRatio());

    Matrix4x4 matView;
    cam.GetViewMatrix(matView);
    matView.Transpose(matView);
    ConstantDataSetEntry(CBSTATIC_ENTRY(matView), &matView);

    Matrix4x4 matProj;
    cam.GetProjMatrix(matProj);
    matProj.Transpose(matProj);
    ConstantDataSetEntry(CBSTATIC_ENTRY(matProj), &matProj);

    Vector3 directionalLight(1, 1, -1);
    directionalLight.Normalize();
    ConstantDataSetEntry(CBSTATIC_ENTRY(directionalLight), &directionalLight);

    float diffuse = 0.5f;
    ConstantDataSetEntry(CBSTATIC_ENTRY(diffuse), &diffuse);

    float specular = 0.5f;
    ConstantDataSetEntry(CBSTATIC_ENTRY(specular), &specular);

    float specularHardness = 10.0f;
    ConstantDataSetEntry(CBSTATIC_ENTRY(specularHardness), &specularHardness);

    ConstantDataFlush();

    m_core->Draw();
    m_core->Present();
}