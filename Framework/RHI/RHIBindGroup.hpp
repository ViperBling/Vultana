#pragma once

#include "Utilities/Utility.hpp"
#include "RHICommon.hpp"
#include "RHIBindGroupLayout.hpp"

namespace RHI
{
    class RHISampler;
    class RHIBufferView;
    class RHITextureView;

    struct BindGroupEntry
    {
        ResourceBinding Binding;
        union
        {
            RHISampler* Sampler;
            RHIBufferView* BufferView;
            RHITextureView* TextureView;
        };
    };

    struct BindGroupCreateInfo
    {
        RHIBindGroupLayout* Layout;
        uint32_t EntryCount;
        BindGroupEntry* Entries;
        std::string Name;
    };

    class RHIBindGroup
    {
    public:
        NOCOPY(RHIBindGroup)
        virtual ~RHIBindGroup() = default;

        virtual void Destroy() = 0;

    protected:
        explicit RHIBindGroup(const BindGroupCreateInfo& createInfo) {}
    };
}