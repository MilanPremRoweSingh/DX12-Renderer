#include "Scene.h"

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
    for (auto it = renderables.begin(); it != renderables.end(); it++)
    {
        delete *it;
    }
}

Scene* Scene::CreateFromFile(
    char* fileName)
{
    Assimp::Importer importer;

    const aiScene* assimpScene = importer.ReadFile(fileName, ASSIMP_DEFAULT_IMPORT_FLAGS);

    if (!assimpScene || !assimpScene->HasMeshes())
    {
        return nullptr;
    }

    Scene* pScene = new Scene();

    pScene->renderables.reserve(assimpScene->mNumMeshes);

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

            indices.push_back(assimpMesh.mFaces[idxMesh].mIndices[0]);
            indices.push_back(assimpMesh.mFaces[idxMesh].mIndices[1]);
            indices.push_back(assimpMesh.mFaces[idxMesh].mIndices[2]);
        }

        if (!allFacesValid || !verts.size() || !indices.size() || (indices.size() % 3) != 0 )
        {
            continue;
        }

        Renderable* pRenderable = Renderable::CreateRenderable(verts.size(), verts.data(), indices.size(), indices.data());
        ASSERT(pRenderable);

        pScene->renderables.push_back(pRenderable);
    }

    return nullptr;
    /*
    aiMesh* teapot = teapotScene->mMeshes[0];
    assert(teapot);
    assert(teapot->HasFaces());
    assert(teapot->HasPositions());

    //Create Vertex Buffer
    Vertex* verts = nullptr;
    if (teapot->HasVertexColors(0))
    {
        verts = (Vertex*)teapot->mVertices;
    }
    else
    {
        verts = new Vertex[teapot->mNumVertices];
        for (uint32 i = 0; i < teapot->mNumVertices; i++)
        {
            verts[i].col[0] = 1.0f;
            verts[i].col[1] = 1.0f;
            verts[i].col[2] = 1.0f;
            verts[i].col[3] = 1.0f;

            verts[i].pos[0] = teapot->mVertices[i].x;
            verts[i].pos[1] = teapot->mVertices[i].y;
            verts[i].pos[2] = teapot->mVertices[i].z;

            verts[i].normal[0] = teapot->mNormals[i].x;
            verts[i].normal[1] = teapot->mNormals[i].y;
            verts[i].normal[2] = teapot->mNormals[i].z;
        }

    }
    context.VertexBufferCreate(teapot->mNumVertices, verts);

    // Create Index Buffer
    std::vector<uint32> indices;
    indices.reserve(3 * teapot->mNumFaces);
    for (uint32 dwFace = 0; dwFace < teapot->mNumFaces; dwFace++)
    {
        const aiFace& face = teapot->mFaces[dwFace];
        for (uint32 indexIndex = 0; indexIndex < face.mNumIndices; indexIndex++)
        {
            indices.push_back(face.mIndices[indexIndex]);
        }
    }
    context.IndexBufferCreate(indices.size(), indices.data());
    */
}