#include "RenderGraph.hpp"

namespace Renderer
{
    static std::vector<RHI::GraphicsPassColorAttachment> GetRHIColorAttachments(const RGRasterPassDescriptor& inDesc)
    {
        std::vector<RHI::GraphicsPassColorAttachment> colorAttachments(inDesc.ColorAttachments.size());
        for (size_t i = 0; i < colorAttachments.size(); i++)
        {
            auto& dst = colorAttachments[i];
            const auto& src = inDesc.ColorAttachments[i];

            dst.TextureView = src.TextureView == nullptr ? nullptr : src.TextureView->GetRHI();
            dst.ResolveTextureView = src.ResolveTextureView == nullptr ? nullptr : src.ResolveTextureView->GetRHI();
            dst.ColorClearValue = src.ColorClearValue;
            dst.LoadOp = src.LoadOp;
            dst.StoreOp = src.StoreOp;
        }
        return colorAttachments;
    }

    static std::optional<RHI::GraphicsPassDepthStencilAttachment> GetRHIDepthStencilAttachment(const RGRasterPassDescriptor& inDesc)
    {
        if (!inDesc.DepthStencilAttachment.has_value()) { return {}; }

        const auto& src = inDesc.DepthStencilAttachment.value();
        RHI::GraphicsPassDepthStencilAttachment res;
        res.TextureView = src.TextureView == nullptr ? nullptr : src.TextureView->GetRHI();
        res.DepthClearValue = src.DepthClearValue;
        res.DepthLoadOp = src.DepthLoadOp;
        res.DepthStoreOp = src.DepthStoreOp;
        res.DepthReadOnly = src.DepthReadOnly;
        res.StencilClearValue = src.StencilClearValue;
        res.StencilLoadOp = src.StencilLoadOp;
        res.StencilStoreOp = src.StencilStoreOp;
        res.StencilReadOnly = src.StencilReadOnly;
        return res;
    }

    static RHI::GraphicsPassBeginInfo GetRHIGraphicsPassBeginInfo(
        const RGRasterPassDescriptor& inDesc
        , uint32_t colorAttachmentCount
        , const RHI::GraphicsPassColorAttachment* colorAttachments
        , const RHI::GraphicsPassDepthStencilAttachment* depthStencilAttachment)
    {
        RHI::GraphicsPassBeginInfo passBI {};
        passBI.ColorAttachmentCount = colorAttachmentCount;
        passBI.ColorAttachments = colorAttachments;
        passBI.DepthStencilAttachment = depthStencilAttachment;
        return passBI;
    }

    static RHI::RHIBarrier GetBarrier(const RGResourceTransition& transition)
    {
        if (transition.ResourceType == RHI::RHIResourceType::Buffer)
        {
            const auto& bufferTans = transition.BufferTransition;
            return RHI::RHIBarrier::Transition(bufferTans.Pointer->GetRHI(), bufferTans.Before, bufferTans.After);
        }
        if (transition.ResourceType == RHI::RHIResourceType::Texture)
        {
            const auto& textureTans = transition.TextureTransition;
            return RHI::RHIBarrier::Transition(textureTans.Pointer->GetRHI(), textureTans.Before, textureTans.After);
        }
        assert(false);
        return {};
    }

    static RHI::BufferCreateInfo GetRHIBufferCreateInfo(const RGBufferDescriptor& inDesc)
    {
        RHI::BufferCreateInfo bufferCI;
        bufferCI.Size = inDesc.Size;
        bufferCI.Usage = inDesc.Usage;
        return bufferCI;
    }

    static RHI::TextureCreateInfo GetRHITextureCreateInfo(const RGTextureDescriptor& inDesc)
    {
        RHI::TextureCreateInfo textureCI;
        textureCI.Extent = inDesc.Extent;
        textureCI.Format = inDesc.Format;
        textureCI.Usage = inDesc.Usage;
        textureCI.Samples = inDesc.Samples;
        textureCI.MipLevels = inDesc.MipLevels;
        textureCI.Dimension = inDesc.Dimension;
        return textureCI;
    }

