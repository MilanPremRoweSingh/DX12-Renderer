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

private:
    Scene();
    ~Scene();
    
    std::vector<Renderable*> renderables;
};

