#include "AssetCooker/AssetWriter.hpp"

#include <city.h>
#include <fstream>
#include <sys/stat.h>

namespace
{
    constexpr uint32_t kSectionDescSize = 24;
    inline uint64_t AlignUp(uint64_t v, uint64_t a)
    {
        return (v + a - 1) & ~(a - 1);
    }
}

namespace Assets
{
    bool ReadFileToBuffer(const eastl::string& path, eastl::vector<uint8_t>& outBuffer)
    {
        std::ifstream is(path.c_str(), std::ios::binary);
        if (is.fail()) return false;
        is.seekg(0, std::ios::end);
        std::streamoff length = is.tellg();
        is.seekg(0, std::ios::beg);
        if (length <= 0)
        {
            return false;
        }
        outBuffer.resize(static_cast<size_t>(length));
        is.read(reinterpret_cast<char*>(outBuffer.data()), length);
        return !is.fail();
    }

    bool ReadFileHeadToBuffer(const eastl::string& path, uint32_t nBytes, eastl::vector<uint8_t>& outBuffer)
    {
        std::ifstream is(path.c_str(), std::ios::binary);
        if (is.fail()) return false;
        outBuffer.resize(nBytes);
        is.read(reinterpret_cast<char*>(outBuffer.data()), nBytes);
        const std::streamsize bytesRead = is.gcount();
        is.clear();
        outBuffer.resize(static_cast<size_t>(bytesRead));
        return bytesRead > 0;
    }

    bool WriteFileFromBuffer(const eastl::string& path, const eastl::vector<uint8_t>& data)
    {
        std::ofstream ofs(path.c_str(), std::ios::binary);
        if (ofs.fail()) return false;
        ofs.write(reinterpret_cast<const char*>(data.data()), data.size());
        return !ofs.fail();
    }

    bool FAssetWriter::Write(const eastl::string& dstPath, const FVAssetHeader& header, const FModelCookedData& payload)
    {
        // 1. Serialize the two sections.
        FBinaryWriter metaWriter, payloadWriter;
        WriteMetadataSection(metaWriter, header);
        WriteModelPayload(payloadWriter, payload);

        // 2. Layout: fixed header | section table | metadata section | payload section
        const uint32_t sectionCount = 2;
        const uint64_t metaOffset = AlignUp(kAssetHeaderSize + sectionCount * kSectionDescSize, 16);
        const uint64_t payloadOffset = AlignUp(metaOffset + metaWriter.GetSize(), 16);

        FVAssetHeader finalHeader = header;
        finalHeader.SectionCount = sectionCount;
        finalHeader.PayloadOffset = metaOffset;
        finalHeader.PayloadSize = metaWriter.GetSize() + payloadWriter.GetSize();
        finalHeader.ContentHash = CityHash64(reinterpret_cast<const char*>(payloadWriter.GetData().data()), payloadWriter.GetData().size());

        FBinaryWriter finalWriter;
        WriteHeader(finalWriter, finalHeader);

        FAssetSectionDesc metaDesc;
        metaDesc.Type = EAssetSection::Metadata;
        metaDesc.Offset = metaOffset;
        metaDesc.Size = metaWriter.GetSize();
        WriteSectionDesc(finalWriter, metaDesc);

        FAssetSectionDesc modelDesc;
        modelDesc.Type = EAssetSection::Model;
        modelDesc.Offset = payloadOffset;
        modelDesc.Size = payloadWriter.GetSize();
        WriteSectionDesc(finalWriter, modelDesc);

        const size_t padding1 = metaOffset - finalWriter.GetSize();
        for (size_t i = 0; i < padding1; ++i)
            finalWriter.WriteU8(0);
        finalWriter.Write(metaWriter.GetData().data(), metaWriter.GetSize());
        const size_t padding2 = payloadOffset - finalWriter.GetSize();
        for (size_t i = 0; i < padding2; ++i)
            finalWriter.WriteU8(0);
        finalWriter.Write(payloadWriter.GetData().data(), payloadWriter.GetSize());

        return WriteFileFromBuffer(dstPath, finalWriter.GetData());
    }

