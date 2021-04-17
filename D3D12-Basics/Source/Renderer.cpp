#include "Renderer.h"

#include "Camera.h"
#include "ConstantBuffers.h"
#include "D3D12Core.h"
#include "Renderable.h"
#include "Scene.h"

size_t g_cbSizes[CBCount] = {
   sizeof(CBStatic),
};

struct RenderContext
{    
    const Camera* pCamera;
    const Scene* pScene;

    ConstantData* pConstantData[CBCount];
    uint32 dirtyCBFlags;
};

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

void Renderer::ConstantDataInitialise()
{
    int32 id = CBStart;
    m_context->pConstantData[id++] = new CBStatic();

    ASSERT(id == CBCount);

    m_context->dirtyCBFlags = 0;
}

void Renderer::ConstantDataDispose()
{
    for (int32 id = CBStart; id < CBCount; id++)
    {
        delete m_context->pConstantData[id];
    }
}

void Renderer::ConstantDataSetEntry(
    const ConstantDataEntry& entry,
    void* data)
{
    Utils::SetBit32(entry.id, m_context->dirtyCBFlags);
    memcpy(m_context->pConstantData[entry.id] + entry.offset, data, entry.size);
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
            m_core->ConstantBufferSetData((ConstantBufferID)id, g_cbSizes[id], m_context->pConstantData[id]);
        }
    }
}

void Renderer::CameraSet(
    const Camera* pCamera)
{
    m_context->pCamera = pCamera;
}

void Renderer::SceneSet(
    const Scene* pScene)
{
    m_context->pScene = pScene;
}

VertexBufferID Renderer::VertexBufferCreate(
    size_t numVerts, 
    Vertex* pVertexData)
{
    return m_core->VertexBufferCreate(numVerts, pVertexData);
}

void Renderer::VertexBufferDestroy(
    VertexBufferID vbid)
{
    m_core->VertexBufferDestroy(vbid);
}

IndexBufferID Renderer::IndexBufferCreate(size_t numIndices, uint32* pIndexData)
{
    return m_core->IndexBufferCreate(numIndices, pIndexData);
}

void Renderer::IndexBufferDestroy(IndexBufferID ibid)
{
    m_core->IndexBufferDestroy(ibid);
}

void Renderer::Render()
{
    if (!m_context->pCamera)
    {
        return;
    }

    Matrix4x4 matView;
    m_context->pCamera->GetViewMatrix(matView);
    matView.Transpose(matView);
    ConstantDataSetEntry(CBSTATIC_ENTRY(matView), &matView);

    Matrix4x4 matProj;
    m_context->pCamera->GetProjMatrix(matProj);
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

    m_core->Begin();

    if (m_context->pScene)
    {
        for (int32 i = 0; i < m_context->pScene->pRenderables.size(); i++)
        {
            const Renderable* pRenderable = m_context->pScene->pRenderables[i];
            m_core->Draw(pRenderable->vbid, pRenderable->ibid);
        }
    }

    m_core->End();

    m_core->Present();
}