#include "RHIFenceVK.hpp"
#include "RHIDeviceVK.hpp"
#include "Utilities/Log.hpp"

namespace RHI
{
    FVulkanFence::FVulkanFence(FVulkanDevice *device, const eastl::string &name)
    {
        m_pDevice = device;
        m_Name = name;
    }

    FVulkanFence::~FVulkanFence()
    {
        ((FVulkanDevice*)m_pDevice)->Delete(m_Semaphore);
    }

    bool FVulkanFence::Create()
    {
        auto device = ((FVulkanDevice*)m_pDevice)->GetDevice();
        auto dynamicLoader = ((FVulkanDevice*)m_pDevice)->GetDynamicLoader();

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

    void FVulkanFence::Wait(uint64_t value)
    {
        vk::SemaphoreWaitInfo waitInfo {};
        waitInfo.semaphoreCount = 1;
        waitInfo.pSemaphores = &m_Semaphore;
        waitInfo.pValues = &value;
        
        auto device = ((FVulkanDevice*)m_pDevice)->GetDevice();
        // assert(device.waitSemaphores(waitInfo, UINT64_MAX) == vk::Result::eSuccess);
        const vk::Result result = device.waitSemaphores(waitInfo, UINT64_MAX);
        assert(result == vk::Result::eSuccess);
        (void)result;
    }

    void FVulkanFence::Signal(uint64_t value)
    {
        vk::SemaphoreSignalInfo signalInfo {};
        signalInfo.semaphore = m_Semaphore;
        signalInfo.value = value;

        auto device = ((FVulkanDevice*)m_pDevice)->GetDevice();
        device.signalSemaphore(signalInfo);
    }
}