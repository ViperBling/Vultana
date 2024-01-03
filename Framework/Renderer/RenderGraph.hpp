#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <optional>

#include "Utilities/Hash.hpp"
#include "RHI/RHIPCH.hpp"
#include "Pipeline.hpp"

namespace Renderer
{
    class RGResource;
    class RGBuffer;
    class RGBufferView;
    class RGTexture;
    class RGTextureView;
    class RGSampler;
    class RGPass;
    class RGPassBuilder;
    class RGCopyPassBuilder;
    class RGComputePassBuilder;
    class RGRasterPassBuilder;

    enum class RGResourceType
    {
        Buffer,
        BufferView,
        Texture,
        TextureView,
        Sampler,
        BindGroup,
        Count
    };

    enum class RGPassType
    {
        Copy,
        Compute,
        Raster,
        Count
    };

    enum class RGResourceAccessType
    {
        Read,
        Write,
        Count
    };

    struct RGBufferTransition : public RHI::BufferTransitionBase
    {
        RGBuffer* Pointer;
    };

    struct RGTextureTransition : public RHI::TextureTransitionBase
    {
        RGTexture* Pointer;
    };

    struct RGResourceTransition
    {
        RHI::RHIResourceType ResourceType;
        union
        {
            RGBufferTransition BufferTransition;
            RGTextureTransition TextureTransition;
        };

        static RGResourceTransition Buffer(RGBuffer* inBuffer, RHI::RHIBufferState inState, RHI::RHIBufferState outState);
        static RGResourceTransition Texture(RGTexture* inTexture, RHI::RHITextureState inState, RHI::RHITextureState outState);
    };

    struct RGBufferDescriptor : public RHI::BufferCreateInfo {};
    struct RGTextureDescriptor : public RHI::TextureCreateInfo {};
    struct RGBufferViewDescriptor : public RHI::BufferViewCreateInfo {};
    struct RGTextureViewDescriptor : public RHI::TextureViewCreateInfo {};

    struct RGBindItem
    {
        static RGBindItem Sampler(RGSampler* sampler);
        static RGBindItem Texture(RGTextureView* textureView);
        static RGBindItem StorageTexture(RGTextureView* textureView);
        static RGBindItem UniformBuffer(RGBufferView* bufferView);
        static RGBindItem StorageBuffer(RGBufferView* bufferView);

        static std::pair<std::string, RGBindItem> Sampler(const std::string& name, RGSampler* sampler);
        static std::pair<std::string, RGBindItem> Texture(const std::string& name, RGTextureView* textureView);
        static std::pair<std::string, RGBindItem> StorageTexture(const std::string& name, RGTextureView* textureView);
        static std::pair<std::string, RGBindItem> UniformBuffer(const std::string& name, RGBufferView* bufferView);
        static std::pair<std::string, RGBindItem> StorageBuffer(const std::string& name, RGBufferView* bufferView);

        RHI::RHIBindingType Type;
        union 
        {
            RGSampler* ResSampler;
            RGBufferView* ResBufferView;
            RGTextureView* ResTextureView;
        };
    };

    struct RGBindGroupDescriptor
    {
        template <typename... Args>
        static RGBindGroupDescriptor Create(BindGroupLayout* layout, Args&&... args)
        {
            RGBindGroupDescriptor desc;
            desc.Layout = layout->GetRHI();
            desc.Bindings.reserve(sizeof...(args));
            (void) std::initializer_list<int> 
            {
                ([&]() -> void
                {
                    desc.Bindings.emplace_back(std::forward<Args>(args));
                }(), 0)...
            };
            return desc;
        }

        RHI::RHIBindGroupLayout* Layout;
        std::vector<std::pair<std::string, RGBindItem>> Bindings;
    };

    struct RGColorAttachment : public RHI::GraphicsPassColorAttachmentBase
    {
        RGTextureView* TextureView = nullptr;
        RGTextureView* ResolveTextureView = nullptr;
    };

    struct RGDepthStencilAttachment : public RHI::GraphicsPassDepthStencilAttachmentBase
    {
        RGTextureView* TextureView = nullptr;
    };

    struct RGRasterPassDescriptor
    {
        std::vector<RGColorAttachment> ColorAttachments;
        std::optional<RGDepthStencilAttachment> DepthStencilAttachment;
    };

    class RGResource
    {
    public:
        virtual ~RGResource() = default;

        bool IsExternal() const { return mbIsExternal; }
        bool IsCulled() const { return mbIsCulled; }
        bool CanAccessRHI() const { return mbRHIAccess; }
        const std::string& GetName() const { return mName; }
        RGResource* GetParent() const { return Parent; }

