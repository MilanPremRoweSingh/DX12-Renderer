#pragma once

enum VertexBufferID;
enum IndexBufferID;
struct Vertex;

// For now, a renderable is just a mesh. Later it will have material information and whatever else information is required to draw it
class Renderable
{
public:
    Renderable(VertexBufferID _vbid, IndexBufferID _ibid) :
        vbid(_vbid),
        ibid(_ibid) {};

    static Renderable* Create(
        size_t numVerts,
        Vertex* pVerts,
        size_t numIndices,
        uint32* pIndices);

    static void Destroy(
        Renderable* renderable);

    VertexBufferID vbid;
    IndexBufferID ibid;
private:
};