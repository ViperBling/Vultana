#include "Pipeline.hpp"

#include <utility>

namespace Renderer
{
    class PipelineLayoutCache
    {
    public:
        static PipelineLayoutCache& Get(RHI::RHIDevice& inDevice);
        ~PipelineLayoutCache() = default;

        void InValidate();
        
        template<typename T>
        requires std::is_same_v<T, ComputePipelineLayoutDesc> || std::is_same_v<T, RasterPipelineLayoutDesc>
        PipelineLayout* GetPipelineLayout(const T& inDesc)
        {
            size_t hash = inDesc.Hash();
            auto it = mPipelineLayoutMap.find(hash);
            if (it == mPipelineLayoutMap.end())
            {
                mPipelineLayoutMap[hash] = std::unique_ptr<PipelineLayout>(new PipelineLayout(mDevice, inDesc, hash));
            }
            return mPipelineLayoutMap[hash].get();
        }

    private:
        explicit PipelineLayoutCache(RHI::RHIDevice& inDevice) : mDevice(inDevice) {}
    
    private:
        RHI::RHIDevice& mDevice;
        std::unordered_map<size_t, std::unique_ptr<PipelineLayout>> mPipelineLayoutMap;
    };

    size_t ComputePipelineShaderSet::Hash() const
    {
        std::vector<size_t> hashes = { ComputeShader.Hash() };
        return Utility::HashUtils::CityHash(hashes.data(), hashes.size() * sizeof(size_t));
    }

    size_t RasterPipelineShaderSet::Hash() const
    {
        std::vector<size_t> hashes = { 
            VertexShader.Hash(), 
            PixelShader.Hash(),
            GeometryShader.Hash(),
            HullShader.Hash(),
            DomainShader.Hash()
        };
        return Utility::HashUtils::CityHash(hashes.data(), hashes.size() * sizeof(size_t));
    }

    size_t ComputePipelineLayoutDesc::Hash() const
    {
        return ShaderSet.Hash();
    }

    size_t RasterPipelineLayoutDesc::Hash() const
    {
        return ShaderSet.Hash();
    }

    size_t ComputePipelineStateDesc::Hash() const
    {
        return ShaderSet.Hash();
    }

    size_t RasterPipelineStateDesc::Hash() const
    {
        auto computeVertexAttributeHash = [](const RHI::VertexAttribute& attribute) -> size_t
        {
            std::vector<size_t> hashes = {
                Utility::HashUtils::CityHash(&attribute.Format, sizeof(attribute.Format)),
                Utility::HashUtils::CityHash(&attribute.Offset, sizeof(attribute.Offset)),
                Utility::HashUtils::CityHash(&attribute.SemanticName, sizeof(attribute.SemanticName)),
                Utility::HashUtils::CityHash(&attribute.SemanticIndex, sizeof(attribute.SemanticIndex))
            };
            return Utility::HashUtils::CityHash(hashes.data(), hashes.size() * sizeof(size_t));
        };
        auto computeVertexBufferLayoutHash = [computeVertexAttributeHash](const RHI::VertexBufferLayout& bufferLayout) -> size_t
        {
            std::vector<size_t> hashes;
            hashes.reserve(bufferLayout.AttributeCount);
            hashes.emplace_back(Utility::HashUtils::CityHash(&bufferLayout.Stride, sizeof(bufferLayout.Stride)));
            hashes.emplace_back(Utility::HashUtils::CityHash(&bufferLayout.StepMode, sizeof(bufferLayout.StepMode)));

            for (size_t i = 0; i < bufferLayout.AttributeCount; ++i)
            {
                hashes.push_back(computeVertexAttributeHash(bufferLayout.Attributes[i]));
            }
            return Utility::HashUtils::CityHash(hashes.data(), hashes.size() * sizeof(size_t));
        };
        auto computeVertexStateHash = [computeVertexBufferLayoutHash](const RHI::VertexState& vertexState) -> size_t
        {
            std::vector<size_t> hashes;
            hashes.reserve(vertexState.BufferLayoutCount);
            for (size_t i = 0; i < vertexState.BufferLayoutCount; ++i)
            {
                hashes.push_back(computeVertexBufferLayoutHash(vertexState.BufferLayouts[i]));
            }
            return Utility::HashUtils::CityHash(hashes.data(), hashes.size() * sizeof(size_t));
        };
        auto computeFragmentStateHash = [](const RHI::FragmentState& fragmentState) -> size_t
        {
            return Utility::HashUtils::CityHash(fragmentState.ColorTargets, fragmentState.ColorTargetCount * sizeof(RHI::ColorTargetState));
        };

        std::vector<size_t> hashes = {
            ShaderSet.Hash(),
            computeVertexStateHash(VertexState),
            Utility::HashUtils::CityHash(&PrimitiveState, sizeof(RHI::PrimitiveState)),
            Utility::HashUtils::CityHash(&DepthStencilState, sizeof(RHI::DepthStencilState)),
            Utility::HashUtils::CityHash(&MultiSampleState, sizeof(RHI::MultiSampleState)),
            computeFragmentStateHash(FragmentState)
        };
        return Utility::HashUtils::CityHash(hashes.data(), hashes.size() * sizeof(size_t));
    }

