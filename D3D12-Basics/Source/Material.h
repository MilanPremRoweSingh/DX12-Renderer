#pragma once

class Texture;

struct Material
{
    char name[100]; 

    Vector3 diffuse;
    // Bind just one texture per material for now
    Texture* diffuseTexture = nullptr;
};