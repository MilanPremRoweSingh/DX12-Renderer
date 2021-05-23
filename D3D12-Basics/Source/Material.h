#pragma once

class Texture;

struct Material
{
    Vector3 diffuse;
    // Bind just one texture per material for now
    Texture* diffuseTexture = nullptr;
};