        virtual RGResourceType GetType() = 0;

    protected:
        RGResource(std::string inName, bool inIsExternal, RGResource* inParent = nullptr);

        virtual void Realize(RHI::RHIDevice& device) = 0;
        virtual void Destroy() = 0;

        void SetRHIAccess(bool inAccess) { mbRHIAccess = inAccess; }

    private:
        friend class RenderGraph;
        void SetCulled(bool inCulled) { mbIsCulled = inCulled; }
    
    private:
        std::string mName;
        RGResource* Parent;
        bool mbIsExternal;
        bool mbRHIAccess;
        bool mbIsCulled;
    };

    class RGBuffer : public RGResource
    {
    public:
        RGBuffer(std::string inName, const RGBufferDescriptor& inDesc);
        RGBuffer(std::string inName, RHI::RHIBuffer* inBuffer);
        ~RGBuffer() = default;

        RGResourceType GetType() override { return RGResourceType::Buffer; }
        void Realize(RHI::RHIDevice& device) override;
        void Destroy() override;

        RHI::RHIBuffer* GetRHI() const { return mRHIHandle; }
        const RGBufferDescriptor& GetDescriptor() const { return mDescriptor; }

        template <typename T>
        void UploadData(const T& data)
        {
            assert((mDescriptor.Usage & RHI::RHIBufferUsageBits::Uniform) != 0 && CanAccessRHI() && sizeof(T) == mDescriptor.Size);
            void* mapResult = mRHIHandle->Map(RHI::RHIMapMode::Write, 0, mDescriptor.Size);
            memcpy(mapResult, &data, mDescriptor.Size);
            mRHIHandle->Unmap();
        }

    private:
        RGBufferDescriptor mDescriptor;
        RHI::RHIBuffer* mRHIHandle = nullptr;
    };

    class RGTexture : public RGResource
    {
    public:
        RGTexture(std::string inName, const RGTextureDescriptor& inDesc);
        RGTexture(std::string inName, RHI::RHITexture* inTexture);
        ~RGTexture() = default;

        RGResourceType GetType() override { return RGResourceType::Texture; }
        void Realize(RHI::RHIDevice& device) override;
        void Destroy() override;

        RHI::RHITexture* GetRHI() const { return mRHIHandle; }
        const RGTextureDescriptor& GetDescriptor() const { return mDescriptor; }

    private:
        RGTextureDescriptor mDescriptor;
        RHI::RHITexture* mRHIHandle = nullptr;
    };

    class RGBufferView : public RGResource
    {
    public:
        explicit RGBufferView(const std::pair<RGBuffer*, RGBufferViewDescriptor>& bufferViewDesc);
        RGBufferView(RGBuffer* inBuffer, const RGBufferViewDescriptor& inDesc);
        RGBufferView(std::string inName, const std::pair<RGBuffer*, RGBufferViewDescriptor>& bufferViewDesc);
        RGBufferView(std::string inName, RGBuffer* inBuffer, const RGBufferViewDescriptor& inDesc);
        RGBufferView(std::string inName, RHI::RHIBufferView* inBufferView);
        ~RGBufferView() = default;

        RGResourceType GetType() override { return RGResourceType::BufferView; }
        void Realize(RHI::RHIDevice& device) override;
        void Destroy() override;

        RHI::RHIBufferView* GetRHI() const { return mRHIHandle; }
        RGBuffer* GetBuffer() const { return mBuffer; }
        const RGBufferViewDescriptor& GetDescriptor() const { return mDescriptor; }

    private:
        RGBufferViewDescriptor mDescriptor;
        RGBuffer* mBuffer;
        RHI::RHIBufferView* mRHIHandle = nullptr;
    };

    class RGTextureView : public RGResource
    {
    public:
        explicit RGTextureView(const std::pair<RGTexture*, RGTextureViewDescriptor>& textureViewDesc);
        RGTextureView(RGTexture* inTexture, const RGTextureViewDescriptor& inDesc);
        RGTextureView(std::string inName, const std::pair<RGTexture*, RGTextureViewDescriptor>& textureViewDesc);
        RGTextureView(std::string inName, RGTexture* inTexture, const RGTextureViewDescriptor& inDesc);
        RGTextureView(std::string inName, RHI::RHITextureView* inTextureView);
        ~RGTextureView() = default;

        RGResourceType GetType() override { return RGResourceType::TextureView; }
        void Realize(RHI::RHIDevice& device) override;
        void Destroy() override;

