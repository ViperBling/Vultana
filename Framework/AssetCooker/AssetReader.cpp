#include "AssetCooker/AssetReader.hpp"
#include "AssetCooker/AssetValidate.hpp"

#include <city.h>

namespace
{
    constexpr uint32_t kSectionDescSize = 24;
}

namespace Assets
{
    bool FAssetReader::ReadHeader(const eastl::string& path, FVAssetHeader& outHeader)
    {
        eastl::vector<uint8_t> data;
        if (!ReadFileToBuffer(path, data))
        {
            return false;
        }
        FBinaryReader r(data.data(), data.size());
        return ReadHeaderRegion(r, outHeader);
    }

    EAssetReadResult FAssetReader::ReadModelChecked(const eastl::string& path, FVAssetHeader& outHeader, FModelCookedData& outPayload)
    {
        eastl::vector<uint8_t> head;
        if (!ReadFileHeadToBuffer(path, kAssetHeaderSize, head) || head.size() < kAssetHeaderSize)
            return EAssetReadResult::NotFound;

        FBinaryReader headReader(head.data(), head.size());
        uint32_t magic = 0, version = 0, cookerVersion = 0;
        if (!headReader.ReadU32(magic) || !headReader.ReadU32(version) || !headReader.ReadU32(cookerVersion))
            return EAssetReadResult::Corrupt;
        if (magic != FVAssetHeader::MagicConst)
            return EAssetReadResult::BadMagic;
        if (version != FVAssetHeader::CurrentVersion || cookerVersion != kAssetCookerVersion)
            return EAssetReadResult::VersionMismatch;

        // Full read + parse + payload hash + cross-reference validation.
        eastl::vector<uint8_t> data;
        if (!ReadFileToBuffer(path, data))
            return EAssetReadResult::NotFound;
        FBinaryReader reader(data.data(), data.size());
        if (!ReadHeaderRegion(reader, outHeader) || outHeader.Type != EAssetType::Model)
            return EAssetReadResult::Corrupt;

        // Re-walk the section table to locate the Model section.
        if (!reader.Seek(kAssetHeaderSize))
            return EAssetReadResult::Corrupt;
        for (uint32_t i = 0; i < outHeader.SectionCount; i++)
        {
            uint32_t secType = 0, secReserved = 0;
            uint64_t secOffset = 0, secSize = 0;
            if (!reader.ReadU32(secType) || !reader.ReadU32(secReserved) ||
                !reader.ReadU64(secOffset) || !reader.ReadU64(secSize))
            {
                return EAssetReadResult::Corrupt;
            }
            if ((EAssetSection)secType != EAssetSection::Model)
                continue;

            // ContentHash covers the Model section's serialized bytes (FAssetWriter::Write:70).
            // A half-written file survives the length checks but fails here (S2-7).
            if (secOffset + secSize > data.size())
                return EAssetReadResult::Corrupt;
            const uint64_t actualHash = CityHash64(reinterpret_cast<const char*>(data.data() + secOffset), static_cast<size_t>(secSize));
            if (actualHash != outHeader.ContentHash)
                return EAssetReadResult::Corrupt;

            if (!reader.Seek(static_cast<size_t>(secOffset)))
                return EAssetReadResult::Corrupt;
            if (!ReadModelPayload(reader, outPayload))
                return EAssetReadResult::Corrupt;
            if (!ValidateModel(outPayload, "ReadModelChecked"))
                return EAssetReadResult::Corrupt;
            return EAssetReadResult::Ok;
        }
        return EAssetReadResult::Corrupt; // no Model section
    }

    bool FAssetReader::ReadModel(const eastl::string& path, FVAssetHeader& outHeader, FModelCookedData& outPayload)
    {
        return ReadModelChecked(path, outHeader, outPayload) == EAssetReadResult::Ok;
    }