    bool FAssetWriter::Write(const eastl::string& dstPath, const FVAssetHeader& header, const FTextureCookedData& payload)
    {
        FBinaryWriter metaWriter, payloadWriter;
        WriteMetadataSection(metaWriter, header);
        payloadWriter.WriteU32(payload.Width);
        payloadWriter.WriteU32(payload.Height);
        payloadWriter.WriteU32(payload.Depth);
        payloadWriter.WriteU32(payload.MipLevels);
        payloadWriter.WriteU32(payload.ArraySize);
        payloadWriter.WriteU32((uint32_t)payload.Format);
        payloadWriter.WriteU32((uint32_t)payload.Type);
        payloadWriter.WriteBlob(payload.MipData);

        const uint32_t sectionCount = 2;
        const uint64_t metaOffset = AlignUp(kAssetHeaderSize + sectionCount * kSectionDescSize, 16);
        const uint64_t payloadOffset = AlignUp(metaOffset + metaWriter.GetSize(), 16);

        FVAssetHeader finalHeader = header;
        finalHeader.SectionCount = sectionCount;
        finalHeader.PayloadOffset = metaOffset;
        finalHeader.PayloadSize = metaWriter.GetSize() + payloadWriter.GetSize();
        finalHeader.ContentHash = CityHash64(reinterpret_cast<const char*>(payloadWriter.GetData().data()), payloadWriter.GetData().size());

        FBinaryWriter finalWriter;
        WriteHeader(finalWriter, finalHeader);

        FAssetSectionDesc metaDesc;
        metaDesc.Type = EAssetSection::Metadata;
        metaDesc.Offset = metaOffset;
        metaDesc.Size = metaWriter.GetSize();
        WriteSectionDesc(finalWriter, metaDesc);

        FAssetSectionDesc texDesc;
        texDesc.Type = EAssetSection::Texture;
        texDesc.Offset = payloadOffset;
        texDesc.Size = payloadWriter.GetSize();
        WriteSectionDesc(finalWriter, texDesc);

        const size_t pad1 = (size_t)(metaOffset - finalWriter.GetSize());
        for (size_t i = 0; i < pad1; i++)
            finalWriter.WriteU8(0);
        finalWriter.Write(metaWriter.GetData().data(), metaWriter.GetSize());
        const size_t pad2 = (size_t)(payloadOffset - finalWriter.GetSize());
        for (size_t i = 0; i < pad2; i++)
            finalWriter.WriteU8(0);
        finalWriter.Write(payloadWriter.GetData().data(), payloadWriter.GetSize());

        return WriteFileFromBuffer(dstPath, finalWriter.GetData());
    }

