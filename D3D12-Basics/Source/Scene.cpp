#include "Scene.h"

#include "Engine.h"
#include "VertexFormats.h"
#include "Renderable.h"

// For now we do all assimp parsing in Scene::CreateFromFile, if we do more parsing later, it's probably worth moving it to its own file 
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#define ASSIMP_DEFAULT_IMPORT_FLAGS  aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals | aiProcess_MakeLeftHanded | aiProcess_FlipWindingOrder

const float constDefaultVertexNormal[] = { 0.0f, 0.0f, 1.0f };
const float constDefaultVertexColour[] = { 1.0f, 1.0f, 1.0f, 1.0f };

Scene::Scene()
{

}

Scene::~Scene()
{
    for (auto it = pRenderables.begin(); it != pRenderables.end(); it++)
    {
        delete *it;
    }
}

Scene* Scene::Load(
    const char* fileName)
{
    Assimp::Importer importer;

    const aiScene* assimpScene = importer.ReadFile(fileName, ASSIMP_DEFAULT_IMPORT_FLAGS);

    if (!assimpScene || !assimpScene->HasMeshes())
    {
        return nullptr;
    }

    Scene* pScene = new Scene();

    pScene->pRenderables.reserve(assimpScene->mNumMeshes);

    for (uint32 idxMesh = 0; idxMesh < assimpScene->mNumMeshes; idxMesh++)
    {
        if (!assimpScene->mMeshes[idxMesh])
        {
            continue;
        }

        aiMesh& assimpMesh = *assimpScene->mMeshes[idxMesh];

        if (!assimpMesh.HasPositions())
        {
            continue;
        }

        std::vector<Vertex> verts;
        verts.reserve(assimpMesh.mNumVertices);
        for (uint32 idxVert = 0; idxVert < assimpMesh.mNumVertices; idxVert++)
        {
            Vertex vertex;
            static_assert(sizeof(vertex.pos) == sizeof(assimpMesh.mVertices[0]), "Vertex sizes does not match");
            memcpy(&vertex.pos[0], &assimpMesh.mVertices[idxVert], sizeof(vertex.pos));

            if (assimpMesh.HasNormals())
            {
                static_assert(sizeof(vertex.normal) == sizeof(assimpMesh.mNormals[0]), "Normal sizes does not match");
                memcpy(&vertex.normal, (void*)&assimpMesh.mNormals[idxVert], sizeof(vertex.normal));
            }
            else
            {
                static_assert(sizeof(vertex.normal) == sizeof(constDefaultVertexNormal), "Default normal size does not match");
                memcpy(&vertex.normal, (void*)constDefaultVertexNormal, sizeof(vertex.normal));
            }

            if (assimpMesh.HasVertexColors(0))
            {
                static_assert(sizeof(vertex.col) == sizeof(assimpMesh.mColors[0][0]), "Colour sizes does not match");
                memcpy(&vertex.col, (void*)&assimpMesh.mColors[0][idxVert], sizeof(vertex.col));
            }
            else
            {
                static_assert(sizeof(vertex.col) == sizeof(constDefaultVertexColour), "Default colour size does not match");
                memcpy(&vertex.col, (void*)constDefaultVertexColour, sizeof(vertex.col));
            }

            verts.push_back(vertex);
        }

        bool allFacesValid = verts.size();
        
        std::vector<uint32> indices;
        indices.reserve(3 * assimpMesh.mNumFaces);
        for (uint32 idxFace = 0; idxFace < assimpMesh.mNumFaces; idxFace++)
        {
            if (assimpMesh.mFaces[idxFace].mNumIndices != 3)
            {
                // Require that meshes are triangulated before being loaded.
                allFacesValid = false;
                continue;
            }

            indices.push_back(assimpMesh.mFaces[idxFace].mIndices[0]);
            indices.push_back(assimpMesh.mFaces[idxFace].mIndices[1]);
            indices.push_back(assimpMesh.mFaces[idxFace].mIndices[2]);
        }

        if (!allFacesValid || !verts.size() || !indices.size() || (indices.size() % 3) != 0 )
        {
            continue;
        }

        VertexBufferID vbid = g_pRenderer->VertexBufferCreate(verts.size(), verts.data());
        IndexBufferID ibid = g_pRenderer->IndexBufferCreate(indices.size(), indices.data());
        Renderable* pRenderable = new Renderable(vbid, ibid);
        ASSERT(pRenderable);

        pScene->pRenderables.push_back(pRenderable);
    }

    return pScene;
}