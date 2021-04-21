#pragma once

#include "Material.h"

enum VertexBufferID;
enum IndexBufferID;
struct Vertex;

// For now, a renderable is just a mesh. Later it will have material information and whatever else information is required to draw it
class Renderable
{
public:
    Renderable(VertexBufferID _vbid, IndexBufferID _ibid, const Material& _material) :
        vbid(_vbid),
        ibid(_ibid),
        material(_material){};

    ~Renderable();

    VertexBufferID vbid;
    IndexBufferID ibid;

    Material material;
private:
};