        RHI::RHITextureView* GetRHI() const { return mRHIHandle; }
        RGTexture* GetTexture() const { return mTexture; }
        const RGTextureViewDescriptor& GetDescriptor() const { return mDescriptor; }

    private:
        RGTextureViewDescriptor mDescriptor;
        RGTexture* mTexture;
        RHI::RHITextureView* mRHIHandle = nullptr;
    };

    class RGBindGroup : public RGResource
    {
    public:
        explicit RGBindGroup(RGBindGroupDescriptor inDesc);
        explicit RGBindGroup(RHI::RHIBindGroup* inBindGroup);
        RGBindGroup(std::string inName, RGBindGroupDescriptor inDesc);
        RGBindGroup(std::string inName, RHI::RHIBindGroup* inBindGroup);
        ~RGBindGroup() = default;

        RGResourceType GetType() override { return RGResourceType::BindGroup; }
        void Realize(RHI::RHIDevice& device) override;
        void Destroy() override;
        RHI::RHIBindGroup* GetRHI() const { return mRHIHandle; }
        const RGBindGroupDescriptor& GetDescriptor() const { return mDescriptor; }

    private:
        RGBindGroupDescriptor mDescriptor;
        RHI::RHIBindGroup* mRHIHandle = nullptr;
    };

    class RGPass
    {
    public:
        virtual ~RGPass() = default;

    protected:
        explicit RGPass(std::string inName) : mName(std::move(inName)) {}
        virtual RGPassType GetType() = 0;

    private:
        friend class RenderGraph;
        friend class RGPassBuilder;

        std::string mName;
        std::unordered_set<RGResource*> mResRead;
        std::unordered_set<RGResource*> mResWrite;
    };

    class RGCopyPass : public RGPass
    {
    public:
        ~RGCopyPass() = default;

    protected:
        explicit RGCopyPass(std::string inName) : RGPass(std::move(inName)) {}

        RGPassType GetType() override { return RGPassType::Copy; }
        virtual void Setup(RGCopyPassBuilder& builder) = 0;
        virtual void Execute(RHI::RHICommandList& cmdList) = 0;
    
    private:
        friend class RenderGraph;
        friend class RGCopyPassBuilder;

        bool mbIsAsyncCopy;
    };

    class RGComputePass : public RGPass
    {
    public:
        ~RGComputePass() = default;

    protected:
        explicit RGComputePass(std::string inName) : RGPass(std::move(inName)) {}

        RGPassType GetType() override { return RGPassType::Compute; }
        virtual void Setup(RGComputePassBuilder& builder) = 0;
        virtual void Execute(RHI::RHIComputePassCommandList& cmdList) = 0;
    
    private:
        friend class RenderGraph;
        friend class RGComputePassBuilder;

        bool mbIsAsyncCompute;
    };

    class RGRasterPass : public RGPass
    {
    public:
        ~RGRasterPass() = default;

    protected:
        explicit RGRasterPass(std::string inName) : RGPass(std::move(inName)) {}

        RGPassType GetType() override { return RGPassType::Raster; }
        virtual void Setup(RGRasterPassBuilder& builder) = 0;
        virtual void Execute(RHI::RHIGraphicsPassCommandList& cmdList) = 0;
    
    private:
        friend class RenderGraph;
        friend class RGRasterPassBuilder;

        RGRasterPassDescriptor mDescriptor;
    };

    using RGCopyPassExecuteFunction = std::function<void(RHI::RHICommandList&)>;
    using RGCopyPassSetupFunction = std::function<RGCopyPassExecuteFunction(RGCopyPassBuilder&)>;
    using RGComputePassExecuteFunction = std::function<void(RHI::RHIComputePassCommandList&)>;
    using RGComputePassSetupFunction = std::function<RGComputePassExecuteFunction(RGComputePassBuilder&)>;
    using RGRasterPassExecuteFunction = std::function<void(RHI::RHIGraphicsPassCommandList&)>;
    using RGRasterPassSetupFunction = std::function<RGRasterPassExecuteFunction(RGRasterPassBuilder&)>;

    class RGFunctionCopyPass : public RGCopyPass
    {
    public:
        RGFunctionCopyPass(std::string inName, RGCopyPassSetupFunction inFunc);
        ~RGFunctionCopyPass() = default;

        void Setup(RGCopyPassBuilder& builder) override;
        void Execute(RHI::RHICommandList& cmdList) override;

    private:
        friend class RGCopyPassBuilder;

        RGCopyPassSetupFunction mSetupFunc;
        RGCopyPassExecuteFunction mExecFunc;
    };

