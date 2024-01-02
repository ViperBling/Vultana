#pragma once

#include "RHICommon.hpp"

namespace RHI
{
    class RHIDevice;
    class RHIBuffer;
    class RHITexture;

    struct BufferTransitionBase
    {
        RHIBufferState Before;
        RHIBufferState After;
    };

    struct TextureTransitionBase
    {
        RHITextureState Before;
        RHITextureState After;
    };

    struct BufferTransition : public BufferTransitionBase
    {
        RHIBuffer* Buffer;
    };

    struct TextureTransition : public TextureTransitionBase
    {
        RHITexture* Texture;
    };

    class RHIBarrier
    {
    public:
        ~RHIBarrier() = default;

        static RHIBarrier Transition(RHIBuffer* buffer, RHIBufferState before, RHIBufferState after)
        {
            RHIBarrier barrier {};
            barrier.Type = RHIResourceType::Buffer;
            barrier.Buffer.Buffer = buffer;
            barrier.Buffer.Before = before;
            barrier.Buffer.After = after;
            return barrier;
        }

        static RHIBarrier Transition(RHITexture* texture, RHITextureState before, RHITextureState after)
        {
            RHIBarrier barrier {};
            barrier.Type = RHIResourceType::Texture;
            barrier.Texture.Texture = texture;
            barrier.Texture.Before = before;
            barrier.Texture.After = after;
            return barrier;
        }

        RHIResourceType Type;
        union
        {
            BufferTransition Buffer;
            TextureTransition Texture;
        };
    };

    enum class FenceStatus
    {
        Wait,
        Signal,
        Count
    };

    class RHIFence
    {
    public:
        NOCOPY(RHIFence)
        virtual ~RHIFence() = default;

        virtual FenceStatus GetFenceStatus() = 0;
        virtual void Reset() = 0;
        virtual void Wait() = 0;
        virtual void Destroy() = 0;

    protected:
        explicit RHIFence(RHIDevice& device) {}
    };
}