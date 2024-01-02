#pragma once

#include <unordered_map>

#include "RHI/RHIPCH.hpp"
#include "Shader.hpp"

namespace Renderer
{
    class PipelineLayout;
    class ComputePipelineState;
    class RasterPipelineState;

    using BindingMap = std::unordered_map<std::string, std::pair<RHI::RHIShaderStageFlags, RHI::ResourceBinding>>;
    using SamplerDescriptor = RHI::SamplerCreateInfo;
    using VertexState = RHI::VertexState;
    using PrimitiveState = RHI::PrimitiveState;
    using DepthStencilState = RHI::DepthStencilState;
    using MultiSampleState = RHI::MultiSampleState;
    using FragmentState = RHI::FragmentState;

    struct BindGroupLayoutDesc
    {
        uint8_t LayoutIndex = 0;
        BindingMap Binding;
    };

    struct ComputePipelineShaderSet
    {
        size_t Hash() const;

        ShaderInstance ComputeShader;
    };

    struct RasterPipelineShaderSet
    {
        size_t Hash() const;

        ShaderInstance VertexShader;
        ShaderInstance PixelShader;
        ShaderInstance GeometryShader;
        ShaderInstance HullShader;
        ShaderInstance DomainShader;
    };

    struct ComputePipelineLayoutDesc
    {
        size_t Hash() const;

        ComputePipelineShaderSet ShaderSet;
    };

    struct RasterPipelineLayoutDesc
    {
        size_t Hash() const;

        RasterPipelineShaderSet ShaderSet;
    };

    struct ComputePipelineStateDesc
    {
        size_t Hash() const;

        ComputePipelineShaderSet ShaderSet;
    };

    struct RasterPipelineStateDesc
    {
        size_t Hash() const;

        RasterPipelineShaderSet ShaderSet;
        VertexState VertexState;
        PrimitiveState PrimitiveState;
        DepthStencilState DepthStencilState;
        MultiSampleState MultiSampleState;
        FragmentState FragmentState;
    };

    class Sampler
    {
    public:
        ~Sampler() = default;

        RHI::RHISampler* GetRHI() const { return mRHIHandle.get(); }

    private:
        friend class SamplerCache;
        Sampler(RHI::RHIDevice& inDevice, const SamplerDescriptor& inDesc);

        std::unique_ptr<RHI::RHISampler> mRHIHandle;
    };

    class BindGroupLayout
    {
    public:
        ~BindGroupLayout() = default;

        const RHI::ResourceBinding* GetBinding(const std::string& name, RHI::RHIShaderStageFlags stage) const;
        RHI::RHIBindGroupLayout* GetRHI() const { return mRHIHandle.get(); }

    private:
        friend class PipelineLayout;
        BindGroupLayout(RHI::RHIDevice& inDevice, const BindGroupLayoutDesc& inDesc);

    private:
        BindingMap mBindingMap;
        std::unique_ptr<RHI::RHIBindGroupLayout> mRHIHandle;
    };

    class PipelineLayout
    {
    public:
        ~PipelineLayout() = default;

        BindGroupLayout* GetBindGroupLayout(uint32_t index) const;
        RHI::RHIPipelineLayout* GetRHI() const { return mRHIHandle.get(); }
        size_t GetHash() const { return mHash; }
    
    private:
        struct ShaderInstancePack
        {
            const ShaderInstance* ShaderInstance;
            RHI::RHIShaderStageFlags Stage;
        };

        friend class PipelineLayoutCache;

        PipelineLayout(RHI::RHIDevice& inDevice, const ComputePipelineLayoutDesc& inDesc, size_t inHash);
        PipelineLayout(RHI::RHIDevice& inDevice, const RasterPipelineLayoutDesc& inDesc, size_t inHash);
        void CreateBindGroupLayout(RHI::RHIDevice& inDevice, const std::vector<ShaderInstancePack>& inShaderInstances);
        void CreateRHIPipelineLayout(RHI::RHIDevice& inDevice);

    private:
        size_t mHash;
        std::unordered_map<uint32_t, std::unique_ptr<BindGroupLayout>> mBindGroupLayouts;
        std::unique_ptr<RHI::RHIPipelineLayout> mRHIHandle;
    };

    class ComputePipelineState
    {
    public:
        ~ComputePipelineState() = default;

        BindGroupLayout* GetBindGroupLayout(uint32_t index) const { return mPipelineLayout->GetBindGroupLayout(index); }
        PipelineLayout* GetPipelineLayout() const { return mPipelineLayout; }
        RHI::RHIComputePipeline* GetRHI() const { return mRHIHandle.get(); }
        size_t GetHash() const { return mHash; }

    private:
        friend class PipelineCache;

        ComputePipelineState(RHI::RHIDevice& inDevice, const ComputePipelineStateDesc& inDesc, size_t inHash);

    private:
        size_t mHash;
        PipelineLayout* mPipelineLayout;
        std::unique_ptr<RHI::RHIComputePipeline> mRHIHandle;
    };

    class RasterPipelineState
    {
    public:
        ~RasterPipelineState() = default;

        PipelineLayout* GetPipelineLayout() const { return mPipelineLayout; }
        RHI::RHIGraphicsPipeline* GetRHI() const { return mRHIHandle.get(); }
        size_t GetHash() const { return mHash; }

    private:
        friend class PipelineCache;

        RasterPipelineState(RHI::RHIDevice& inDevice, const RasterPipelineStateDesc& inDesc, size_t inHash);

    private:
        size_t mHash;
        PipelineLayout* mPipelineLayout;
        std::unique_ptr<RHI::RHIGraphicsPipeline> mRHIHandle;
    };

    class SamplerCache
    {
    public:
        static SamplerCache& Get(RHI::RHIDevice& inDevice);
        ~SamplerCache() = default;

        Sampler* FindOrCreateSampler(const SamplerDescriptor& inDesc);

    private:
        explicit SamplerCache(RHI::RHIDevice& inDevice) : mDevice(inDevice) {}

    private:
        RHI::RHIDevice& mDevice;
        std::unordered_map<size_t, std::unique_ptr<Sampler>> mSamplerMap;
    };

    class PipelineCache
    {
    public:
        static PipelineCache& Get(RHI::RHIDevice& inDevice);
        ~PipelineCache() = default;

        void InValidate();
        ComputePipelineState* GetPipeline(const ComputePipelineStateDesc& inDesc);
        RasterPipelineState* GetPipeline(const RasterPipelineStateDesc& inDesc);

    private:
        explicit PipelineCache(RHI::RHIDevice& inDevice) : mDevice(inDevice) {}

    private:
        RHI::RHIDevice& mDevice;
        std::unordered_map<size_t, std::unique_ptr<ComputePipelineState>> mComputePipelineMap;
        std::unordered_map<size_t, std::unique_ptr<RasterPipelineState>> mRasterPipelineMap;
    };
}