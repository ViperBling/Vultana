#pragma once

#include "Utilities/Utility.hpp"

#include <unordered_map>
#include <vulkan/vulkan.hpp>

namespace Vultana
{
    class VKInstance
    {
    public:
        NOCOPY(VKInstance);

        VKInstance(vk::Instance instance);
        ~VKInstance();
        const std::vector<const char*>& GetExtensions() const;
        
        vk::Instance GetHandle() const;

    private:
        vk::Instance mInstance;
    };
}