    Sampler::Sampler(RHI::RHIDevice &inDevice, const SamplerDescriptor &inDesc)
    {
        mRHIHandle = std::unique_ptr<RHI::RHISampler>(inDevice.CreateSampler(inDesc));
    }

    const RHI::ResourceBinding *BindGroupLayout::GetBinding(const std::string &name, RHI::RHIShaderStageFlags stage) const
    {
        auto it = mBindingMap.find(name);
        if (it == mBindingMap.end()) { return nullptr; }

        const auto& bindingPair = it->second;
        return bindingPair.first & stage ? &bindingPair.second : nullptr;
    }

    BindGroupLayout::BindGroupLayout(RHI::RHIDevice &inDevice, const BindGroupLayoutDesc &inDesc)
        : mBindingMap(inDesc.Binding)
    {
        std::vector<RHI::BindGroupLayoutEntry> entries;
        entries.reserve(inDesc.Binding.size());
        for (const auto& binding : mBindingMap)
        {
            RHI::BindGroupLayoutEntry entry;
            entry.Binding = binding.second.second;
            entry.ShaderStage = binding.second.first;
            entries.emplace_back(entry);
        }
        RHI::BindGroupLayoutCreateInfo bindGroupCI {};
        bindGroupCI.Entries = entries.data();
        bindGroupCI.EntryCount = entries.size();
        bindGroupCI.LayoutIndex = inDesc.LayoutIndex;
        mRHIHandle = std::unique_ptr<RHI::RHIBindGroupLayout>(inDevice.CreateBindGroupLayout(bindGroupCI));
    }

    BindGroupLayout* PipelineLayout::GetBindGroupLayout(uint32_t index) const
    {
        auto it = mBindGroupLayouts.find(index);
        return it == mBindGroupLayouts.end() ? nullptr : it->second.get();
    }

    PipelineLayout::PipelineLayout(RHI::RHIDevice &inDevice, const ComputePipelineLayoutDesc &inDesc, size_t inHash)
        : mHash(inHash)
    {
        std::vector<ShaderInstancePack> shaderInstances = { 
            { &inDesc.ShaderSet.ComputeShader, RHI::RHIShaderStageBits::Compute } 
        };
        CreateBindGroupLayout(inDevice, shaderInstances);
        CreateRHIPipelineLayout(inDevice);
    }

    PipelineLayout::PipelineLayout(RHI::RHIDevice &inDevice, const RasterPipelineLayoutDesc &inDesc, size_t inHash)
        : mHash(inHash)
    {
        std::vector<ShaderInstancePack> shaderInstances = {
            { &inDesc.ShaderSet.VertexShader, RHI::RHIShaderStageBits::Vertex },
            { &inDesc.ShaderSet.PixelShader, RHI::RHIShaderStageBits::Pixel },
            { &inDesc.ShaderSet.GeometryShader, RHI::RHIShaderStageBits::Geometry },
            { &inDesc.ShaderSet.DomainShader, RHI::RHIShaderStageBits::Domain },
            { &inDesc.ShaderSet.HullShader, RHI::RHIShaderStageBits::Hull }
        };
        CreateBindGroupLayout(inDevice, shaderInstances);
        CreateRHIPipelineLayout(inDevice);
    }

