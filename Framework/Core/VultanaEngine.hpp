#pragma once

#include <iostream>

namespace Vultana
{
    class Engine
    {
    public:
        void Init(void* windowHandle, uint32_t width, uint32_t height);
        void Shutdown();
        void Tick();

    private:
        void* mWndHandle;
        uint64_t mLastFrameTime = 0;
        float mFrameTime = 0.0f;
    };
} // namespace Vultana