    EAssetReadResult FAssetReader::ReadTextureChecked(const eastl::string& path, FVAssetHeader& outHeader, FTextureCookedData& outPayload)
    {
        eastl::vector<uint8_t> head;
        if (!ReadFileHeadToBuffer(path, kAssetHeaderSize, head) || head.size() < kAssetHeaderSize)
            return EAssetReadResult::NotFound;

        FBinaryReader headReader(head.data(), head.size());
        uint32_t magic = 0, version = 0, cookerVersion = 0;
        if (!headReader.ReadU32(magic) || !headReader.ReadU32(version) || !headReader.ReadU32(cookerVersion))
            return EAssetReadResult::Corrupt;
        if (magic != FVAssetHeader::MagicConst)
            return EAssetReadResult::BadMagic;
        if (version != FVAssetHeader::CurrentVersion || cookerVersion != kAssetCookerVersion)
            return EAssetReadResult::VersionMismatch;

        // Full read + parse + payload hash + cross-reference validation (mirrors ReadModelChecked).
        eastl::vector<uint8_t> data;
        if (!ReadFileToBuffer(path, data))
            return EAssetReadResult::NotFound;
        FBinaryReader reader(data.data(), data.size());
        if (!ReadHeaderRegion(reader, outHeader) || outHeader.Type != EAssetType::Texture)
            return EAssetReadResult::Corrupt;

        if (!reader.Seek(kAssetHeaderSize))
            return EAssetReadResult::Corrupt;
        for (uint32_t i = 0; i < outHeader.SectionCount; i++)
        {
            uint32_t secType = 0, secReserved = 0;
            uint64_t secOffset = 0, secSize = 0;
            if (!reader.ReadU32(secType) || !reader.ReadU32(secReserved) ||
                !reader.ReadU64(secOffset) || !reader.ReadU64(secSize))
            {
                return EAssetReadResult::Corrupt;
            }
            if ((EAssetSection)secType != EAssetSection::Texture)
                continue;

            if (secOffset + secSize > data.size())
                return EAssetReadResult::Corrupt;
            const uint64_t actualHash = CityHash64(reinterpret_cast<const char*>(data.data() + secOffset), static_cast<size_t>(secSize));
            if (actualHash != outHeader.ContentHash)
                return EAssetReadResult::Corrupt;

            if (!reader.Seek(static_cast<size_t>(secOffset)))
                return EAssetReadResult::Corrupt;
            uint32_t format = 0, type = 0;
            if (!reader.ReadU32(outPayload.Width) || !reader.ReadU32(outPayload.Height) ||
                !reader.ReadU32(outPayload.Depth) || !reader.ReadU32(outPayload.MipLevels) ||
                !reader.ReadU32(outPayload.ArraySize) || !reader.ReadU32(format) || !reader.ReadU32(type) ||
                !reader.ReadBlob(outPayload.MipData))
            {
                return EAssetReadResult::Corrupt;
            }
            outPayload.Format = (ETextureFormat)format;
            outPayload.Type = (ETextureType)type;
            if (!ValidateTexture(outPayload, "ReadTextureChecked"))
                return EAssetReadResult::Corrupt;
            return EAssetReadResult::Ok;
        }
        return EAssetReadResult::Corrupt; // no Texture section
    }

    bool FAssetReader::ReadTexture(const eastl::string& path, FVAssetHeader& outHeader, FTextureCookedData& outPayload)
    {
        return ReadTextureChecked(path, outHeader, outPayload) == EAssetReadResult::Ok;
    }

    bool FAssetReader::ReadHeaderRegion(FBinaryReader& reader, FVAssetHeader& outHeader)
    {
        if (!reader.ReadU32(outHeader.Magic) || outHeader.Magic != FVAssetHeader::MagicConst)
            return false;
        
        uint32_t type = 0;
        if (!reader.ReadU32(outHeader.Version) || outHeader.Version != FVAssetHeader::CurrentVersion ||
            !reader.ReadU32(outHeader.CookerVersion) || outHeader.CookerVersion != kAssetCookerVersion ||
            !reader.ReadU32(type) ||
            !reader.ReadU32(outHeader.SectionCount) || !reader.ReadU32(outHeader.Reserved) ||
            !reader.ReadU64(outHeader.PayloadOffset) || !reader.ReadU64(outHeader.PayloadSize) ||
            !reader.ReadU64(outHeader.ContentHash))
        {
            return false;
        }
        outHeader.Type = static_cast<EAssetType>(type);

        // section table -> locate the Metadata section
        for (uint32_t i = 0; i < outHeader.SectionCount; i++)
        {
            uint32_t sectionType = 0, sectionReserved = 0;
            uint64_t sectionOffset = 0, sectionSize = 0;
            if (!reader.ReadU32(sectionType) || !reader.ReadU32(sectionReserved) ||
                !reader.ReadU64(sectionOffset) || !reader.ReadU64(sectionSize))
            {
                return false;
            }
            if ((EAssetSection)sectionType != EAssetSection::Metadata)
            {
                continue;
            }
            if (!reader.Seek((size_t)sectionOffset))
            {
                return false;
            }
            if (!reader.ReadU64(outHeader.SourceMTime) || !reader.ReadU64(outHeader.SourceSize) ||
                !reader.ReadString(outHeader.SourcePath))
            {
                return false;
            }
            uint32_t refCount = 0;
            // A reference is at least 4B (string length prefix; M8: 2 was too lax for a corrupt file)
            if (!reader.ReadU32(refCount) || refCount > (reader.GetSize() - reader.GetOffset()) / 4)
            {
                return false;
            }
            outHeader.References.clear();
            outHeader.References.reserve(refCount);
            for (uint32_t j = 0; j < refCount; j++)
            {
                eastl::string ref;
                if (!reader.ReadString(ref))
                {
                    return false;
                }
                outHeader.References.push_back(eastl::move(ref));
            }
            return true;
        }
        return false; // no Metadata section
    }

