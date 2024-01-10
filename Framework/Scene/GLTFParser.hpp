#pragma once

#include "Utilities/Math.hpp"

#include <iostream>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Scene
{
    struct TextureData
    {
        bool IsValid() const { return Buffer.size() == Width * Height * Component; }
        uint32_t GetSize() const { return Buffer.size(); }
    
        uint32_t Width;
        uint32_t Height;
        uint8_t Component;
        std::vector<uint8_t> Buffer;
    };

    struct MaterialData
    {
        std::unique_ptr<TextureData> BaseColorTexture = nullptr;
        std::unique_ptr<TextureData> NormalTexture = nullptr;
    };

    struct VertexData
    {
        Math::Vector4 Position;
        Math::Vector2 TexCoord;
        Math::Vector4 Color;
        Math::Vector3 Normal;
    };

    struct MeshData
    {
        MeshData(uint32_t firstIndex, uint32_t indexCount, uint32_t firstVertex, uint32_t vertexCount, std::unique_ptr<MaterialData>& material)
            : FirstIndex(firstIndex)
            , IndexCount(indexCount)
            , FirstVertex(firstVertex)
            , VertexCount(vertexCount)
            , Material(std::move(material))
        {
        }

        void SetDimensions(Math::Vector3 min, Math::Vector3 max)
        {
            Dimension.Min = min;
            Dimension.Max = max;
            Dimension.Size = max - min;
            Dimension.Center = (min + max) * 0.5f;
            Dimension.Radius = Math::Length(Dimension.Size) * 0.5f;
        }

        std::string Name;
        uint32_t FirstIndex;
        uint32_t IndexCount;
        uint32_t FirstVertex;
        uint32_t VertexCount;

        std::unique_ptr<MaterialData> Material = nullptr;

        struct Dimensions
        {
            Math::Vector3 Min = Math::Vector3(FLT_MAX);
            Math::Vector3 Max = Math::Vector3(-FLT_MAX);
            Math::Vector3 Size;
            Math::Vector3 Center;
            float Radius;
        } Dimension;
    };

    struct Node
    {
        explicit Node(std::shared_ptr<Node>& p)
            : Parent(std::weak_ptr<Node>(p))
            , Matrix(1.0f)
        {}

        Math::Matrix4x4 LocalMatrix() const { return Matrix; }
        Math::Matrix4x4 GetMatrix()
        {
            Math::Matrix4x4 mat = LocalMatrix();
            auto& p = Parent;
            while (p.lock().get() != nullptr)
            {
                mat = p.lock()->LocalMatrix() * mat;
                p = p.lock()->Parent;
            }
            return mat;
        }

        std::weak_ptr<Node> Parent;
        std::vector<std::shared_ptr<Node>> Children;
        Math::Matrix4x4 Matrix;
    };

    class Model
    {
    public:
        Model() = default;
        ~Model() = default;

        void LoadFromFile(const std::string& path);
        void LoadNode(const aiScene* scene, aiNode* node, std::shared_ptr<Node>& parent);
        void LoadMaterials(const aiScene* scene);

    private:
        std::unique_ptr<TextureData> LoadMaterialTexture(const aiScene* scene, const aiMaterial* mat, aiTextureType type, bool fromEmbedded = true) const;

        void CreateEmptyTexture();
    
    public:
        std::shared_ptr<Node> mRootNode;
        std::vector<std::shared_ptr<MaterialData>> mMaterialData;
        std::vector<uint32_t> mRawIndexBuffer;
        std::vector<VertexData> mRawVertexBuffer;

        std::vector<std::unique_ptr<MeshData>> mMeshes;

        std::string mDirectory;

    private:
        std::unique_ptr<TextureData> mEmptyTexture = nullptr;
    };
}