    static RHI::BufferViewCreateInfo GetRHIBufferViewCreateInfo(const RGBufferViewDescriptor& inDesc)
    {
        RHI::BufferViewCreateInfo bufferViewCI;
        bufferViewCI.Type = inDesc.Type;
        bufferViewCI.Offset = inDesc.Offset;
        bufferViewCI.Size = inDesc.Size;
        bufferViewCI.Vertex.Stride = inDesc.Vertex.Stride;
        return bufferViewCI;
    }

    static RHI::TextureViewCreateInfo GetRHITextureViewCreateInfo(const RGTextureViewDescriptor& inDesc)
    {
        RHI::TextureViewCreateInfo textureViewCI;
        textureViewCI.Type = inDesc.Type;
        textureViewCI.Dimension = inDesc.Dimension;
        textureViewCI.TextureType = inDesc.TextureType;
        textureViewCI.BaseArrayLayer = inDesc.BaseArrayLayer;
        textureViewCI.BaseMipLevel = inDesc.BaseMipLevel;
        textureViewCI.ArrayLayerCount = inDesc.ArrayLayerCount;
        textureViewCI.MipLevelCount = inDesc.MipLevelCount;
        return textureViewCI;
    }

    static RHI::BindGroupCreateInfo GetRHIBindGroupCreateInfo(const RGBindGroupDescriptor& inDesc)
    {
    }

    RGResourceTransition RGResourceTransition::Buffer(RGBuffer *inBuffer, RHI::RHIBufferState inState, RHI::RHIBufferState outState)
    {
        RGResourceTransition resTrans;
        resTrans.ResourceType = RHI::RHIResourceType::Buffer;
        resTrans.BufferTransition.Pointer = inBuffer;
        resTrans.BufferTransition.Before = inState;
        resTrans.BufferTransition.After = outState;
        return resTrans;
    }
    RGResourceTransition RGResourceTransition::Texture(RGTexture *inTexture, RHI::RHITextureState inState, RHI::RHITextureState outState)
    {
        RGResourceTransition resTrans;
        resTrans.ResourceType = RHI::RHIResourceType::Texture;
        resTrans.TextureTransition.Pointer = inTexture;
        resTrans.TextureTransition.Before = inState;
        resTrans.TextureTransition.After = outState;
        return resTrans;
    }

    RGBindItem RGBindItem::Sampler(RGSampler *sampler)
    {
        RGBindItem res;
        res.Type = RHI::RHIBindingType::Sampler;
        res.ResSampler = sampler;
        return res;
    }

    RGBindItem RGBindItem::Texture(RGTextureView *textureView)
    {
        RGBindItem res;
        res.Type = RHI::RHIBindingType::Texture;
        res.ResTextureView = textureView;
        return res;
    }

    RGBindItem RGBindItem::StorageTexture(RGTextureView *textureView)
    {
        RGBindItem res;
        res.Type = RHI::RHIBindingType::StorageTexture;
        res.ResTextureView = textureView;
        return res;
    }
    
    RGBindItem RGBindItem::UniformBuffer(RGBufferView *bufferView)
    {
        RGBindItem res;
        res.Type = RHI::RHIBindingType::UniformBuffer;
        res.ResBufferView = bufferView;
        return res;
    }

    RGBindItem RGBindItem::StorageBuffer(RGBufferView *bufferView)
    {
        RGBindItem res;
        res.Type = RHI::RHIBindingType::StorageBuffer;
        res.ResBufferView = bufferView;
        return res;
    }

    std::pair<std::string, RGBindItem> RGBindItem::Sampler(const std::string &name, RGSampler *sampler)
    {
        RGBindItem res;
        res.Type = RHI::RHIBindingType::Sampler;
        res.ResSampler = sampler;
        return std::make_pair(std::move(name), res);
    }

