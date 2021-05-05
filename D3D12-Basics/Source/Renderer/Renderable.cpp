#include "Renderable.h"

#include "Engine.h"
#include "Renderer/Core/D3D12Core.h"

Renderable::~Renderable()
{
    ASSERT(vbid != VertexBufferIDInvalid);
    ASSERT(ibid != IndexBufferIDInvalid);

    g_pRenderer->VertexBufferDestroy(vbid);
    g_pRenderer->IndexBufferDestroy(ibid);
}