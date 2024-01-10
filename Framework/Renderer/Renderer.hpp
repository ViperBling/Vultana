#pragma once

#include "RHI/RHIPCH.hpp"
#include "Utilities/Utility.hpp"
#include "Scene/GLTFParser.hpp"

#include <iostream>
#include <deque>
#include <memory>
#include <functional>

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace Window
{
    class GLFWindow;
}

namespace Renderer
{
    struct RendererCreateInfo
    {
        RHI::RHIDeviceType DeviceType = RHI::RHIDeviceType::Hardware;
        const char* ApplicationName = "Vultana";
        uint32_t Width = 1280;
        uint32_t Height = 720;
        bool bEnableValidationLayers = true;
    };
    
    class RendererBase
    {
    public:
        NOCOPY(RendererBase);
        RendererBase(Window::GLFWindow* window) : mWndHandle(window) {}
        virtual ~RendererBase();

        virtual void Init(RendererCreateInfo& createInfo);
        virtual void Cleanup();
        virtual void RenderFrame();

    private:
        void InitContext(RendererCreateInfo& createInfo);
        void InitSwapchain(RendererCreateInfo& createInfo);
        void CreateSwapchainImageView();
        void InitPipelines();
        void CreateVertexBuffer();
        void InitSyncStructures();
        void InitCommands();

        void RecordCommandBuffer();
        void SubmitCommandBuffer();

        void CompileShader(std::vector<uint8_t>& byteCode, const std::string& fileName, const std::string& entryPoint, RHI::RHIShaderStageBits shaderStage, std::vector<std::string> includePath = {});

    private:
        static constexpr uint8_t mBackBufferCount = 2;
        static constexpr uint8_t mSSAOKernelSize = 64;
        static constexpr uint8_t mSSAONoiseSize = 16;

        RHI::RHIGPU* mGPU = nullptr;
        RHI::RHIInstance* mInstance = nullptr;
        RHI::RHIQueue* mQueue = nullptr;
        std::unique_ptr<RHI::RHIDevice> mDevice;
        std::unique_ptr<RHI::RHISurface> mSurface;
        std::unique_ptr<RHI::RHISwapchain> mSwapchain;
        RHI::RHIFormat mSwapchainFormat = RHI::RHIFormat::Count;
        std::array<RHI::RHITexture*, mBackBufferCount> mSwapchainTextures;
        std::array<std::unique_ptr<RHI::RHITextureView>, mBackBufferCount> mSwapchainTextureViews;
        std::unique_ptr<RHI::RHIBuffer> mVertexBuffer;
        std::unique_ptr<RHI::RHIBufferView> mVertexBufferView;
        std::unique_ptr<RHI::RHIBuffer> mIndexBuffer;
        std::unique_ptr<RHI::RHIBufferView> mIndexBufferView;
        std::unique_ptr<RHI::RHIPipelineLayout> mPipelineLayout;
        std::unique_ptr<RHI::RHIGraphicsPipeline> mGraphicsPipeline;
        std::unique_ptr<RHI::RHIShaderModule> mVertexShader;
        std::unique_ptr<RHI::RHIShaderModule> mFragmentShader;
        std::unique_ptr<RHI::RHICommandBuffer> mCommandBuffer;
        std::unique_ptr<RHI::RHIFence> mFence;

        std::unique_ptr<RHI::RHISampler> mSampler;
        std::unique_ptr<RHI::RHISampler> mNoiseSampler;

        Math::Vector2u mSwapchainExtent;
        Window::GLFWindow* mWndHandle;

        std::unique_ptr<Scene::Model> mModel;

        struct UBuffer
        {
            std::unique_ptr<RHI::RHIBuffer> Buffer;
            std::unique_ptr<RHI::RHIBufferView> BufferView;
        };
        struct UniformBuffers
        {
            UBuffer SceneParams;
        } mUniformBuffers;

        struct UBOSceneParams
        {
            Math::Matrix4x4 Projection;
            Math::Matrix4x4 Model;
            Math::Matrix4x4 View;
            float NearPlane = 0.1f;
            float FarPlane = 100.0f;
        } mUBOSceneParams;

        
    };
}