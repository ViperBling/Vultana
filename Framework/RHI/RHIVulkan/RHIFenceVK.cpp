#include "RHIFenceVK.hpp"
#include "RHIDeviceVK.hpp"
#include "Utilities/Log.hpp"

namespace RHI
{
    RHIFenceVK::RHIFenceVK(RHIDeviceVK *device, const eastl::string &name)
    {
        m_pDevice = device;
        m_Name = name;
    }

    RHIFenceVK::~RHIFenceVK()
    {
        ((RHIDeviceVK*)m_pDevice)->Delete(m_Semaphore);
    }

    bool RHIFenceVK::Create()
    {
        auto device = ((RHIDeviceVK*)m_pDevice)->GetDevice();
        auto dynamicLoader = ((RHIDeviceVK*)m_pDevice)->GetDynamicLoader();

        vk::SemaphoreTypeCreateInfoKHR semaphoreTypeCI {};
        semaphoreTypeCI.semaphoreType = vk::SemaphoreType::eTimeline;
        semaphoreTypeCI.initialValue = 0;

        vk::SemaphoreCreateInfo semaphoreCI {};
        semaphoreCI.pNext = &semaphoreTypeCI;

        m_Semaphore = device.createSemaphore(semaphoreCI);
        if (!m_Semaphore)
        {
            VTNA_LOG_ERROR("[RHIFenceVK] Failed to create {}", m_Name);
            return false;
        }
        SetDebugName(device, vk::ObjectType::eSemaphore, m_Semaphore, m_Name.c_str(), dynamicLoader);

        return true;
    }

    void RHIFenceVK::Wait(uint64_t value)
    {
        vk::SemaphoreWaitInfo waitInfo {};
        waitInfo.semaphoreCount = 1;
        waitInfo.pSemaphores = &m_Semaphore;
        waitInfo.pValues = &value;
        
        auto device = ((RHIDeviceVK*)m_pDevice)->GetDevice();
        assert(device.waitSemaphores(waitInfo, UINT64_MAX) == vk::Result::eSuccess);
    }

    void RHIFenceVK::Signal(uint64_t value)
    {
        vk::SemaphoreSignalInfo signalInfo {};
        signalInfo.semaphore = m_Semaphore;
        signalInfo.value = value;

        auto device = ((RHIDeviceVK*)m_pDevice)->GetDevice();
        device.signalSemaphore(signalInfo);
    }
}