#pragma once

#include <functional>
#include <iostream>
#include <vector>

#include "Utilities/Math.hpp"
#include "Utilities/KeyCodes.hpp"
#include "RHI/RHIVulkan/RHICommonVK.hpp"
#include "Renderer/RendererBase.hpp"

struct GLFWwindow;

namespace Window
{
    struct WindowCreateInfo
    {
        bool TransparentFramebuffer = false;
        bool Resizeable = true;
        bool TileBar = true;
        float2 Size = { 800.0f, 600.0f };
        float2 Position = { 100.0f, 100.0f };
        const char* Title = "Vultana";
    };

    class GLFWindow
    {
    public:
        GLFWindow(const WindowCreateInfo& options);
        GLFWindow(const GLFWindow&) = delete;
        GLFWindow& operator=(const GLFWindow&) = delete;
        GLFWindow(GLFWindow&& other) noexcept;
        GLFWindow& operator=(GLFWindow&& other) noexcept;
        ~GLFWindow();

        GLFWwindow* GetNativeHandle() const { return this->mHwnd; }
        std::vector<const char*> GetRequiredExtensions() const;

        void PollEvents() const;
        bool ShouldClose() const;
        void Close();

        void SetSize(float2 size);
        float2 GetSize() const;
        void SetPosition(float2 position);
        float2 GetPosition() const;
        void SetTitle(const char* title);

        float2 GetCursorPosition() const;
        void SetCursorPosition(float2 position);
        float2 GetLastCursorPosition() const { return mLastCursorPosition; }
        void SetLastCursorPosition(float2 position) { mLastCursorPosition = position; }
        float2 GetCursorDelta();

        Utility::CursorMode GetCursorMode() const;
        void SetCursorMode(Utility::CursorMode mode);

        bool IsKeyPressed(Utility::KeyCode key) const;
        bool IsKeyReleased(Utility::KeyCode key) const;

        bool IsMousePressed(Utility::MouseButton button) const;
        bool IsMouseReleased(Utility::MouseButton button) const;

        float GetTimeSinceCreation() const;
        void SetTimeSinceCreation(float time);

        void OnResize(std::function<void(GLFWindow&, uint32_t width, uint32_t height)> callback);
        void OnKeyChanged(std::function<void(GLFWindow&, Utility::KeyCode, bool)> callback);
        void OnMouseChanged(std::function<void(GLFWindow&, Utility::MouseButton, bool)> callback);
        
        void SetContext(GLFWwindow* window);

        vk::Result CreateVulkanSurface(vk::Instance instance, vk::SurfaceKHR& surface);

    private:
        GLFWwindow* mHwnd = nullptr;
        std::vector<std::function<void(GLFWindow&, uint32_t width, uint32_t height)>> mOnResizeCallbacks;
        std::function<void(GLFWindow&, Utility::KeyCode, bool)> mOnKeyChanged;
        std::function<void(GLFWindow&, Utility::MouseButton, bool)> mOnMouseChanged;

        float2 mLastCursorPosition = { 0.0f, 0.0f };
    };

    // const vk::SurfaceKHR&  CreateVulkanSurface(GLFWwindow *window, vk::Instance instance);
    // bool CheckVulkanPresentationSupport(const vk::Instance& instance, const vk::PhysicalDevice& physicalDevice, uint32_t familyQueueIndex);
}