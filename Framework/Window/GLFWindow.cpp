#include "GLFWindow.hpp"

#include "Utilities/Log.hpp"

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

namespace Window
{
    std::vector<const char *> GLFWindow::GetRequiredExtensions() const
    {
        uint32_t extensionCount = 0;
        const char** extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
        return std::vector<const char*>(extensions, extensions + extensionCount);
    }

    GLFWindow::GLFWindow(const WindowCreateInfo &createInfo)
    {
        if (glfwInit() != GLFW_TRUE)
        {
            GDebugInfoCallback("Failed to initialize GLFW", "Window");
            return;
        }
        if (glfwVulkanSupported() != GLFW_TRUE)
        {
            GDebugInfoCallback("Vulkan is not supported in glfw context", "Window");
            return;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_DECORATED, createInfo.TileBar);
        glfwWindowHint(GLFW_RESIZABLE, createInfo.Resizeable);
        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, createInfo.TransparentFramebuffer);

        this->m_Hwnd = glfwCreateWindow((int)createInfo.Size.x, (int)createInfo.Size.y, createInfo.Title, nullptr, nullptr);
        if (this->m_Hwnd == nullptr)
        {
            GDebugInfoCallback("Failed to create window", "Window");
            return;
        }

        glfwSetWindowPos(this->m_Hwnd, (int)createInfo.Position.x, (int)createInfo.Position.y);
        glfwSetWindowUserPointer(this->m_Hwnd, (void*)this);
        glfwSetWindowSizeCallback(this->m_Hwnd, [](GLFWwindow* handle, int width, int height)
        {
                auto& window = *(GLFWindow*)glfwGetWindowUserPointer(handle);
                for (auto& callback : window.m_OnResizeCallbacks)
                {
                    if (callback) callback(window, (uint32_t)width, (uint32_t)height);
                }
        });
        glfwSetKeyCallback(this->m_Hwnd, [](GLFWwindow* handle, int key, int scancode, int action, int mods)
        {
            auto& window = *(GLFWindow*)glfwGetWindowUserPointer(handle);
            if (window.m_OnKeyChanged) window.m_OnKeyChanged(window, (Utility::KeyCode)key, action == GLFW_PRESS);
        });
        glfwSetMouseButtonCallback(this->m_Hwnd, [](GLFWwindow* handle, int button, int action, int mods)
        {
            auto& window = *(GLFWindow*)glfwGetWindowUserPointer(handle);
            if (window.m_OnMouseChanged) window.m_OnMouseChanged(window, (Utility::MouseButton)button, action == GLFW_PRESS);
        });

