#pragma once
#include <vector>

class Renderable;

class Scene
{
public:
    static Scene* Load(
        const char* fileName);

    static void Unload(
        Scene* pScene);


    std::vector<Renderable*> pRenderables;
private:
    Scene();
    ~Scene();
    
};

