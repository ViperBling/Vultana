#pragma once

#include "Utilities/Math.hpp"

namespace Vultana::Scene
{
    class Camera
    {
    public:
        void Tick(float deltaTime);

    private:
        Vector3 mPosition = { 0.0f, 0.0f, 0.0f };
        Vector3 mRotation = { 0.0f, 0.0f, 0.0f };

        bool mbUpdated = false;

        Matrix4x4 mWorld;
        Matrix4x4 mView;
        Matrix4x4 mProjection;
        Matrix4x4 mVPMat;

        float mAspectRatio = 1.0f;
        float mFov = 45.0f;
        float mNear = 0.1f;
        float mFar = 1000.0f;

        float mMoveSpeed = 10.0f;
    };
}