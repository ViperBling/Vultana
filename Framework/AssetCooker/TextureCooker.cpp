#include "AssetCooker/TextureCooker.hpp"

#include "Utilities/Log.hpp"
#include "AssetCooker/BinaryIO.hpp" // ReadFileToBuffer

#include "stb_image.h"
#include "stb_image_resize2.h"

#if VULTANA_HAS_ISPC_TEXCOMP
#include <ISPCTextureCompressor/ispc_texcomp/ispc_texcomp.h>
#endif

#include <EASTL/algorithm.h>
#include <EASTL/vector.h>

#include <cstring>

namespace Assets
{
    // One cooked payload must fit a single staging allocation (RHI_MAX_BUFFER_SIZE,
    // Framework/RHI/RHICommon.hpp:20). Keep in sync if that limit changes.
    constexpr uint32_t kMaxTexturePayloadBytes = 64u * 1024u * 1024u;

    inline uint32_t Align4Up(uint32_t v)
    {
        return (v + 3u) & ~3u;
    }

    bool IsHDRUsage(ETextureUsage usage)
    {
        return usage == ETextureUsage::EnvironmentHDR;
    }

    // Copy an RGBA8 image into a larger 4-aligned buffer, repeating the last column/row (edge clamp) — no stretching, no source texel dropped.
    void EdgePadRGBA8(const uint8_t* src, uint32_t sw, uint32_t sh, uint8_t* dst, uint32_t dw, uint32_t dh)
    {
        for (uint32_t y = 0; y < dh; ++y)
        {
            const uint32_t sy = y < sh ? y : sh - 1;
            const uint8_t* srcRow = src + (size_t)sy * sw * 4;
            uint8_t* dstRow = dst + (size_t)y * dw * 4;
            memcpy(dstRow, srcRow, (size_t)sw * 4);
            const uint8_t* lastTexel = srcRow + (size_t)(sw - 1) * 4;
            for (uint32_t x = sw; x < dw; ++x)
                memcpy(dstRow + (size_t)x * 4, lastTexel, 4);
        }
    }

#if VULTANA_HAS_ISPC_TEXCOMP
    // ispc_texcomp: one whole-surface call per mip, output tightly packed 4x4 blocks
    // (16B BC7/BC5, 8B BC4). bAlpha selects the BC7 alpha profile — the RGB profiles
    // IGNORE the alpha channel, which would break alpha-tested materials.
    bool CompressMip(ETextureFormat target, const uint8_t* rgba, uint32_t rw, uint32_t rh, bool bAlpha, eastl::vector<uint8_t>& outMipData)
    {
        rgba_surface surface;
        surface.ptr = const_cast<uint8_t*>(rgba);
        surface.width = (int32_t)rw;
        surface.height = (int32_t)rh;
        surface.stride = (int32_t)(rw * 4); // bytes; RGBA8 (BC7)

        bc7_enc_settings bc7Settings;
        if (target == ETextureFormat::BC7UNORM || target == ETextureFormat::BC7SRGB)
            bAlpha ? GetProfile_alpha_basic(&bc7Settings) : GetProfile_basic(&bc7Settings);
        else
            GetProfile_basic(&bc7Settings); // BC5/BC4 ignore it; keep the call cheap

        // BC4/BC5 kernels expect PACKED R8 / RG8 surfaces (kernel.ispc: load_block_r_8bit
        // reads one uint32 as 4 consecutive R8 pixels) — feeding RGBA8 would read one
        // pixel's B,G,R,A bytes as four R values -> noise (ComplexAgent B4).
        eastl::vector<uint8_t> packed;
        if (target == ETextureFormat::BC4UNORM)
        {
            packed.resize((size_t)rw * rh);
            for (size_t i = 0; i < (size_t)rw * rh; ++i)
                packed[i] = rgba[i * 4 + 0];
            surface.ptr = packed.data();
            surface.stride = (int32_t)rw;
        }
        else if (target == ETextureFormat::BC5UNORM)
        {
            packed.resize((size_t)rw * rh * 2);
            for (size_t i = 0; i < (size_t)rw * rh; ++i)
            {
                packed[i * 2 + 0] = rgba[i * 4 + 0];
                packed[i * 2 + 1] = rgba[i * 4 + 1];
            }
            surface.ptr = packed.data();
            surface.stride = (int32_t)(rw * 2);
        }

        const uint32_t blocksX = rw / 4;
        const uint32_t blocksY = rh / 4;
        const uint32_t blockBytes = target == ETextureFormat::BC4UNORM ? 8u : 16u;
        const size_t dstOffset = outMipData.size();
        outMipData.resize(dstOffset + (size_t)blocksX * blocksY * blockBytes);

        switch (target)
        {
        case ETextureFormat::BC7UNORM:
        case ETextureFormat::BC7SRGB:
            CompressBlocksBC7(&surface, outMipData.data() + dstOffset, &bc7Settings);
            break;
        case ETextureFormat::BC5UNORM:
            CompressBlocksBC5(&surface, outMipData.data() + dstOffset); // R+G channels
            break;
        case ETextureFormat::BC4UNORM:
            CompressBlocksBC4(&surface, outMipData.data() + dstOffset); // R channel
            break;
        default:
            return false;
        }
        return true;
    }
#endif

