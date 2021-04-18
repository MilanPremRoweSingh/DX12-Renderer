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

Renderable::~Renderable()
{
    ASSERT(vbid != VertexBufferIDInvalid);
    ASSERT(ibid != IndexBufferIDInvalid);

    g_pRenderer->VertexBufferDestroy(vbid);
    g_pRenderer->IndexBufferDestroy(ibid);
}