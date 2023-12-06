#pragma once

#include <functional>
#include <iostream>
#include <vector>

#include "Utilities/Math.hpp"
#include "Utilities/KeyCodes.hpp"
#include "Renderer/Renderer.hpp"

struct GLFWwindow;

namespace Vultana
{
    inline void DefaultWindowCallback(const std::string& message)
    {
        std::cout << message << std::endl;
    }

    struct WindowCreateInfo
    {
        bool TransparentFramebuffer = false;
        bool Resizeable = true;
        bool TileBar = true;
        std::function<void(const std::string&)> ErrorCallback = DefaultWindowCallback;
        Vector2 Size = { 800.0f, 600.0f };
        Vector2 Position = { 100.0f, 100.0f };
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

        void SetSize(Vector2 size);
        Vector2 GetSize() const;
        void SetPosition(Vector2 position);
        Vector2 GetPosition() const;
        void SetTitle(const char* title);

        Vector2 GetCursorPosition() const;
        void SetCursorPosition(Vector2 position);

        CursorMode GetCursorMode() const;
        void SetCursorMode(CursorMode mode);

        bool IsKeyPressed(KeyCode key) const;
        bool IsKeyReleased(KeyCode key) const;

        bool IsMousePressed(MouseButton button) const;
        bool IsMouseReleased(MouseButton button) const;

        float GetTimeSinceCreation() const;
        void SetTimeSinceCreation(float time);

        void OnResize(std::function<void(GLFWindow&, Vector2)> callback);
        void OnKeyChanged(std::function<void(GLFWindow&, KeyCode, bool)> callback);
        void OnMouseChanged(std::function<void(GLFWindow&, MouseButton, bool)> callback);

        // const WindowSurface& CreateWindowSurface(const VulkanContext& context);
        
        
        void SetContext(GLFWwindow* window);

    private:
        GLFWwindow* mHwnd = nullptr;
        std::function<void(GLFWindow&, Vector2)> mOnResize;
        std::function<void(GLFWindow&, KeyCode, bool)> mOnKeyChanged;
        std::function<void(GLFWindow&, MouseButton, bool)> mOnMouseChanged;
    };

    void CreateWindowSurface(const vk::Instance& instance, void* windowHandle, vk::SurfaceKHR& surface);
    bool CheckVulkanSupport(const vk::Instance& instance, const vk::PhysicalDevice& physicalDevice, uint32_t familyQueueIndex);

    inline void WindowErrorCallback(const std::string& msg)
    {
        std::cerr << "[ERROR Window] : " << msg << std::endl;
    }
}