    class RGFunctionComputePass : public RGComputePass
    {
    public:
        RGFunctionComputePass(std::string inName, RGComputePassSetupFunction inFunc);
        ~RGFunctionComputePass() = default;

        void Setup(RGComputePassBuilder& builder) override;
        void Execute(RHI::RHIComputePassCommandList& cmdList) override;

    private:
        RGComputePassSetupFunction mSetupFunc;
        RGComputePassExecuteFunction mExecFunc;
    };

    class RGFunctionRasterPass : public RGRasterPass
    {
    public:
        RGFunctionRasterPass(std::string inName, RGRasterPassSetupFunction inFunc);
        ~RGFunctionRasterPass() = default;

        void Setup(RGRasterPassBuilder& builder) override;
        void Execute(RHI::RHIGraphicsPassCommandList& cmdList) override;

    private:
        RGRasterPassSetupFunction mSetupFunc;
        RGRasterPassExecuteFunction mExecFunc;
    };

    class RenderGraph
    {
    public:
        explicit RenderGraph(RHI::RHIDevice& inDevice);
        ~RenderGraph() = default;

        void AddCopyPass(RGCopyPass* inPass);
        void AddComputePass(RGComputePass* inPass);
        void AddRasterPass(RGRasterPass* inPass);
        void AddCopyPass(std::string inName, RGCopyPassSetupFunction inFunc);
        void AddComputePass(std::string inName, RGComputePassSetupFunction inFunc);
        void AddRasterPass(std::string inName, RGRasterPassSetupFunction inFunc);

        RHI::RHIDevice& GetDevice() { return mDevice; }
        void Setup();
        void Compile();
        void Execute(RHI::RHIFence* mainFence, RHI::RHIFence* asyncComputeFence = nullptr, RHI::RHIFence* asyncCopyFence = nullptr);
    
    private:
        using LastResStates = std::unordered_map<RGResource*, std::pair<RGPassType, RGResourceAccessType>>;

        struct ResourcePassPtrPairHash
        {
            size_t operator()(const std::pair<RGResource*, RGPass*>& pair) const
            {
                return Utility::HashUtils::CityHash(&pair, sizeof(pair));
            }
        };

        friend class RGPassBuilder;

        static void ExecuteCopyPass(RHI::RHICommandList* cmdList, RGCopyPass* pass);
        static void ExecuteComputePass(RHI::RHICommandList* cmdList, RGComputePass* pass);
        static void ExecuteRasterPass(RHI::RHICommandList* cmdList, RGRasterPass* pass);
        static RGBuffer* GetBufferResource(RGResource* res);
        static RGTexture* GetTextureResource(RGResource* res);
        static RHI::RHIBufferState GetBufferState(RGPassType passType, RGResourceAccessType access);
        static RHI::RHITextureState GetTextureState(RGPassType passType, RGResourceAccessType access);

        void GetResourceBarriers();
        void TransitionResources(RHI::RHICommandList* cmdList, RGPass* pass);

        template <RGResourceAccessType AT>
        void GetResourceTransitionByAccessGroup(RGPass* pass, const std::unordered_set<RGResource*>& resAccessGroup, LastResStates& lastResState);

        template <RGResourceAccessType AT>
        void UpdateLastResourceStateByAccessGroup(RGPassType passType, const std::unordered_set<RGResource*>& resAccessGroup, LastResStates& lastResState);

    private:
        RHI::RHIDevice& mDevice;
        std::vector<std::unique_ptr<RGResource>> mResources;
        std::vector<std::unique_ptr<RGPass>> mPasses;
        std::unordered_map<std::pair<RGResource*, RGPass*>, RGResourceTransition, ResourcePassPtrPairHash> mResourceTransitionMap;
    };

    class RGPassBuilder
    {
    public:
        RGPassBuilder(RenderGraph& inGraph, RGPass& inPass);
        virtual ~RGPassBuilder() = default;

        template <typename... Args>
        RGBuffer* CreateBuffer(Args&&... args)
        {
            return Create<RGBuffer>(std::forward<Args>(args)...);
        }

        template <typename... Args>
        RGBufferView* CreateBufferView(Args&&... args)
        {
            return Create<RGBufferView>(std::forward<Args>(args)...);
        }

        template <typename... Args>
        RGTexture* CreateTexture(Args&&... args)
        {
            return Create<RGTexture>(std::forward<Args>(args)...);
        }

        template <typename... Args>
        RGTextureView* CreateTextureView(Args&&... args)
        {
            return Create<RGTextureView>(std::forward<Args>(args)...);
        }
        