    void PipelineLayout::CreateBindGroupLayout(RHI::RHIDevice &inDevice, const std::vector<ShaderInstancePack> &inShaderInstances)
    {
        std::unordered_map<uint8_t, BindingMap> layoutBindingMaps;

        for (const auto& si : inShaderInstances)
        {
            const auto & resourceBindings = si.ShaderInstance->ReflectionData.ResourceBindings;

            for (const auto& resBinding : resourceBindings)
            {
                auto layoutIndex = resBinding.second.first;
                const auto& name = resBinding.first;
                const auto& binding = resBinding.second.second;

                if (!layoutBindingMaps.contains(layoutIndex)) { layoutBindingMaps[layoutIndex] = {}; }
                auto& layoutBindingMap = layoutBindingMaps[layoutIndex];
                layoutBindingMap[name] = std::make_pair(si.Stage, binding);
            }
        }
        for (const auto& lb : layoutBindingMaps)
        {
            BindGroupLayoutDesc desc;
            desc.Binding = lb.second;
            desc.LayoutIndex = lb.first;
            mBindGroupLayouts[lb.first] = std::unique_ptr<BindGroupLayout>(new BindGroupLayout(inDevice, desc));
        }
    }

    void PipelineLayout::CreateRHIPipelineLayout(RHI::RHIDevice &inDevice)
    {
        std::vector<BindGroupLayout*> bindGroupLayouts;
        bindGroupLayouts.reserve(mBindGroupLayouts.size());
        for (const auto& bgl : mBindGroupLayouts)
        {
            bindGroupLayouts.emplace_back(bgl.second.get());
        }

        RHI::PipelineLayoutCreateInfo pipelineLayoutCI {};
        pipelineLayoutCI.BindGroupLayouts = reinterpret_cast<const RHI::RHIBindGroupLayout* const*>(bindGroupLayouts.data());
        pipelineLayoutCI.BindGroupLayoutCount = bindGroupLayouts.size();
        pipelineLayoutCI.PipelineConstantLayouts = nullptr;

        mRHIHandle = std::unique_ptr<RHI::RHIPipelineLayout>(inDevice.CreatePipelineLayout(pipelineLayoutCI));
    }

    ComputePipelineState::ComputePipelineState(RHI::RHIDevice &inDevice, const ComputePipelineStateDesc &inDesc, size_t inHash)
        : mHash(inHash)
    {
        ComputePipelineLayoutDesc layoutDesc = { inDesc.ShaderSet };
        mPipelineLayout = PipelineLayoutCache::Get(inDevice).GetPipelineLayout(layoutDesc);

        RHI::ComputePipelineCreateInfo pipelineCI {};
        pipelineCI.PipelineLayout = mPipelineLayout->GetRHI();
        pipelineCI.Shader = inDesc.ShaderSet.ComputeShader.ShaderHandle;
        mRHIHandle = std::unique_ptr<RHI::RHIComputePipeline>(inDevice.CreateComputePipeline(pipelineCI));
    }

