#pragma once

#include "Utilities/Math.hpp"

namespace Scene
{
    class Camera
    {
    public:
        void Tick(float deltaTime);

    private:
        Math::Vector3 mPosition = { 0.0f, 0.0f, 0.0f };
        Math::Vector3 mRotation = { 0.0f, 0.0f, 0.0f };

        bool mbUpdated = false;

        Math::Matrix4x4 mWorld;
        Math::Matrix4x4 mView;
        Math::Matrix4x4 mProjection;
        Math::Matrix4x4 mVPMat;

        float mAspectRatio = 1.0f;
        float mFov = 45.0f;
        float mNear = 0.1f;
        float mFar = 1000.0f;

        float mMoveSpeed = 10.0f;
    };
}