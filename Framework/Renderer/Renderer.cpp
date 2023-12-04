#include "Renderer.hpp"

using namespace Vultana;

namespace Vultana::Renderer
{
    RendererBase::RendererBase()
    {
    }

    RendererBase::~RendererBase()
    {
    }

    void RendererBase::CreateDevice(void *wndHandle, uint32_t wndWidth, uint32_t wndHeight)
    {
        mDisplayWidth = wndWidth;
        mDisplayHeight = wndHeight;

        RHI::RHIDeviceInfo deviceInfo;
        deviceInfo.MaxFrameLag = RHI::RHI_MAX_IN_FLIGHT_FRAMES;
        mpDevice.reset(RHI::CreateRHIDevice(deviceInfo));
    }

    void RendererBase::RenderFrame()
    {
    }

} // namespace Vultana::Renderer
