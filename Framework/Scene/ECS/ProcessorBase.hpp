#pragma once

#include <EASTL/unique_ptr.h>
#include <cstdint>

namespace Scene::ECS
{
    class FEntitySubsystem;

    enum class EProcessingPhase : uint8_t
    {
        PreUpdate   = 0,
        Update      = 1,
        PostUpdate  = 2,
        Render      = 3,
        Count       = 4
    };

    class FProcessorBase
    {
    public:
        virtual ~FProcessorBase() = default;
        virtual EProcessingPhase GetProcessingPhase() const = 0;
        virtual void Process(FEntitySubsystem& entitySubsystem, float deltaTime) = 0;
        virtual const char* GetName() const = 0;
    };
}