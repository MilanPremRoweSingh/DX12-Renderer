#pragma once
#include <vector>

class Renderable;

class Scene
{
public:
    static Scene* CreateFromFile(
        char* fileName);

private:
    Scene();
    ~Scene();
    
    std::vector<Renderable*> renderables;
};

