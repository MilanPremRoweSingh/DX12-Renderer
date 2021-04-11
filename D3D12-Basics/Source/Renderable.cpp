#include "Renderable.h"

#include "Engine.h"
#include "D3D12Core.h"

Renderable* Renderable::Create(
    size_t numVerts,
    Vertex* pVerts,
    size_t numIndices,
    uint32* pIndices)
{
    VertexBufferID vbid = g_pRenderer->VertexBufferCreate(numVerts, pVerts);
    if (vbid == VertexBufferIDInvalid)
    {
        return nullptr;
    }

    IndexBufferID ibid = g_pRenderer->IndexBufferCreate(numIndices, pIndices);
    if (vbid == IndexBufferIDInvalid)
    {
        return nullptr;
    }

    return new Renderable(vbid, ibid);
}

void Renderable::Destroy(
    Renderable* pRenderable)
{
    ASSERT(pRenderable);
    ASSERT(pRenderable->m_vbid != VertexBufferIDInvalid);
    ASSERT(pRenderable->m_ibid != IndexBufferIDInvalid);

    g_pRenderer->VertexBufferDestroy(pRenderable->m_vbid);
    g_pRenderer->IndexBufferDestroy(pRenderable->m_ibid);
}