        template <typename... Args>
        RGSampler* CreateSampler(Args&&... args)
        {
            return Create<RGSampler>(std::forward<Args>(args)...);
        }

        template <typename... Args>
        RGBindGroup* AllocateBindGroup(Args&&... args)
        {
            RGBindGroup* bindGroup = Create<RGBindGroup>(std::forward<Args>(args)...);
            MarkDependenciesFromBindGroup(bindGroup);
            return bindGroup;
        }

        void MarkAsConsumed(RGResource* res);
        RHI::RHIDevice& GetDevice() { return mRenderGraph.GetDevice(); }

    protected:
        template <typename R, typename... Args>
        R* Create(Args&&... args)
        {
            mRenderGraph.mResources.emplace_back(new R(std::forward<Args>(args)...));
            return static_cast<R*>(mRenderGraph.mResources.back().get());
        }

    protected:
        RenderGraph& mRenderGraph;
        RGPass& mPass;

    private:
        void MarkDependenciesFromBindGroup(RGBindGroup* bindGroup);
    };

    class RGCopyPassBuilder : public RGPassBuilder
    {
    public:
        ~RGCopyPassBuilder() = default;
        void SetAsyncCopy(bool isAsyncCopy) { mCopyPass.mbIsAsyncCopy = isAsyncCopy; }
    
    private:
        friend class RenderGraph;
        RGCopyPassBuilder(RenderGraph& inGraph, RGCopyPass& inPass) 
            : RGPassBuilder(inGraph, inPass)
            , mCopyPass(inPass)
        {}
    
    private:
        RGCopyPass& mCopyPass;
    };

    class RGComputePassBuilder : public RGPassBuilder
    {
    public:
        ~RGComputePassBuilder() = default;
        void SetAsyncCompute(bool isAsyncCompute) { mComputePass.mbIsAsyncCompute = isAsyncCompute; }

    private:
        friend class RenderGraph;
        RGComputePassBuilder(RenderGraph& inGraph, RGComputePass& inPass)
            : RGPassBuilder(inGraph, inPass)
            , mComputePass(inPass)
        {}
    
    private:
        RGComputePass& mComputePass;
    };

    class RGRasterPassBuilder : public RGPassBuilder
    {
    public:
        ~RGRasterPassBuilder() = default;
        void SetPassDescriptor(const RGRasterPassDescriptor& inDesc) { mRasterPass.mDescriptor = inDesc; }

    private:
        friend class RenderGraph;
        RGRasterPassBuilder(RenderGraph& inGraph, RGRasterPass& inPass)
            : RGPassBuilder(inGraph, inPass)
            , mRasterPass(inPass)
        {}

    private:
        RGRasterPass& mRasterPass;
    };

    // Some Impementation
    template <RGResourceAccessType AT>
    inline void RenderGraph::GetResourceTransitionByAccessGroup(RGPass* pass, const std::unordered_set<RGResource*>& resAccessGroup, LastResStates& lastResState)
    {
        for (auto& res : resAccessGroup)
        {
            if (res->mbIsCulled) continue;

            const auto resType = res->GetType();
            const bool isBuffer = resType == RGResourceType::Buffer || resType == RGResourceType::BufferView;
            const bool isTexture = resType == RGResourceType::Texture || resType == RGResourceType::TextureView;

            auto it = lastResState.find(res);
            if (isBuffer)
            {
                auto before = it == lastResState.end() ? 
                    GetBufferState(RGPassType::Count, RGResourceAccessType::Count) :
                    GetBufferState(it->second.first, it->second.second);
                auto after = GetBufferState(pass->GetType(), AT);
                mResourceTransitionMap[std::make_pair(res, pass)] = RGResourceTransition::Buffer(GetBufferResource(res), before, after);
            }
            else if (isTexture)
            {
                auto before = it == lastResState.end() ? 
                    GetTextureState(RGPassType::Count, RGResourceAccessType::Count) :
                    GetTextureState(it->second.first, it->second.second);
                auto after = GetTextureState(pass->GetType(), AT);
                mResourceTransitionMap[std::make_pair(res, pass)] = RGResourceTransition::Texture(GetTextureResource(res), before, after);
            }
        }
    }

    template <RGResourceAccessType AT>
    inline void RenderGraph::UpdateLastResourceStateByAccessGroup(RGPassType passType, const std::unordered_set<RGResource *>& resAccessGroup, LastResStates& lastResState)
    {
        for (auto& res : resAccessGroup)
        {
            lastResState[res] = std::make_pair(passType, AT);
        }
    }
}