    bool FTextureCooker::IsSRGBUsage(ETextureUsage usage)
    {
        return usage == ETextureUsage::BaseColor || usage == ETextureUsage::Emissive || usage == ETextureUsage::Diffuse;
    }

    bool FTextureCooker::Cook(const eastl::string& srcPath, ETextureUsage usage, FTextureCookedData& outPayload)
    {
        outPayload = FTextureCookedData();
        const bool bHDR = IsHDRUsage(usage);
        const bool bSRGB = IsSRGBUsage(usage);

        // ---- 1. Decode (file -> memory -> stbi_*_from_memory; mirrors FTextureLoader::LoadSTB) ----
        eastl::vector<uint8_t> fileData;
        if (!ReadFileToBuffer(srcPath, fileData))
        {
            VTNA_LOG_ERROR("[TextureCooker] failed to read {}", srcPath);
            return false;
        }
        const int fileSize = (int)fileData.size();

        int w = 0, h = 0, comp = 0;
        if (bHDR)
        {
            float* pixels = stbi_loadf_from_memory(fileData.data(), fileSize, &w, &h, &comp, 4);
            if (pixels == nullptr)
            {
                VTNA_LOG_ERROR("[TextureCooker] stbi failed to decode HDR {}", srcPath);
                return false;
            }

            // RGBA32F full mip chain (no BCn in v1; BC6H is a v2 item — Design doc).
            outPayload.Width = (uint32_t)w;
            outPayload.Height = (uint32_t)h;
            outPayload.Depth = 1;
            outPayload.MipLevels = GetTextureMipLevels((uint32_t)w, (uint32_t)h, 1);
            outPayload.ArraySize = 1;
            outPayload.Type = ETextureType::Texture2D;
            outPayload.Format = ETextureFormat::RGBA32F;

            eastl::vector<float> prev;
            eastl::vector<float> cur;
            for (uint32_t mip = 0; mip < outPayload.MipLevels; ++mip)
            {
                const uint32_t rw = GetTextureMipDim((uint32_t)w, mip, 1);
                const uint32_t rh = GetTextureMipDim((uint32_t)h, mip, 1);
                if (mip == 0)
                {
                    cur.assign(pixels, pixels + (size_t)w * h * 4);
                }
                else
                {
                    cur.resize((size_t)rw * rh * 4);
                    const bool ok = stbir_resize_float_linear(
                        prev.data(), (int)GetTextureMipDim((uint32_t)w, mip - 1, 1), (int)GetTextureMipDim((uint32_t)h, mip - 1, 1), 0,
                        cur.data(), (int)rw, (int)rh, 0, STBIR_RGBA) != nullptr;
                    if (!ok)
                    {
                        VTNA_LOG_ERROR("[TextureCooker] stbir failed at mip {} of {}", mip, srcPath);
                        stbi_image_free(pixels);
                        return false;
                    }
                }
                const size_t dstOffset = outPayload.MipData.size();
                outPayload.MipData.resize(dstOffset + cur.size() * sizeof(float));
                memcpy(outPayload.MipData.data() + dstOffset, cur.data(), cur.size() * sizeof(float));
                if (outPayload.MipData.size() > kMaxTexturePayloadBytes)
                {
                    VTNA_LOG_ERROR("[TextureCooker] {} payload exceeds {} bytes", srcPath, kMaxTexturePayloadBytes);
                    stbi_image_free(pixels);
                    return false;
                }
                prev.swap(cur);
            }
            stbi_image_free(pixels);
        }
        else
        {
            uint8_t* pixels = stbi_load_from_memory(fileData.data(), fileSize, &w, &h, &comp, 4);
            if (pixels == nullptr)
            {
                VTNA_LOG_ERROR("[TextureCooker] stbi failed to decode LDR {}", srcPath);
                return false;
            }

            ETextureFormat targetFormat = ETextureFormat::Unknown;
#if VULTANA_HAS_ISPC_TEXCOMP
            switch (usage)
            {
            case ETextureUsage::BaseColor:          targetFormat = ETextureFormat::BC7SRGB; break;
            case ETextureUsage::Emissive:           targetFormat = ETextureFormat::BC7SRGB; break;
            case ETextureUsage::Diffuse:            targetFormat = ETextureFormat::BC7SRGB; break;
            case ETextureUsage::Normal:             targetFormat = ETextureFormat::BC5UNORM; break;
            case ETextureUsage::MetallicRoughness:  targetFormat = ETextureFormat::BC7UNORM; break;
            case ETextureUsage::AmbientOcclusion:   targetFormat = ETextureFormat::BC4UNORM; break;
            case ETextureUsage::SpecularGlossiness: targetFormat = ETextureFormat::BC7UNORM; break;
            default:                                targetFormat = ETextureFormat::RGBA8UNORM; break;
            }
#else
            targetFormat = bSRGB ? ETextureFormat::RGBA8SRGB : ETextureFormat::RGBA8UNORM;
#endif
            const bool bBCN = targetFormat == ETextureFormat::BC7UNORM || targetFormat == ETextureFormat::BC7SRGB ||
                              targetFormat == ETextureFormat::BC5UNORM || targetFormat == ETextureFormat::BC4UNORM;
            const uint32_t blockSize = bBCN ? 4 : 1;

            // PADDED base dims ARE the payload dims: all mips derive from them (GetTextureMipDim),
            // so ValidateTexture — which only sees the payload — computes the exact same sizes.
            const uint32_t baseW = blockSize <= 1 ? (uint32_t)w : Align4Up((uint32_t)w);
            const uint32_t baseH = blockSize <= 1 ? (uint32_t)h : Align4Up((uint32_t)h);

            outPayload.Width = baseW;
            outPayload.Height = baseH;
            outPayload.Depth = 1;
            outPayload.MipLevels = GetTextureMipLevels(baseW, baseH, blockSize);
            outPayload.ArraySize = 1;
            outPayload.Type = ETextureType::Texture2D;
            outPayload.Format = targetFormat;

            // mip0 alpha scan: BC7 alpha profiles preserve alpha; RGB profiles ignore it.
            bool bHasAlpha = false;
            if (bBCN)
            {
                const uint8_t* p = pixels;
                for (size_t i = 0; i < (size_t)w * h; ++i, p += 4)
                {
                    if (p[3] < 255)
                    {
                        bHasAlpha = true;
                        break;
                    }
                }
            }

            eastl::vector<uint8_t> prev;
            eastl::vector<uint8_t> cur;
            for (uint32_t mip = 0; mip < outPayload.MipLevels; ++mip)
            {
                const uint32_t rw = GetTextureMipDim(baseW, mip, blockSize);
                const uint32_t rh = GetTextureMipDim(baseH, mip, blockSize);
                if (mip == 0)
                {
                    if (rw == (uint32_t)w && rh == (uint32_t)h)
                    {
                        cur.assign(pixels, pixels + (size_t)w * h * 4);
                    }
                    else
                    {
                        cur.resize((size_t)rw * rh * 4);
                        EdgePadRGBA8(pixels, (uint32_t)w, (uint32_t)h, cur.data(), rw, rh);
                    }
                }
                else
                {
                    cur.resize((size_t)rw * rh * 4);
                    const int prevW = (int)GetTextureMipDim(baseW, mip - 1, blockSize);
                    const int prevH = (int)GetTextureMipDim(baseH, mip - 1, blockSize);
                    // sRGB targets filter in linear space (no dark/bright fringes on mip blends).
                    const bool ok = bSRGB
                                        ? stbir_resize_uint8_srgb(prev.data(), prevW, prevH, 0, cur.data(), (int)rw, (int)rh, 0, STBIR_RGBA) != nullptr
                                        : stbir_resize_uint8_linear(prev.data(), prevW, prevH, 0, cur.data(), (int)rw, (int)rh, 0, STBIR_RGBA) != nullptr;
                    if (!ok)
                    {
                        VTNA_LOG_ERROR("[TextureCooker] stbir failed at mip {} of {}", mip, srcPath);
                        stbi_image_free(pixels);
                        return false;
                    }
                }

#if VULTANA_HAS_ISPC_TEXCOMP
                if (bBCN)
                {
                    if (!CompressMip(targetFormat, cur.data(), rw, rh, bHasAlpha, outPayload.MipData))
                    {
                        VTNA_LOG_ERROR("[TextureCooker] compression failed at mip {} of {}", mip, srcPath);
                        stbi_image_free(pixels);
                        return false;
                    }
                }
                else
#endif
                {
                    const size_t dstOffset = outPayload.MipData.size();
                    outPayload.MipData.resize(dstOffset + cur.size());
                    memcpy(outPayload.MipData.data() + dstOffset, cur.data(), cur.size());
                }

                if (outPayload.MipData.size() > kMaxTexturePayloadBytes)
                {
                    VTNA_LOG_ERROR("[TextureCooker] {} payload exceeds {} bytes", srcPath, kMaxTexturePayloadBytes);
                    stbi_image_free(pixels);
                    return false;
                }
                prev.swap(cur);
            }
            stbi_image_free(pixels);
        }
        VTNA_LOG_INFO("[TextureCooker] cooked {} -> {}x{} mips={} format={}", srcPath, outPayload.Width, outPayload.Height, outPayload.MipLevels, (uint32_t)outPayload.Format);
        return true;
    }
}