#pragma once

#include "Utilities/Utility.hpp"
#include "RHICommon.hpp"
#include "RHIBindGroupLayout.hpp"

#include <cstddef>
#include <variant>
#include <utility>

namespace RHI
{
    class RHISampler;
    class RHIBufferView;
    class RHITextureView;

    struct BindGroupEntry
    {
        ResourceBinding Binding;
        std::variant<RHISampler*, RHIBufferView*, RHITextureView*> Resource;

        BindGroupEntry(const ResourceBinding& inBinding, const std::variant<RHISampler*, RHIBufferView*, RHITextureView*>& inResource)
            : Binding(inBinding)
            , Resource(inResource) 
        {}
    };

    struct BindGroupCreateInfo
    {
        RHIBindGroupLayout* Layout;
        std::vector<BindGroupEntry> Entries;
        std::string Name;

        explicit BindGroupCreateInfo(RHIBindGroupLayout* inLayout, std::string inName = "")
            : Layout(inLayout)
            , Name(std::move(inName)) 
        {}
        BindGroupCreateInfo& AddEntry(const BindGroupEntry& entry)
        {
            Entries.emplace_back(entry);
            return *this;
        }
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