    std::pair<std::string, RGBindItem> RGBindItem::Texture(const std::string &name, RGTextureView *textureView)
    {
        RGBindItem res;
        res.Type = RHI::RHIBindingType::Texture;
        res.ResTextureView = textureView;
        return std::make_pair(std::move(name), res);
    }

    std::pair<std::string, RGBindItem> RGBindItem::StorageTexture(const std::string &name, RGTextureView *textureView)
    {
        RGBindItem res;
        res.Type = RHI::RHIBindingType::StorageTexture;
        res.ResTextureView = textureView;
        return std::make_pair(std::move(name), res);
    }

    std::pair<std::string, RGBindItem> RGBindItem::UniformBuffer(const std::string &name, RGBufferView *bufferView)
    {
        RGBindItem res;
        res.Type = RHI::RHIBindingType::UniformBuffer;
        res.ResBufferView = bufferView;
        return std::make_pair(std::move(name), res);
    }

    std::pair<std::string, RGBindItem> RGBindItem::StorageBuffer(const std::string &name, RGBufferView *bufferView)
    {
        RGBindItem res;
        res.Type = RHI::RHIBindingType::StorageBuffer;
        res.ResBufferView = bufferView;
        return std::make_pair(std::move(name), res);
    }
    
    RGResource::RGResource(std::string inName, bool inIsExternal, RGResource *inParent)
        : mName(inName)
        , Parent(inParent)
        , mbIsExternal(inIsExternal)
        , mbRHIAccess(false)
    {}

    RGBuffer::RGBuffer(std::string inName, const RGBufferDescriptor &inDesc)
        : RGResource(std::move(inName), false)
        , mDescriptor(inDesc)
    {
    }

    RGBuffer::RGBuffer(std::string inName, RHI::RHIBuffer *inBuffer)
        : RGResource(std::move(inName), true)
        , mRHIHandle(inBuffer)
    {
    }

    void RGBuffer::Realize(RHI::RHIDevice &device)
    {
        if (mRHIHandle == nullptr && !IsExternal())
        {
            auto createInfo = GetRHIBufferCreateInfo(mDescriptor);
            mRHIHandle = device.CreateBuffer(createInfo);
        }
        assert(mRHIHandle);
        SetRHIAccess(true);
    }

    void RGBuffer::Destroy()
    {
        if (!IsExternal() && mRHIHandle != nullptr) 
        {
            mRHIHandle->Destroy();
        }
    }

    RGTexture::RGTexture(std::string inName, const RGTextureDescriptor &inDesc)
        : RGResource(std::move(inName), false)
        , mDescriptor(inDesc)
    {
    }

    RGTexture::RGTexture(std::string inName, RHI::RHITexture *inTexture)
        : RGResource(std::move(inName), true)
        , mRHIHandle(inTexture)
    {
    }

    void RGTexture::Realize(RHI::RHIDevice &device)
    {
        if (mRHIHandle == nullptr && !IsExternal())
        {
            auto createInfo = GetRHITextureCreateInfo(mDescriptor);
            mRHIHandle = device.CreateTexture(createInfo);
        }
        assert(mRHIHandle);
        SetRHIAccess(true);
    }

    void RGTexture::Destroy()
    {
        if (!IsExternal() && mRHIHandle != nullptr) 
        {
            mRHIHandle->Destroy();
        }
    }

    RGBufferView::RGBufferView(const std::pair<RGBuffer *, RGBufferViewDescriptor> &bufferViewDesc)
        : RGResource(bufferViewDesc.first->GetName() + "View", false, bufferViewDesc.first)
        , mBuffer(bufferViewDesc.first)
        , mDescriptor(bufferViewDesc.second)
    {
    }

    RGBufferView::RGBufferView(RGBuffer *inBuffer, const RGBufferViewDescriptor &inDesc)
        : RGResource(inBuffer->GetName() + "View", false, inBuffer)
        , mBuffer(inBuffer)
        , mDescriptor(inDesc)
    {
    }

