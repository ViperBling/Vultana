#pragma once

#include "RHI/RHIPCH.hpp"
#include "Utilities/Utility.hpp"

#include <iostream>
#include <deque>
#include <memory>
#include <functional>

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace Vultana
{
    struct RendererCreateInfo
    {
        RHIDeviceType DeviceType = RHIDeviceType::Hardware;
        const char* ApplicationName = "Vultana";
        uint32_t Width = 1280;
        uint32_t Height = 720;
        bool bEnableValidationLayers = true;
    };

    enum class ImageTransitionMode
    {
        ToAttachment,
        ToGeneral,
        GeneralToPresent,
        AttachmentToPresent
    };

    struct AllocatedImage
    {
        vk::Image Image;
        VmaAllocation ImageAllocation;
    };

    struct DeletionQueue
    {
        std::deque<std::function<void()>> deletors;

        void PushFunction(std::function<void()>&& function)
        {
            deletors.push_back(function);
        }

        void Flush()
        {
            for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
            {
                (*it)();
            }
            deletors.clear();
        }
    };

    class GLFWindow;
    
    class RendererBase
    {
    public:
        NOCOPY(RendererBase);
        RendererBase(GLFWindow* window) : mWndHandle(window) {}
        virtual ~RendererBase();

        void Init(RendererCreateInfo& createInfo);
        void Cleanup();
        void RenderFrame();

    private:
        void InitContext(RendererCreateInfo& createInfo);
        void InitSwapchain(RendererCreateInfo& createInfo);
        void InitCommands();
        void InitPipelines();
        void InitDescriptors();
        void InitSyncStructures();

    private:
        static const uint8_t mBackBufferCount = 2;

        RHIGPU* mGPU = nullptr;
        RHIInstance* mInstance = nullptr;
        RHIQueue* mQueue = nullptr;
        std::unique_ptr<RHIDevice> mDevice;
        std::unique_ptr<RHISurface> mSurface;
        std::unique_ptr<RHISwapchain> mSwapchain;
        RHIFormat mSwapchainFormat = RHIFormat::Count;
        std::array<RHITexture*, mBackBufferCount> mSwapchainTextures;
        std::array<std::unique_ptr<RHITextureView>, mBackBufferCount> mSwapchainTextureViews;
        std::unique_ptr<RHIBuffer> mVertexBuffer;
        std::unique_ptr<RHIBufferView> mVertexBufferView;
        std::unique_ptr<RHIPipelineLayout> mPipelineLayout;
        std::unique_ptr<RHIGraphicsPipeline> mGraphicsPipeline;
        std::unique_ptr<RHIShaderModule> mVertexShader;
        std::unique_ptr<RHIShaderModule> mFragmentShader;
        std::unique_ptr<RHICommandBuffer> mCommandBuffer;
        std::unique_ptr<RHIFence> mFence;

        GLFWindow* mWndHandle;
    };
}