    bool FAssetReader::ReadModelPayload(FBinaryReader& reader, FModelCookedData& outPayload)
    {
        // ---- Meshes ----
        uint32_t meshCount = 0;
        if (!reader.ReadU32(meshCount) || meshCount > (reader.GetSize() - reader.GetOffset()) / kMinMeshRecordSize)
        {
            return false;
        }
        outPayload.Meshes.clear();
        outPayload.Meshes.reserve(meshCount);
        for (uint32_t i = 0; i < meshCount; ++i)
        {
            FMeshCookedData mesh;
            int32_t materialIndex = -1;
            if (!reader.ReadString(mesh.Name) || !reader.ReadI32(materialIndex) ||
                !reader.ReadPod(mesh.Center) || !reader.ReadF32(mesh.Radius) ||
                !reader.ReadU32(mesh.IndexCount) || !reader.ReadU32(mesh.VertexCount) ||
                !reader.ReadU32(mesh.IndexStride) || !reader.ReadBlob(mesh.IndexData) ||
                !reader.ReadBool(mesh.bSkeletal) || !reader.ReadBool(mesh.bSkeletalAnim))
            {
                return false;
            }
            mesh.MaterialIndex = materialIndex;

            uint32_t streamCount = 0;
            if (!reader.ReadU32(streamCount) || streamCount > (reader.GetSize() - reader.GetOffset()) / 12)
            {
                return false;   // A stream is at least 12B (Type 4 + Stride 4 + blob 4)
            }
            mesh.Streams.reserve(streamCount);
            for (uint32_t s = 0; s < streamCount; ++s)
            {
                FMeshCookedData::FStream stream;
                uint32_t streamType = 0;
                if (!reader.ReadU32(streamType) || !reader.ReadU32(stream.Stride) || !reader.ReadBlob(stream.Data))
                {
                    return false;
                }
                stream.Type = (EVertexStreamType)streamType;
                mesh.Streams.push_back(eastl::move(stream));
            }
            if (!reader.ReadU32(mesh.MeshletCount) || !reader.ReadVector(mesh.MeshletBounds) ||
                !reader.ReadBlob(mesh.MeshletIndexData) || !reader.ReadBlob(mesh.MeshletVertexData))
            {
                return false;
            }
            outPayload.Meshes.push_back(eastl::move(mesh));
        }

        // ---- Material table ----
        uint32_t materialCount = 0;
        if (!reader.ReadU32(materialCount) || materialCount > (reader.GetSize() - reader.GetOffset()) / kMinMaterialRecordSize)
        {
            return false; // M8: a corrupt count must not drive an unbounded reserve below
        }
        outPayload.Materials.clear();
        outPayload.Materials.reserve(materialCount);
        for (uint32_t i = 0; i < materialCount; i++)
        {
            FAssetMaterial mat;
            uint32_t workflow = 0, textureCount = 0;
            if (!reader.ReadString(mat.Name) || !reader.ReadU32(workflow) ||
                !reader.ReadU32(textureCount) ||
                textureCount > (reader.GetSize() - reader.GetOffset()) / kMinTextureRefRecordSize)
            {
                return false;
            }
            mat.WorkFlow = (EMaterialWorkFlow)workflow;
            mat.Textures.reserve(textureCount);
            for (uint32_t t = 0; t < textureCount; t++)
            {
                FAssetTextureRef ref;
                uint32_t usage = 0;
                if (!reader.ReadString(ref.RelativePath) || !reader.ReadU32(usage) ||
                    !reader.ReadBool(ref.bSRGB) || !reader.ReadBool(ref.bHasTransform))
                {
                    return false;
                }
                ref.Usage = (ETextureUsage)usage;
                if (!reader.ReadPod(ref.Offset) || !reader.ReadPod(ref.Scale) || !reader.ReadF32(ref.Rotation))
                {
                    return false;
                }
                mat.Textures.push_back(eastl::move(ref));
            }
            if (!reader.ReadPod(mat.AlbedoColor) || !reader.ReadF32(mat.Metallic) || !reader.ReadF32(mat.Roughness) ||
                !reader.ReadPod(mat.DiffuseColor) || !reader.ReadPod(mat.SpecularColor) || !reader.ReadF32(mat.Glossiness) ||
                !reader.ReadPod(mat.EmissiveColor) || !reader.ReadF32(mat.AlphaCutout))
            {
                return false;
            }
            if (!reader.ReadBool(mat.bAlphaTest) || !reader.ReadBool(mat.bAlphaBlend) ||
                !reader.ReadBool(mat.bDoubleSided))
            {
                return false;
            }
            outPayload.Materials.push_back(eastl::move(mat));
        }

        // ---- Node table ----
        uint32_t nodeCount = 0;
        if (!reader.ReadU32(nodeCount) || nodeCount > (reader.GetSize() - reader.GetOffset()) / 57)
        {
            return false; // A node is at least 57B (Name 4 + Parent 4 + Children 4 + TRS 40 + bFrontFaceCCW 1 + Primitives 4)
        }
        outPayload.Nodes.clear();
        outPayload.Nodes.reserve(nodeCount);
        for (uint32_t i = 0; i < nodeCount; i++)
        {
            FAssetNode assetNode;
            uint32_t childCount = 0;
            if (!reader.ReadString(assetNode.Name) || !reader.ReadI32(assetNode.Parent) || !reader.ReadU32(childCount) ||
                childCount > (reader.GetSize() - reader.GetOffset()) / 4)
            {
                return false;   // A child is at least 4B
            }
            assetNode.Children.reserve(childCount);
            for (uint32_t c = 0; c < childCount; c++)
            {
                int32_t child = -1;
                if (!reader.ReadI32(child))
                {
                    return false;
                }
                assetNode.Children.push_back(child);
            }
            if (!reader.ReadPod(assetNode.Translation) || !reader.ReadPod(assetNode.Rotation) || !reader.ReadPod(assetNode.Scale) ||
                !reader.ReadBool(assetNode.bFrontFaceCCW) || !reader.ReadVector(assetNode.Primitives))
            {
                return false;
            }
            outPayload.Nodes.push_back(eastl::move(assetNode));
        }
        if (!reader.ReadVector(outPayload.RootNodes))
        {
            return false;
        }
        if (!reader.ReadVector(outPayload.Skeleton.Joints))
        {
            return false;
        }
        if (!reader.ReadVector(outPayload.Skeleton.InverseBindMatrices))
        {
            return false;
        }

        // ---- Animations ----
        uint32_t animCount = 0;
        if (!reader.ReadU32(animCount) || animCount > (reader.GetSize() - reader.GetOffset()) / 12)
        {
            return false; // An animation is at least 12B (Name 4 + Channels 4 + TimeDuration 4)
        }
        outPayload.Animations.clear();
        outPayload.Animations.reserve(animCount);
        for (uint32_t i = 0; i < animCount; i++)
        {
            FAssetAnimation anim;
            uint32_t channelCount = 0;
            if (!reader.ReadString(anim.Name) || !reader.ReadU32(channelCount) ||
                channelCount > (reader.GetSize() - reader.GetOffset()) / 12)
            {
                return false; // A channel is at least 12B (TargetNode 4 + Mode 4 + KeyFrames 4)
            }
            anim.Channels.reserve(channelCount);
            for (uint32_t c = 0; c < channelCount; c++)
            {
                FAssetAnimationChannel channel;
                uint32_t keyFrameCount = 0;
                if (!reader.ReadI32(channel.TargetNode) || !reader.ReadU32(channel.Mode) || !reader.ReadU32(keyFrameCount) ||
                    keyFrameCount > (reader.GetSize() - reader.GetOffset()) / 20)
                {
                    return false; // A keyframe is at least 20B (time 4 + float4 16)
                }
                channel.KeyFrames.reserve(keyFrameCount);
                for (uint32_t k = 0; k < keyFrameCount; k++)
                {
                    float time = 0.0f;
                    float4 value;
                    if (!reader.ReadF32(time) || !reader.ReadPod(value))
                    {
                        return false;
                    }
                    channel.KeyFrames.push_back(eastl::make_pair(time, value));
                }
                anim.Channels.push_back(eastl::move(channel));
            }
            if (!reader.ReadF32(anim.TimeDuration))
            {
                return false;
            }
            outPayload.Animations.push_back(eastl::move(anim));
        }
        return true;
    }
} // namespace Assets