    RGBufferView::RGBufferView(std::string inName, const std::pair<RGBuffer *, RGBufferViewDescriptor> &bufferViewDesc)
        : RGResource(std::move(inName), false, bufferViewDesc.first)
        , mBuffer(bufferViewDesc.first)
        , mDescriptor(bufferViewDesc.second)
    {
    }

    RGBufferView::RGBufferView(std::string inName, RGBuffer *inBuffer, const RGBufferViewDescriptor &inDesc)
        : RGResource(std::move(inName), false, inBuffer)
        , mBuffer(inBuffer)
        , mDescriptor(inDesc)
    {
    }

    RGBufferView::RGBufferView(std::string inName, RHI::RHIBufferView *inBufferView)
        : RGResource(std::move(inName), true)
        , mRHIHandle(inBufferView)
        , mBuffer(nullptr)
    {
    }

    void RGBufferView::Realize(RHI::RHIDevice &device)
    {
        if (mRHIHandle == nullptr && !IsExternal())
        {
            auto createInfo = GetRHIBufferViewCreateInfo(mDescriptor);
            assert(mBuffer);
            assert(mBuffer->GetRHI());
            mRHIHandle = mBuffer->GetRHI()->CreateBufferView(createInfo);
        }
        assert(mRHIHandle);
        SetRHIAccess(true);
    }

    void RGBufferView::Destroy()
    {
        if (!IsExternal() && mRHIHandle != nullptr) 
        {
            mRHIHandle->Destroy();
        }
    }

    RGTextureView::RGTextureView(const std::pair<RGTexture *, RGTextureViewDescriptor> &textureViewDesc)
        : RGResource(textureViewDesc.first->GetName() + "View", false, textureViewDesc.first)
        , mTexture(textureViewDesc.first)
        , mDescriptor(textureViewDesc.second)
    {
    }

    RGTextureView::RGTextureView(RGTexture *inTexture, const RGTextureViewDescriptor &inDesc)
        : RGResource(inTexture->GetName() + "View", false, inTexture)
        , mTexture(inTexture)
        , mDescriptor(inDesc)
    {
    }

    RGTextureView::RGTextureView(std::string inName, const std::pair<RGTexture *, RGTextureViewDescriptor> &textureViewDesc)
        : RGResource(std::move(inName), false, textureViewDesc.first)
        , mTexture(textureViewDesc.first)
        , mDescriptor(textureViewDesc.second)
    {
    }

    RGTextureView::RGTextureView(std::string inName, RGTexture *inTexture, const RGTextureViewDescriptor &inDesc)
        : RGResource(std::move(inName), false, inTexture)
        , mTexture(inTexture)
        , mDescriptor(inDesc)
    {
    }

    RGTextureView::RGTextureView(std::string inName, RHI::RHITextureView *inTextureView)
        : RGResource(std::move(inName), true)
        , mRHIHandle(inTextureView)
        , mTexture(nullptr)
    {
    }

    void RGTextureView::Realize(RHI::RHIDevice &device)
    {
        if (mRHIHandle == nullptr && !IsExternal())
        {
            auto createInfo = GetRHITextureViewCreateInfo(mDescriptor);
            assert(mTexture);
            assert(mTexture->GetRHI());
            mRHIHandle = mTexture->GetRHI()->CreateTextureView(createInfo);
        }
        assert(mRHIHandle);
        SetRHIAccess(true);
    }

    void RGTextureView::Destroy()
    {
        if (!IsExternal() && mRHIHandle != nullptr) 
        {
            mRHIHandle->Destroy();
        }
    }

    RGBindGroup::RGBindGroup(RGBindGroupDescriptor inDesc)
        : RGResource("", false, nullptr)
        , mDescriptor(std::move(inDesc))
    {
    }

    RGBindGroup::RGBindGroup(RHI::RHIBindGroup *inBindGroup)
        : RGResource("", true, nullptr)
        , mRHIHandle(inBindGroup)
    {
    }

