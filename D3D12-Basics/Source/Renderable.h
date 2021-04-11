#pragma once

enum VertexBufferID;
enum IndexBufferID;
struct Vertex;

// For now, a renderable is just a mesh. Later it will have material information and whatever else information is required to draw it
class Renderable
{
public:
    static Renderable* Create(
        size_t numVerts,
        Vertex* pVerts,
        size_t numIndices,
        uint32* pIndices);

    static void Destroy(
        Renderable* renderable);
    
private:
    Renderable(VertexBufferID vbid, IndexBufferID ibid) : 
        m_vbid(vbid),
        m_ibid(ibid) {};

    VertexBufferID m_vbid;
    IndexBufferID m_ibid;
};