#pragma once
#include <vector>

class Renderable;

class Scene
{
public:
    static Scene* Load(
        const char* fileName);

    ~Scene();

    std::vector<Renderable*> pRenderables;
private:
    Scene();
    
};