    RGBindGroup::RGBindGroup(std::string inName, RGBindGroupDescriptor inDesc)
        : RGResource(std::move(inName), false, nullptr)
        , mDescriptor(std::move(inDesc))
    {
    }

    RGBindGroup::RGBindGroup(std::string inName, RHI::RHIBindGroup *inBindGroup)
        : RGResource(std::move(inName), true, nullptr)
        , mRHIHandle(inBindGroup)
    {
    }

    void RGBindGroup::Realize(RHI::RHIDevice &device)
    {
        // TODO: Implement
    }

    void RGBindGroup::Destroy()
    {
        if (!IsExternal() && mRHIHandle != nullptr) 
        {
            mRHIHandle->Destroy();
        }
    }

    RGFunctionCopyPass::RGFunctionCopyPass(std::string inName, RGCopyPassSetupFunction inFunc)
        : RGCopyPass(std::move(inName))
        , mSetupFunc(std::move(inFunc))
        , mExecFunc(nullptr)
    {

    }

    void RGFunctionCopyPass::Setup(RGCopyPassBuilder &builder)
    {
        mExecFunc = mSetupFunc(builder);
    }

    void RGFunctionCopyPass::Execute(RHI::RHICommandList &cmdList)
    {
        assert(mExecFunc);
        mExecFunc(cmdList);
    }

    RGFunctionComputePass::RGFunctionComputePass(std::string inName, RGComputePassSetupFunction inFunc)
        : RGComputePass(std::move(inName))
        , mSetupFunc(std::move(inFunc))
        , mExecFunc(nullptr)
    {
    }

    void RGFunctionComputePass::Setup(RGComputePassBuilder &builder)
    {
        mExecFunc = mSetupFunc(builder);
    }

    void RGFunctionComputePass::Execute(RHI::RHIComputePassCommandList &cmdList)
    {
        assert(mExecFunc);
        mExecFunc(cmdList);
    }

    RGFunctionRasterPass::RGFunctionRasterPass(std::string inName, RGRasterPassSetupFunction inFunc)
        : RGRasterPass(std::move(inName))
        , mSetupFunc(std::move(inFunc))
    {
    }

    void RGFunctionRasterPass::Setup(RGRasterPassBuilder &builder)
    {
        mExecFunc = mSetupFunc(builder);
    }

    void RGFunctionRasterPass::Execute(RHI::RHIGraphicsPassCommandList &cmdList)
    {
        assert(mExecFunc);
        mExecFunc(cmdList);
    }

    RenderGraph::RenderGraph(RHI::RHIDevice &inDevice)
        : mDevice(inDevice)
    {
    }

    void RenderGraph::AddCopyPass(RGCopyPass *inPass)
    {
        mPasses.emplace_back(inPass);
    }

    void RenderGraph::AddComputePass(RGComputePass *inPass)
    {
        mPasses.emplace_back(inPass);
    }

    void RenderGraph::AddRasterPass(RGRasterPass * inPass)
    {
        mPasses.emplace_back(inPass);
    }

    void RenderGraph::AddCopyPass(std::string inName, RGCopyPassSetupFunction inFunc)
    {
        mPasses.emplace_back(new RGFunctionCopyPass(std::move(inName), std::move(inFunc)));
    }

    void RenderGraph::AddComputePass(std::string inName, RGComputePassSetupFunction inFunc)
    {
        mPasses.emplace_back(new RGFunctionComputePass(std::move(inName), std::move(inFunc)));
    }

    void RenderGraph::AddRasterPass(std::string inName, RGRasterPassSetupFunction inFunc)
    {
        mPasses.emplace_back(new RGFunctionRasterPass(std::move(inName), std::move(inFunc)));
    }

