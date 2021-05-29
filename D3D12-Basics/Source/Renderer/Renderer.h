#pragma once
class D3D12Core;
class Camera;
class Scene;
struct RenderConstants;
struct RenderConstantEntry;
struct RenderContext;
struct ConstantDataEntry;
struct Vertex;

enum VertexBufferID;
enum IndexBufferID;
enum TextureID;

#define NUM_SWAP_CHAIN_BUFFERS 2
// For now assume all shaders are in the same file
#define SHADER_FILE L"../Shaders/Shaders.hlsl"

class Renderer 
{
public:
    Renderer();
    ~Renderer();

    void Render(
        void);

    void FlushGPU(
        void);

    void UploadBegin(
        void);

    void UploadEnd(
        void);

    void ConstantDataSetEntry(
        const ConstantDataEntry& entry,
        const void* data);
    
    void ConstantDataFlush(
        void);

    void CameraSet(
        const Camera* pCamera);

    void SceneSet(
        const Scene* pScene);

    VertexBufferID VertexBufferCreate(
        size_t  numVerts, 
        Vertex* pData);

    void VertexBufferDestroy(
        VertexBufferID vbid);

    IndexBufferID IndexBufferCreate(
        size_t numIndices, 
        uint32* pData);

    void IndexBufferDestroy(
        IndexBufferID ibid);

    TextureID TextureCreate(
        int32 width, 
        int32 height,
        int32 numChannels,
        void* pData);

    void TextureDestroy(
        TextureID tid);

private:

    void ConstantDataInitialise();
    void ConstantDataDispose();

    D3D12Core* m_core;   
    RenderContext* m_context;
};

