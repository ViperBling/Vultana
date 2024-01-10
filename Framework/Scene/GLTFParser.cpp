#include "GLTFParser.hpp"
#include "Utilities/Utility.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Scene
{
    void Model::LoadFromFile(const std::string &path)
    {
        Assimp::Importer importer;
        const auto* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            GDebugInfoCallback("GLTFLoader", "Failed to load model from file: " + path);
            exit(-1);
        }

        mDirectory = path.substr(0, path.find_last_of('/'));
        assert(scene->HasMaterials() && scene->HasMeshes());

        LoadMaterials(scene);

        std::shared_ptr<Node> parentRoot = nullptr;
        mRootNode = std::shared_ptr<Node>(new Node(parentRoot));
        for (uint32_t i = 0; i < scene->mRootNode->mNumChildren; ++i)
        {
            LoadNode(scene, scene->mRootNode->mChildren[i], mRootNode);
        }
    }

    void Model::LoadNode(const aiScene *scene, aiNode *node, std::shared_ptr<Node> &parent)
    {
        auto tfNode = std::shared_ptr<Node>(new Node(parent));
        tfNode->Matrix = Math::Transpose(glm::make_mat4x4(&node->mTransformation.a1));

        for (uint32_t i = 0; i < node->mNumChildren; i++)
        {
            LoadNode(scene, node->mChildren[i], tfNode);
        }

        const auto localMatrix = tfNode->GetMatrix();
        for (uint32_t i = 0; i < node->mNumMeshes; i++)
        {
            auto* mesh = scene->mMeshes[node->mMeshes[i]];

            auto indexStart = static_cast<uint32_t>(mRawIndexBuffer.size());
            auto vertexStart = static_cast<uint32_t>(mRawVertexBuffer.size());
            uint32_t indexCount = 0;
            uint32_t vertexCount = 0;

            Math::Vector3 posMin {};
            Math::Vector3 posMax {};

            {
                auto indexOffset = static_cast<uint32_t>(mRawVertexBuffer.size());
                for (uint32_t l = 0; l < mesh->mNumFaces; l++)
                {
                    const auto& face = mesh->mFaces[l];
                    for (uint32_t m = 0; m < face.mNumIndices; m++)
                    {
                        mRawIndexBuffer.emplace_back(indexOffset + face.mIndices[m]);
                        indexCount++;
                    }
                }
            }

            // node vertices
            {
                vertexCount = static_cast<uint32_t>(mesh->mNumVertices);

                for (uint32_t i = 0; i < vertexCount; i++) {
                    VertexData vert {};
                    Math::Vector3 vector;

                    // pos
                    vector.x = mesh->mVertices[i].x;
                    vector.y = mesh->mVertices[i].y;
                    vector.z = mesh->mVertices[i].z;

                    vert.Position = Math::Vector4(vector, 1.0f);

                    // normal
                    if (mesh->HasNormals()) {
                        vector.x = mesh->mNormals[i].x;
                        vector.y = mesh->mNormals[i].y;
                        vector.z = mesh->mNormals[i].z;
                        vert.Normal = vector;
                    }

                    if (mesh->mTextureCoords[0]) {
                        vert.TexCoord.x = mesh->mTextureCoords[0][i].x;
                        vert.TexCoord.y = mesh->mTextureCoords[0][i].y;
                    }

                    if (mesh->HasVertexColors(0)) {
                        vert.Color.r = mesh->mColors[0][i].r;
                        vert.Color.g = mesh->mColors[0][i].g;
                        vert.Color.b = mesh->mColors[0][i].b;
                        vert.Color.a = mesh->mColors[0][i].a;
                    } else {
                        vert.Color = Math::Vector4(1.0f);
                    }

                    // pre transform vert
                    vert.Position = localMatrix * vert.Position;
                    vert.Normal = Math::Normalize(glm::mat3(localMatrix) * vert.Normal);

                    mRawVertexBuffer.emplace_back(vert);
                }
            }
            auto matData = std::unique_ptr<MaterialData>(mMaterialData[mesh->mMaterialIndex].get());
            auto tfMesh = std::unique_ptr<MeshData>(new MeshData(indexStart, indexCount, vertexStart, vertexCount, matData));
            tfMesh->SetDimensions(
                glm::make_vec3(&mesh->mAABB.mMin.x),
                glm::make_vec3(&mesh->mAABB.mMax.x)
            );
            mMeshes.emplace_back(std::move(tfMesh));
        }

        parent->Children.emplace_back(std::move(tfNode));
    }

    void Model::LoadMaterials(const aiScene *scene)
    {
        bool fromEmbedded = scene->HasMaterials();

        for (uint32_t i = 0; i < scene->mNumMaterials; i++)
        {
            auto* material = scene->mMaterials[i];

            auto mat = std::shared_ptr<MaterialData>(new MaterialData());
            mat->BaseColorTexture = LoadMaterialTexture(scene, material, aiTextureType_DIFFUSE, fromEmbedded);
            mat->NormalTexture = LoadMaterialTexture(scene, material, aiTextureType_NORMALS, fromEmbedded);

            mMaterialData.emplace_back(std::move(mat));
        }
    }

    std::unique_ptr<TextureData> Model::LoadMaterialTexture(const aiScene *scene, const aiMaterial *mat, aiTextureType type, bool fromEmbedded) const
    {
        if (mat->GetTextureCount(type) == 0) { return nullptr; }

        aiString fileName;
        mat->GetTexture(type, 0, &fileName);

        void* data;
        int width;
        int height;
        int comp;
        if (fromEmbedded)
        {
            const auto* texData = scene->GetEmbeddedTexture(fileName.C_Str());
            data = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(texData->pcData), texData->mWidth, &width, &height, &comp, 0);
        }
        else
        {
            auto filePath = mDirectory + "/" + std::string(fileName.C_Str());
            data = stbi_load(filePath.c_str(), &width, &height, &comp, 0);
        }

        auto textureData = std::unique_ptr<TextureData>(new TextureData());

        if (comp == 3)
        {
            textureData->Buffer.resize(width * height * 4);
            auto* rgba = textureData->Buffer.data();
            auto* rgb = static_cast<unsigned char*>(data);
            for (uint32_t i = 0; i < width * height; ++i)
            {
                for (uint32_t j = 0; j < 3; j++) { rgba[j] = rgb[j]; }
                rgba += 4;
                rgb += 3;
            }
        }
        else
        {
            textureData->Buffer = std::vector<unsigned char>(static_cast<unsigned char*>(data), static_cast<unsigned char*>(data) + width * height * comp);
        }

        textureData->Width = width;
        textureData->Height = height;
        textureData->Component = 4;

        stbi_image_free(data);
        return std::move(textureData);
    }

    void Model::CreateEmptyTexture()
    {
        mEmptyTexture->Width = 1;
        mEmptyTexture->Height = 1;
        mEmptyTexture->Component = 4;
        mEmptyTexture->Buffer = std::vector<unsigned char> {0, 0, 0, 0};
    }
}