    void RenderGraph::Setup()
    {
        for (auto& pass : mPasses)
        {
            switch (pass->GetType())
            {
            case RGPassType::Copy:
            {
                auto* copyPass = static_cast<RGCopyPass*>(pass.get());
                RGCopyPassBuilder builder(*this, *copyPass);
                copyPass->Setup(builder);
                break;
            }
            case RGPassType::Compute:
            {
                auto* computePass = static_cast<RGComputePass*>(pass.get());
                RGComputePassBuilder builder(*this, *computePass);
                computePass->Setup(builder);
                break;
            }
            case RGPassType::Raster:
            {
                auto* rasterPass = static_cast<RGRasterPass*>(pass.get());
                RGRasterPassBuilder builder(*this, *rasterPass);
                rasterPass->Setup(builder);
                break;
            }
            default:
                assert(false);
                break;
            }
        }
    }

    void RenderGraph::Compile()
    {
        for (auto& res : mResources)
        {
            res->Destroy();
            res->SetCulled(false);
        }

        std::unordered_set<RGResource*> consumeds;
        for (auto& pass : mPasses)
        {
            for (const auto& read : pass->mResRead)
            {
                consumeds.emplace(read);
            }
        }
        for (auto& res : mResources)
        {
            if (!consumeds.contains(res.get()))
            {
                res->SetCulled(true);
            }
        }
        GetResourceBarriers();
    }

    void RenderGraph::Execute(RHI::RHIFence *mainFence, RHI::RHIFence *asyncComputeFence, RHI::RHIFence *asyncCopyFence)
    {
        std::vector<RGResource*> resToRealize;
        std::vector<RGResource*> resViewToRealize;

        for (auto& res : mResources)
        {
            res->SetRHIAccess(false);

            auto type = res->GetType();
            if (type == RGResourceType::Buffer || type == RGResourceType::Texture)
            {
                resToRealize.emplace_back(res.get());
            }
            if (type == RGResourceType::BufferView || type == RGResourceType::TextureView)
            {
                resViewToRealize.emplace_back(res.get());
            }
        }

        for (auto * res : resToRealize)
        {
            if (!res->IsCulled()) { res->Realize(mDevice); }
        }
        for (auto * res : resViewToRealize)
        {
            if (!res->IsCulled()) { res->Realize(mDevice); }
        }

        assert(mDevice.GetQueueCount(RHI::RHICommandQueueType::Graphics) > 0);
        RHI::RHIQueue* mainQueue = mDevice.GetQueue(RHI::RHICommandQueueType::Graphics, 0);
        RHI::RHIQueue* asyncComputeQueue = mDevice.GetQueueCount(RHI::RHICommandQueueType::Compute) > 1 ? mDevice.GetQueue(RHI::RHICommandQueueType::Compute, 1) : mainQueue;
        RHI::RHIQueue* asyncCopyQueue = mDevice.GetQueueCount(RHI::RHICommandQueueType::Graphics) > 1 ? mDevice.GetQueue(RHI::RHICommandQueueType::Graphics, 1) : mainQueue;

        auto mainBuffer = std::unique_ptr<RHI::RHICommandBuffer>(mDevice.CreateCommandBuffer());
        auto asyncComputeBuffer = std::unique_ptr<RHI::RHICommandBuffer>(mDevice.CreateCommandBuffer());
        auto asyncCopyBuffer = std::unique_ptr<RHI::RHICommandBuffer>(mDevice.CreateCommandBuffer());
        auto mainCmdList = std::unique_ptr<RHI::RHICommandList>(mainBuffer->Begin());
        auto asyncComputeCmdList = std::unique_ptr<RHI::RHICommandList>(asyncComputeBuffer->Begin());
        auto asyncCopyCmdList = std::unique_ptr<RHI::RHICommandList>(asyncCopyBuffer->Begin());

        {
            for (auto& pass : mPasses)
            {
                switch (pass->GetType())
                {
                case RGPassType::Copy:
                {
                    auto* copyPass = static_cast<RGCopyPass*>(pass.get());
                    auto& cmdList = copyPass->mbIsAsyncCopy ? asyncCopyCmdList : mainCmdList;
                    TransitionResources(cmdList.get(), copyPass);
                    ExecuteCopyPass(cmdList.get(), copyPass);
                    break;
                }
                case RGPassType::Compute:
                {
                    auto* computePass = static_cast<RGComputePass*>(pass.get());
                    auto& cmdList = computePass->mbIsAsyncCompute ? asyncComputeCmdList : mainCmdList;
                    TransitionResources(cmdList.get(), computePass);
                    ExecuteComputePass(cmdList.get(), computePass);
                    break;
                }
                case RGPassType::Raster:
                {
                    auto* rasterPass = static_cast<RGRasterPass*>(pass.get());
                    TransitionResources(mainCmdList.get(), rasterPass);
                    ExecuteRasterPass(mainCmdList.get(), rasterPass);
                    break;
                }
                default:
                    assert(false);
                    break;
                }
            }
        }
        mainCmdList->End();
        asyncComputeCmdList->End();
        asyncCopyCmdList->End();

        mainQueue->Submit(mainBuffer.get(), mainFence);
        asyncComputeQueue->Submit(asyncComputeBuffer.get(), asyncComputeFence);
        asyncCopyQueue->Submit(asyncCopyBuffer.get(), asyncCopyFence);
    }