        m_LastCursorPosition = GetCursorPosition();
    }

    GLFWindow::GLFWindow(GLFWindow&& other) noexcept
    {
        this->m_Hwnd = other.m_Hwnd;
        this->m_OnResizeCallbacks = other.m_OnResizeCallbacks;
        other.m_Hwnd = nullptr;
        glfwSetWindowUserPointer(this->m_Hwnd, (void*)this);
    }

    GLFWindow &GLFWindow::operator=(GLFWindow &&other) noexcept
    {
        this->m_Hwnd = other.m_Hwnd;
        this->m_OnResizeCallbacks = other.m_OnResizeCallbacks;
        other.m_Hwnd = nullptr;
        glfwSetWindowUserPointer(this->m_Hwnd, (void*)this);
        return *this;
    }

    GLFWindow::~GLFWindow()
    {
        if (this->m_Hwnd != nullptr)
        {
            glfwDestroyWindow(this->m_Hwnd);
            this->m_Hwnd = nullptr;
        }
        m_OnResizeCallbacks.clear();
    }

    HWND GLFWindow::GetWin32WindowHandle()
    {
        return glfwGetWin32Window(this->m_Hwnd);
    }

    void GLFWindow::PollEvents() const
    {
        glfwPollEvents();
    }

    bool GLFWindow::ShouldClose() const
    {
        return glfwWindowShouldClose(this->m_Hwnd);
    }

    void GLFWindow::Close()
    {
        glfwSetWindowShouldClose(this->m_Hwnd, true);
    }

    void GLFWindow::SetSize(float2 size)
    {
        glfwSetWindowSize(this->m_Hwnd, (int)size.x, (int)size.y);
    }

    float2 GLFWindow::GetSize() const
    {
        int width = 0, height = 0;
        glfwGetWindowSize(this->m_Hwnd, &width, &height);
        return float2((float)width, (float)height);
    }

    void GLFWindow::SetPosition(float2 position)
    {
        glfwSetWindowPos(this->m_Hwnd, (int)position.x, (int)position.y);
    }

    float2 GLFWindow::GetPosition() const
    {
        int x = 0, y = 0;
        glfwGetWindowPos(this->m_Hwnd, &x, &y);
        return float2((float)x, (float)y);
    }

    void GLFWindow::SetTitle(const char *title)
    {
        glfwSetWindowTitle(this->m_Hwnd, title);
    }

    float2 GLFWindow::GetCursorPosition() const
    {
        double x = 0, y = 0;
        glfwGetCursorPos(this->m_Hwnd, &x, &y);
        return float2((float)x, (float)y);
    }

    void GLFWindow::SetCursorPosition(float2 position)
    {
        glfwSetCursorPos(this->m_Hwnd, (int)position.x, (int)position.y);
    }

    float2 GLFWindow::GetCursorDelta()
    {
        if (IsMousePressed(Utility::MouseButton::LEFT) || IsMousePressed(Utility::MouseButton::RIGHT))
        {
            auto curPos = GetCursorPosition();
            auto delta = curPos - m_LastCursorPosition;
            m_LastCursorPosition = curPos;
            return delta;
        }
        else if (IsMouseReleased(Utility::MouseButton::LEFT) || IsMouseReleased(Utility::MouseButton::RIGHT))
        {
            m_LastCursorPosition = GetCursorPosition();
        }
        return float2(0.0f);
    }

    Utility::CursorMode GLFWindow::GetCursorMode() const
    {
        return (Utility::CursorMode)glfwGetInputMode(this->m_Hwnd, GLFW_CURSOR);
    }

    void GLFWindow::SetCursorMode(Utility::CursorMode mode)
    {
        glfwSetInputMode(this->m_Hwnd, GLFW_CURSOR, (int)mode);
    }

    bool GLFWindow::IsKeyPressed(Utility::KeyCode key) const
    {
        return glfwGetKey(this->m_Hwnd, (int)key) == GLFW_PRESS;
    }

    bool GLFWindow::IsKeyReleased(Utility::KeyCode key) const
    {
        return glfwGetKey(this->m_Hwnd, (int)key) == GLFW_RELEASE;
    }

    bool GLFWindow::IsMousePressed(Utility::MouseButton button) const
    {
        return glfwGetMouseButton(this->m_Hwnd, (int)button) == GLFW_PRESS;
    }

    bool GLFWindow::IsMouseReleased(Utility::MouseButton button) const
    {
        return glfwGetMouseButton(this->m_Hwnd, (int)button) == GLFW_RELEASE;
    }

    float GLFWindow::GetTimeSinceCreation() const
    {
        return (float)glfwGetTime();
    }

    void GLFWindow::SetTimeSinceCreation(float time)
    {
        return glfwSetTime((float)time);
    }

    void GLFWindow::OnResize(std::function<void(GLFWindow &, uint32_t width, uint32_t height)> callback)
    {
        this->m_OnResizeCallbacks.push_back(std::move(callback));
    }

    void GLFWindow::OnKeyChanged(std::function<void(GLFWindow &, Utility::KeyCode, bool)> callback)
    {
        this->m_OnKeyChanged = std::move(callback);
    }

    void GLFWindow::OnMouseChanged(std::function<void(GLFWindow &, Utility::MouseButton, bool)> callback)
    {
        this->m_OnMouseChanged = std::move(callback);
    }

    void GLFWindow::SetContext(GLFWwindow *window)
    {
        this->m_Hwnd = window;
        glfwMakeContextCurrent(window);
    }

    vk::Result GLFWindow::CreateVulkanSurface(vk::Instance instance, vk::SurfaceKHR& surface)
    {
        return (vk::Result)glfwCreateWindowSurface(VkInstance(instance), m_Hwnd, nullptr, (VkSurfaceKHR*)&surface);
    }

    // const vk::SurfaceKHR& CreateVulkanSurface(GLFWwindow *window, vk::Instance instance)
    // {
    //     static vk::SurfaceKHR surface;
    //     (void)glfwCreateWindowSurface(VkInstance(instance), window, nullptr, (VkSurfaceKHR*)&surface);
    //     return surface;
    // }

    // bool CheckVulkanPresentationSupport(const vk::Instance& instance, const vk::PhysicalDevice& physicalDevice, uint32_t familyQueueIndex)
    // {
    //     return glfwGetPhysicalDevicePresentationSupport(instance, physicalDevice, familyQueueIndex) == GLFW_TRUE;
    // }
}