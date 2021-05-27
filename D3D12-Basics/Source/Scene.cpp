#include "Scene.h"

#include "Engine.h"

#include "Renderer/VertexFormats.h"
#include "Renderer/Renderable.h"
#include "Renderer/Texture.h"

#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#define TEXTURE_DIR_PATH "../Data/Textures/"

#define ASSIMP_DEFAULT_IMPORT_FLAGS  aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals | aiProcess_MakeLeftHanded | aiProcess_FlipWindingOrder

const float constDefaultVertexNormal[] = { 0.0f, 0.0f, 1.0f };
const float constDefaultVertexColour[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const float constDefaultUV[] = { 0.0f, 0.0f };

Scene::Scene()
{

}

Scene::~Scene()
{
    for (auto it = m_pRenderables.begin(); it != m_pRenderables.end(); it++)
    {
        delete *it;
    }

    for (auto it = m_textures.begin(); it != m_textures.end(); it++)
    {
        delete it->second;
    }
}

Texture* Scene::GetOrCreateTextureFromPath(
    const std::string& filePath)
{
    if (m_textures.find(filePath) == m_textures.end())
    {
        m_textures[filePath] = Texture::CreateFromFile(filePath.c_str());
    }
    return m_textures[filePath];
}


Scene* Scene::Load(
    const char* fileName)
{
    Assimp::Importer importer;

    const aiScene* pAssimpScene = importer.ReadFile(fileName, ASSIMP_DEFAULT_IMPORT_FLAGS);

    if (!pAssimpScene || !pAssimpScene->HasMeshes())
    {
        return nullptr;
    }

    Scene* pScene = new Scene();

    pScene->m_pRenderables.reserve(pAssimpScene->mNumMeshes);

    for (uint32 idxMesh = 0; idxMesh < pAssimpScene->mNumMeshes; idxMesh++)
    {
        if (!pAssimpScene->mMeshes[idxMesh])
        {
            continue;
        }

        aiMesh& assimpMesh = *pAssimpScene->mMeshes[idxMesh];

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

            if (assimpMesh.HasTextureCoords(0))
            {
                ASSERT(sizeof(vertex.uv) == assimpMesh.mNumUVComponents[0]*sizeof(vertex.uv[0]));
                memcpy(&vertex.uv, (void*)&assimpMesh.mTextureCoords[0][idxVert], sizeof(vertex.uv));
            }
            else
            {
                static_assert(sizeof(vertex.uv) == sizeof(constDefaultUV), "Default UV size does not match");
                memcpy(&vertex.uv, (void*)constDefaultUV, sizeof(vertex.uv));
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

        Material material;
        if (pAssimpScene->HasMaterials())
        {
            const aiMaterial* pAssimpMaterial = pAssimpScene->mMaterials[assimpMesh.mMaterialIndex]; 
            aiString aiMaterialName;
            pAssimpMaterial->Get(AI_MATKEY_NAME, aiMaterialName);
            strcpy_s(material.name, aiMaterialName.C_Str());

            aiColor3D color(0.f, 0.f, 0.f);
            pAssimpMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color);
            material.diffuse = { color.r, color.g, color.b};

            if (pAssimpMaterial->GetTextureCount(aiTextureType_DIFFUSE))
            {
                aiString assimpTexName;
                pAssimpMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &assimpTexName);

                std::string texPath = TEXTURE_DIR_PATH;
                texPath.append(assimpTexName.C_Str());

                material.diffuseTexture = pScene->GetOrCreateTextureFromPath(texPath.c_str());
            }
        }

        Renderable* pRenderable = new Renderable(vbid, ibid, material);
        ASSERT(pRenderable);

        pScene->m_pRenderables.push_back(pRenderable);
    }

    return pScene;
}