#pragma once
#include <vector>
#include <unordered_map>

class Renderable;
class Texture;

class Scene
{
public:
    static Scene* Load(
        const char* fileName);

    ~Scene();

    std::vector<Renderable*> m_pRenderables;
    std::unordered_map<std::string, Texture*> m_textures;
private:
    Scene();
    
    Texture* GetOrCreateTextureFromPath(
        const std::string& filePath);
};

