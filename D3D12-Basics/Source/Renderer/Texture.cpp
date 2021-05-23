#include "Texture.h"

#include "Engine.h"
#include "Renderer.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Texture* Texture::CreateFromFile(
    char const* filename)
{
    int32 width, height, numChannels;
    unsigned char* pTexData = stbi_load(filename, &width, &height, &numChannels, 0);

    Texture* pTexture = nullptr;

    if (pTexData)
    {
        TextureID id = g_pRenderer->TextureCreate(width, height, numChannels, (void*)pTexData);
        free(pTexData);

        pTexture = new Texture(width, height, numChannels, id);
    }
    return pTexture;
}

TextureID Texture::GetID(
    void)
{
    return m_id;
}