    void RenderGraph::ExecuteCopyPass(RHI::RHICommandList *cmdList, RGCopyPass *pass)
    {
        pass->Execute(*cmdList);
    }

    void RenderGraph::ExecuteComputePass(RHI::RHICommandList *cmdList, RGComputePass *pass)
    {
        auto computeCmdList = std::unique_ptr<RHI::RHIComputePassCommandList>(cmdList->BeginComputePass());
        {
            pass->Execute(*computeCmdList);
        }
        computeCmdList->EndPass();
    }

    void RenderGraph::ExecuteRasterPass(RHI::RHICommandList *cmdList, RGRasterPass *pass)
    {
        std::vector<RHI::GraphicsPassColorAttachment> colorAttachments = GetRHIColorAttachments(pass->mDescriptor);
        std::optional<RHI::GraphicsPassDepthStencilAttachment> depthStencilAttachment = GetRHIDepthStencilAttachment(pass->mDescriptor);
        RHI::GraphicsPassBeginInfo beginInfo = GetRHIGraphicsPassBeginInfo(
            pass->mDescriptor,
            colorAttachments.size(),
            colorAttachments.data(),
            depthStencilAttachment.has_value() ? &depthStencilAttachment.value() : nullptr
        );
        auto graphicsCmdList = std::unique_ptr<RHI::RHIGraphicsPassCommandList>(cmdList->BeginGraphicsPass(&beginInfo));
        {
            pass->Execute(*graphicsCmdList);
        }
        graphicsCmdList->EndPass();
    }

    RGBuffer *RenderGraph::GetBufferResource(RGResource *res)
    {
        if (res->GetType() == RGResourceType::Buffer)
        {
            return static_cast<RGBuffer*>(res);
        }
        if (res->GetType() == RGResourceType::BufferView)
        {
            return static_cast<RGBufferView*>(res)->GetBuffer();
        }
        assert(false);
        return nullptr;
    }

    RGTexture *RenderGraph::GetTextureResource(RGResource *res)
    {
        if (res->GetType() == RGResourceType::Texture)
        {
            return static_cast<RGTexture*>(res);
        }
        if (res->GetType() == RGResourceType::TextureView)
        {
            return static_cast<RGTextureView*>(res)->GetTexture();
        }
        assert(false);
        return nullptr;
    }