    RasterPipelineState::RasterPipelineState(RHI::RHIDevice &inDevice, const RasterPipelineStateDesc &inDesc, size_t inHash)
    {
        RasterPipelineLayoutDesc layoutDesc = { inDesc.ShaderSet };
        mPipelineLayout = PipelineLayoutCache::Get(inDevice).GetPipelineLayout(layoutDesc);

        RHI::GraphicsPipelineCreateInfo pipelineCI {};
        pipelineCI.PipelineLayout = mPipelineLayout->GetRHI();
        pipelineCI.VertexShader = inDesc.ShaderSet.VertexShader.ShaderHandle;
        pipelineCI.FragmentShader = inDesc.ShaderSet.PixelShader.ShaderHandle;
        pipelineCI.GeometryShader = inDesc.ShaderSet.GeometryShader.ShaderHandle;
        pipelineCI.DomainShader = inDesc.ShaderSet.DomainShader.ShaderHandle;
        pipelineCI.HullShader = inDesc.ShaderSet.HullShader.ShaderHandle;
        pipelineCI.DepthStencilState = inDesc.DepthStencilState;
        pipelineCI.PrimitiveState = inDesc.PrimitiveState;
        pipelineCI.VertexState = inDesc.VertexState;
        pipelineCI.MultiSampleState = inDesc.MultiSampleState;
        pipelineCI.FragState = inDesc.FragmentState;
        mRHIHandle = std::unique_ptr<RHI::RHIGraphicsPipeline>(inDevice.CreateGraphicsPipeline(pipelineCI));
    }

    SamplerCache &SamplerCache::Get(RHI::RHIDevice& inDevice)
    {
        static std::unordered_map<RHI::RHIDevice*, std::unique_ptr<SamplerCache>> sCacheMap;

        auto it = sCacheMap.find(&inDevice);
        if (it == sCacheMap.end())
        {
            sCacheMap[&inDevice] = std::unique_ptr<SamplerCache>(new SamplerCache(inDevice));
        }
        return *sCacheMap[&inDevice];
    }

    Sampler *SamplerCache::FindOrCreateSampler(const SamplerDescriptor &inDesc)
    {
        size_t hash = Utility::HashUtils::CityHash(&inDesc, sizeof(SamplerDescriptor));
        auto it = mSamplerMap.find(hash);
        if (it == mSamplerMap.end())
        {
            mSamplerMap[hash] = std::unique_ptr<Sampler>(new Sampler(mDevice, inDesc));
        }
        return mSamplerMap[hash].get();
    }

    PipelineCache& PipelineCache::Get(RHI::RHIDevice& inDevice)
    {
        static std::unordered_map<RHI::RHIDevice*, std::unique_ptr<PipelineCache>> sCacheMap;

        auto it = sCacheMap.find(&inDevice);
        if (it == sCacheMap.end())
        {
            sCacheMap[&inDevice] = std::unique_ptr<PipelineCache>(new PipelineCache(inDevice));
        }
        return *sCacheMap[&inDevice];
    }

    void PipelineCache::InValidate()
    {
        mComputePipelineMap.clear();
        mRasterPipelineMap.clear();
    }

    ComputePipelineState *PipelineCache::GetPipeline(const ComputePipelineStateDesc &inDesc)
    {
        auto hash = inDesc.Hash();
        auto it = mComputePipelineMap.find(hash);
        if (it == mComputePipelineMap.end())
        {
            mComputePipelineMap[hash] = std::unique_ptr<ComputePipelineState>(new ComputePipelineState(mDevice, inDesc, hash));
        }
        return mComputePipelineMap[hash].get();
    }
    
    RasterPipelineState *PipelineCache::GetPipeline(const RasterPipelineStateDesc &inDesc)
    {
        auto hash = inDesc.Hash();
        auto it = mRasterPipelineMap.find(hash);
        if (it == mRasterPipelineMap.end())
        {
            mRasterPipelineMap[hash] = std::unique_ptr<RasterPipelineState>(new RasterPipelineState(mDevice, inDesc, hash));
        }
        return mRasterPipelineMap[hash].get();
    }
    
    PipelineLayoutCache &PipelineLayoutCache::Get(RHI::RHIDevice &inDevice)
    {
        static std::unordered_map<RHI::RHIDevice*, std::unique_ptr<PipelineLayoutCache>> sCacheMap;

        auto it = sCacheMap.find(&inDevice);
        if (it == sCacheMap.end())
        {
            sCacheMap[&inDevice] = std::unique_ptr<PipelineLayoutCache>(new PipelineLayoutCache(inDevice));
        }
        return *sCacheMap[&inDevice];
    }

    void PipelineLayoutCache::InValidate()
    {
        mPipelineLayoutMap.clear();
    }
}