    bool FAssetWriter::IsUpToDate(const eastl::string& srcPath, const eastl::string& cookedPath)
    {
        struct _stat64 srcStat = {};
        if (_stat64(srcPath.c_str(), &srcStat) != 0)
            return false;
        struct _stat64 cookedStat = {};
        if (_stat64(cookedPath.c_str(), &cookedStat) != 0)
            return false; 

        // Pass 1: read the fixed header only, enough to validate magic/version and get sectionCount.
        eastl::vector<uint8_t> headerBuf;
        if (!ReadFileHeadToBuffer(cookedPath, kAssetHeaderSize, headerBuf) || headerBuf.size() < kAssetHeaderSize)
            return false;

        FBinaryReader headerReader(headerBuf.data(), headerBuf.size());
        uint32_t magic = 0, version = 0, cookerVersion = 0, type = 0, sectionCount = 0, reserved = 0;
        if (!headerReader.ReadU32(magic) || magic != FVAssetHeader::MagicConst ||
            !headerReader.ReadU32(version) || version != FVAssetHeader::CurrentVersion ||
            !headerReader.ReadU32(cookerVersion) || cookerVersion != kAssetCookerVersion ||
            !headerReader.ReadU32(type) || !headerReader.ReadU32(sectionCount) || !headerReader.ReadU32(reserved))
        {
            return false;
        }
        if (sectionCount > 1024)
            return false;

        // Pass 2: header + section table; walk the table to find the Metadata (offset, size).
        const uint32_t headBytes = kAssetHeaderSize + sectionCount * kSectionDescSize;
        eastl::vector<uint8_t> tableBuf;
        if (!ReadFileHeadToBuffer(cookedPath, headBytes, tableBuf) || tableBuf.size() < headBytes)
            return false;

        FBinaryReader tableReader(tableBuf.data(), tableBuf.size());
        if (!tableReader.Seek(kAssetHeaderSize))
            return false;

        uint64_t metaOffset = 0, metaSize = 0;
        bool foundMeta = false;
        for (uint32_t i = 0; i < sectionCount; ++i)
        {
            uint32_t sectionType = 0, sectionReserved = 0;
            uint64_t sectionOffset = 0, sectionSize = 0;
            if (!tableReader.ReadU32(sectionType) || !tableReader.ReadU32(sectionReserved) ||
                !tableReader.ReadU64(sectionOffset) || !tableReader.ReadU64(sectionSize))
            {
                return false;
            }
            if ((EAssetSection)sectionType == EAssetSection::Metadata)
            {
                metaOffset = sectionOffset;
                metaSize = sectionSize;
                foundMeta = true;
                break;
            }
        }
        if (!foundMeta || metaSize < 16)
            return false;

        // Pass 3: read through the end of the Metadata section, then index it by its absolute
        // file offset (the section DATA lives after the fixed header + table — the old bug read
        // only headBytes and indexed by sectionOffset, which always fell outside the buffer).
        const uint64_t metaEnd = metaOffset + metaSize;
        if (metaEnd > UINT32_MAX)
            return false;
        eastl::vector<uint8_t> metaBuf;
        if (!ReadFileHeadToBuffer(cookedPath, static_cast<uint32_t>(metaEnd), metaBuf) || metaBuf.size() < metaEnd)
            return false;

        FBinaryReader metaReader(metaBuf.data() + metaOffset, metaSize);
        uint64_t mtime = 0, size = 0;
        if (!metaReader.ReadU64(mtime) || !metaReader.ReadU64(size))
            return false;

        return mtime == static_cast<uint64_t>(srcStat.st_mtime) && size == static_cast<uint64_t>(srcStat.st_size);
    }

    void FAssetWriter::WriteHeader(FBinaryWriter& writer, const FVAssetHeader& header)
    {
        writer.WriteU32(header.Magic);
        writer.WriteU32(header.Version);
        writer.WriteU32(header.CookerVersion);
        writer.WriteU32((uint32_t)header.Type);
        writer.WriteU32(header.SectionCount);
        writer.WriteU32(header.Reserved);
        writer.WriteU64(header.PayloadOffset);
        writer.WriteU64(header.PayloadSize);
        writer.WriteU64(header.ContentHash);
    }

    void FAssetWriter::WriteSectionDesc(FBinaryWriter& writer, const FAssetSectionDesc& desc)
    {
        writer.WriteU32((uint32_t)desc.Type);
        writer.WriteU32(desc.Reserved);
        writer.WriteU64(desc.Offset);
        writer.WriteU64(desc.Size);
    }

    void FAssetWriter::WriteMetadataSection(FBinaryWriter& writer, const FVAssetHeader& header)
    {
        writer.WriteU64(header.SourceMTime);
        writer.WriteU64(header.SourceSize);
        writer.WriteString(header.SourcePath);
        writer.WriteU32((uint32_t)header.References.size());
        for (const eastl::string& ref : header.References)
        {
            writer.WriteString(ref);
        }
    }

