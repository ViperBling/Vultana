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
        Math::Vector2 Size = { 800.0f, 600.0f };
        Math::Vector2 Position = { 100.0f, 100.0f };
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

        void SetSize(Math::Vector2 size);
        Math::Vector2 GetSize() const;
        void SetPosition(Math::Vector2 position);
        Math::Vector2 GetPosition() const;
        void SetTitle(const char* title);

        Math::Vector2 GetCursorPosition() const;
        void SetCursorPosition(Math::Vector2 position);

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
        std::function<void(GLFWindow&, uint32_t width, uint32_t height)> mOnResize;
        std::function<void(GLFWindow&, Utility::KeyCode, bool)> mOnKeyChanged;
        std::function<void(GLFWindow&, Utility::MouseButton, bool)> mOnMouseChanged;
    };

    // const vk::SurfaceKHR&  CreateVulkanSurface(GLFWwindow *window, vk::Instance instance);
    // bool CheckVulkanPresentationSupport(const vk::Instance& instance, const vk::PhysicalDevice& physicalDevice, uint32_t familyQueueIndex);
}