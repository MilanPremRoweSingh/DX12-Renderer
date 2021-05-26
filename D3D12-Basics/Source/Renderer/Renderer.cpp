#include "Renderer.h"

#include "Camera.h"
#include "Scene.h"

#include "Renderer/ConstantBuffers.h"
#include "Renderer/Renderable.h"
#include "Renderer/Texture.h"
#include "Renderer/Core/D3D12Core.h"

size_t g_cbSizes[CBIDCount] = {
   sizeof(CBCommon),
   sizeof(CBStatic),
};

struct RenderContext
{    
    const Camera* pCamera;
    const Scene* pScene;

    ConstantData* pConstantData[CBIDCount];
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
    int32 id = CBIDStart;

    m_context->pConstantData[CBIDStatic] = new CBStatic();
    m_context->pConstantData[CBIDCommon] = new CBCommon();

    m_context->dirtyCBFlags = 0;
}

void Renderer::ConstantDataDispose()
{
    for (int32 id = CBIDStart; id < CBIDCount; id++)
    {
        delete m_context->pConstantData[id];
    }
}

void Renderer::ConstantDataSetEntry(
    const ConstantDataEntry& entry,
    const void* data)
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

    for (int32 id = CBIDStart; id < CBIDCount; id++)
    {
        if (Utils::TestBit32(id, m_context->dirtyCBFlags))
        {
            m_core->ConstantBufferSetData((ConstantBufferID)id, g_cbSizes[id], m_context->pConstantData[id]);
        }
    }

    m_context->dirtyCBFlags = 0;
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

TextureID Renderer::TextureCreate(
    int32 width,
    int32 height,
    int32 numChannels,
    void* pData)
{
    return m_core->TextureCreate(width, height, numChannels, pData);
}

void Renderer::TextureDestroy(
    TextureID tid)
{
    m_core->TextureDestroy(tid);
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
    ConstantDataFlush();

    m_core->Begin();

    if (m_context->pScene)
    {
        for (int32 i = 0; i < m_context->pScene->m_pRenderables.size(); i++)
        {
            const Renderable* pRenderable = m_context->pScene->m_pRenderables[i];

            float specular = 0.5f;
            ConstantDataSetEntry(CBCOMMON_ENTRY(specular), &specular);

            float specularHardness = 10.0f;
            ConstantDataSetEntry(CBCOMMON_ENTRY(specularHardness), &specularHardness);

            const Material& material = pRenderable->material;

            ConstantDataSetEntry(CBCOMMON_ENTRY(diffuse), &material.diffuse);
            
            ConstantDataFlush();

            if (material.diffuseTexture)
            {
                m_core->TextureBindForDraw(material.diffuseTexture->GetID(), 0);
            }

            m_core->Draw(pRenderable->vbid, pRenderable->ibid);
        }
    }

    m_core->End();

    m_core->Present();

}

void Renderer::FlushGPU()
{
    m_core->WaitForGPU();
}