    RHI::RHIBufferState RenderGraph::GetBufferState(RGPassType passType, RGResourceAccessType access)
    {
        if (passType == RGPassType::Copy)
        {
            if (access == RGResourceAccessType::Read) { return RHI::RHIBufferState::CopySrc; }
            if (access == RGResourceAccessType::Write) { return RHI::RHIBufferState::CopyDst; }
        } 
        else
        {
            if (access == RGResourceAccessType::Read) { return RHI::RHIBufferState::ShaderReadOnly; }
            if (access == RGResourceAccessType::Write) { return RHI::RHIBufferState::Storage; }
        }
        return RHI::RHIBufferState::Undefined;
    }

    RHI::RHITextureState RenderGraph::GetTextureState(RGPassType passType, RGResourceAccessType access)
    {
        if (passType == RGPassType::Copy)
        {
            if (access == RGResourceAccessType::Read) { return RHI::RHITextureState::CopySrc; }
            if (access == RGResourceAccessType::Write) { return RHI::RHITextureState::CopyDst; }
        }
        if (passType == RGPassType::Compute)
        {
            if (access == RGResourceAccessType::Read) { return RHI::RHITextureState::ShaderReadOnly; }
            if (access == RGResourceAccessType::Write) { return RHI::RHITextureState::Storage; }
        }
        if (passType == RGPassType::Raster)
        {
            if (access == RGResourceAccessType::Read) { return RHI::RHITextureState::ShaderReadOnly; }
            if (access == RGResourceAccessType::Write) { return RHI::RHITextureState::RenderTarget; }
        }
        return RHI::RHITextureState::Undefined;
    }

    void RenderGraph::GetResourceBarriers()
    {
        LastResStates lastResState;
        for (const auto& pass : mPasses)
        {
            GetResourceTransitionByAccessGroup<RGResourceAccessType::Read>(pass.get(), pass->mResRead, lastResState);
            GetResourceTransitionByAccessGroup<RGResourceAccessType::Write>(pass.get(), pass->mResWrite, lastResState);
            UpdateLastResourceStateByAccessGroup<RGResourceAccessType::Read>(pass->GetType(), pass->mResRead, lastResState);
            UpdateLastResourceStateByAccessGroup<RGResourceAccessType::Write>(pass->GetType(), pass->mResWrite, lastResState);
        }
    }

    void RenderGraph::TransitionResources(RHI::RHICommandList *cmdList, RGPass* pass)
    {
        std::vector<RGResource*> transitionRes;
        for (auto& read : pass->mResRead)
        {
            transitionRes.emplace_back(read);
        }
        for (auto& write : pass->mResWrite)
        {
            transitionRes.emplace_back(write);
        }
        for (auto& res : transitionRes)
        {
            cmdList->ResourceBarrier(GetBarrier(mResourceTransitionMap[std::make_pair(res, pass)]));
        }
    }

    RGPassBuilder::RGPassBuilder(RenderGraph &inRenderGraph, RGPass &inPass)
        : mRenderGraph(inRenderGraph)
        , mPass(inPass)
    {
    }

    void RGPassBuilder::MarkAsConsumed(RGResource* res)
    {
        auto type = res->GetType();
        assert(type != RGResourceType::Sampler && type != RGResourceType::BindGroup);
        mPass.mResRead.emplace(res);
    }

    void RGPassBuilder::MarkDependenciesFromBindGroup(RGBindGroup* bindGroup)
    {
        const auto& items = bindGroup->GetDescriptor().Bindings;
        for (const auto& item : items)
        {
            const auto& bind = item.second;
            auto type = bind.Type;

            if (type == RHI::RHIBindingType::UniformBuffer)
            {
                mPass.mResRead.emplace(bind.ResBufferView);
            }
            else if (type == RHI::RHIBindingType::StorageBuffer)
            {
                mPass.mResWrite.emplace(bind.ResBufferView);
            }
            else if (type == RHI::RHIBindingType::Texture)
            {
                mPass.mResRead.emplace(bind.ResTextureView);
            }
            else if (type == RHI::RHIBindingType::StorageTexture)
            {
                mPass.mResWrite.emplace(bind.ResTextureView);
            }
        }
    }
}