    void FAssetWriter::WriteModelPayload(FBinaryWriter& writer, const FModelCookedData& payload)
    {
        // ------- Mesh Section ------- //
        writer.WriteU32((uint32_t)payload.Meshes.size());
        for (const FMeshCookedData& mesh : payload.Meshes)
        {
            writer.WriteString(mesh.Name);
            writer.WriteI32(mesh.MaterialIndex);
            writer.WritePod(mesh.Center);
            writer.WriteF32(mesh.Radius);
            writer.WriteU32(mesh.IndexCount);
            writer.WriteU32(mesh.VertexCount);
            writer.WriteU32(mesh.IndexStride);
            writer.WriteBlob(mesh.IndexData);
            writer.WriteBool(mesh.bSkeletal);
            writer.WriteBool(mesh.bSkeletalAnim);

            // ------- Vertex Streams ------- //
            writer.WriteU32((uint32_t)mesh.Streams.size());
            for (const FMeshCookedData::FStream& stream : mesh.Streams)
            {
                writer.WriteU32((uint32_t)stream.Type);
                writer.WriteU32(stream.Stride);
                writer.WriteBlob(stream.Data);
            }

            // ------- Meshlet Section ------- //
            writer.WriteU32(mesh.MeshletCount);
            writer.WriteVector(mesh.MeshletBounds);
            writer.WriteBlob(mesh.MeshletIndexData);
            writer.WriteBlob(mesh.MeshletVertexData);
        }
        // ------- Material Section ------- //
        writer.WriteU32((uint32_t)payload.Materials.size());
        for (const FAssetMaterial& mat : payload.Materials)
        {
            writer.WriteString(mat.Name);
            writer.WriteU32((uint32_t)mat.WorkFlow);
            writer.WriteU32((uint32_t)mat.Textures.size());
            for (const FAssetTextureRef& tex : mat.Textures)
            {
                writer.WriteString(tex.RelativePath);
                writer.WriteU32((uint32_t)tex.Usage);
                writer.WriteBool(tex.bSRGB);
                writer.WriteBool(tex.bHasTransform);
                writer.WritePod(tex.Offset);
                writer.WritePod(tex.Scale);
                writer.WriteF32(tex.Rotation);
            }
            writer.WritePod(mat.AlbedoColor);
            writer.WriteF32(mat.Metallic);
            writer.WriteF32(mat.Roughness);
            writer.WritePod(mat.DiffuseColor);
            writer.WritePod(mat.SpecularColor);
            writer.WriteF32(mat.Glossiness);
            writer.WritePod(mat.EmissiveColor);
            writer.WriteF32(mat.AlphaCutout);
            writer.WriteBool(mat.bAlphaTest);
            writer.WriteBool(mat.bAlphaBlend);
            writer.WriteBool(mat.bDoubleSided);
        }

        // ------- Node Section ------- //
        writer.WriteU32((uint32_t)payload.Nodes.size());
        for (const FAssetNode& node : payload.Nodes)
        {
            writer.WriteString(node.Name);
            writer.WriteI32(node.Parent);
            writer.WriteVector(node.Children);
            writer.WritePod(node.Translation);
            writer.WritePod(node.Rotation);
            writer.WritePod(node.Scale);
            writer.WriteBool(node.bFrontFaceCCW);   // per-node winding (Slice 2 review)
            writer.WriteVector(node.Primitives);
        }
        writer.WriteVector(payload.RootNodes);

        // ------- Skeleton Section ------- //
        writer.WriteVector(payload.Skeleton.Joints);
        writer.WriteVector(payload.Skeleton.InverseBindMatrices);

        // ------- Animation Section ------- //
        writer.WriteU32((uint32_t)payload.Animations.size());
        for (const FAssetAnimation& anim : payload.Animations)
        {
            writer.WriteString(anim.Name);
            writer.WriteU32((uint32_t)anim.Channels.size());
            for (const FAssetAnimationChannel& channel : anim.Channels)
            {
                writer.WriteI32(channel.TargetNode);
                writer.WriteU32(channel.Mode);
                writer.WriteU32((uint32_t)channel.KeyFrames.size());
                for (const eastl::pair<float, float4>& kf : channel.KeyFrames)
                {
                    writer.WriteF32(kf.first);
                    writer.WritePod(kf.second);
                }
            }
            writer.WriteF32(anim.TimeDuration);
        }
    }

} // namespace Assets
