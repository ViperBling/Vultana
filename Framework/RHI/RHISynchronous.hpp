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

    class RHIFence
    {
    public:
        NOCOPY(RHIFence)
        virtual ~RHIFence() = default;

        virtual void IsSignaled() = 0;
        virtual void Reset() = 0;
        virtual void Wait() = 0;

    protected:
        explicit RHIFence(RHIDevice& device, bool bInitAsSignaled) {}
    };

    class RHISemaphore
    {
    public:
        NOCOPY(RHISemaphore)
        virtual ~RHISemaphore() = default;

    protected:
        explicit RHISemaphore(RHIDevice& device) {}
    };
}