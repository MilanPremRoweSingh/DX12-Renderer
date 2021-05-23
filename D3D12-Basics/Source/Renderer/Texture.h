#pragma once

enum TextureID;

class Texture
{
public:
    static Texture* CreateFromFile(
        char const* filename);

    TextureID GetID(
        void);

private:
    Texture() = delete;
    Texture(const Texture&) = delete;

    Texture(
        int32 width, 
        int32 height, 
        int32 numChannels, 
        TextureID id) : 
        m_width(width),
        m_height(height),
        m_numChannels(numChannels),
        m_id(id) {}

    int32 m_width;
    int32 m_height;
    int32 m_numChannels;

    TextureID m_id;
};
