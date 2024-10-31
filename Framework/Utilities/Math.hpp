#pragma once

#include <float.h>
#include <linalg.h>
#include <hlslpp/hlsl++.h>

#include <utility>
#include <cassert>

using namespace linalg;
using namespace linalg::aliases;

using uint = uint32_t;
using quaternion = float4;

static const float PI = 3.14159265358979323846f;

#define ENABLE_HLSLPP 1

inline hlslpp::float4 to_hlslpp(const float4& v)
{
    return hlslpp::float4(v.x, v.y, v.z, v.w);
}

inline hlslpp::float4x4 to_hlslpp(const float4x4& m)
{
    return hlslpp::float4x4(to_hlslpp(m[0]), to_hlslpp(m[1]), to_hlslpp(m[2]), to_hlslpp(m[3]));
}

inline float4x4 Mul(const float4x4& a, const float4x4& b)
{
#if ENABLE_HLSLPP
    hlslpp::float4x4 a1 = to_hlslpp(a);
    hlslpp::float4x4 b1 = to_hlslpp(b);

    return float4x4(mul(b1, a1).f32_128_0);
#else
    return mul<float, 4>(a, b);
#endif
}

inline float4 Mul(const float4x4& a, const float4& b)
{
#if ENABLE_HLSLPP
    hlslpp::float4x4 a1 = to_hlslpp(a);
    hlslpp::float4 b1 = to_hlslpp(b);

    return float4(hlslpp::mul(b1, a1).f32);
#else
    return mul<float, 4>(a, b);
#endif
}

inline float4x4 Inverse(const float4x4& m)
{
#if ENABLE_HLSLPP
    hlslpp::float4x4 m1 = to_hlslpp(m);
    return float4x4(hlslpp::inverse(m1).f32_128_0);
#else
    return inverse<float, 4>(m);
#endif
}

template<class T>
inline T DegreeToRadian(T degree)
{
    return degree * PI / 180.0f;
}

template<class T>
inline T RadianToDegree(T radian)
{
    return radian * 180.0f / PI;
}

inline quaternion RotationQuat(const float3& euler_angles) //pitch-yaw-roll order, in degrees
{
    float3 radians = DegreeToRadian(euler_angles);

    float3 c = cos(radians * 0.5f);
    float3 s = sin(radians * 0.5f);

    quaternion q;
    q.w = c.x * c.y * c.z + s.x * s.y * s.z;
    q.x = s.x * c.y * c.z - c.x * s.y * s.z;
    q.y = c.x * s.y * c.z + s.x * c.y * s.z;
    q.z = c.x * c.y * s.z - s.x * s.y * c.z;

    // quaternion q;
    // q.w = c.y * c.z * c.x + s.y * s.z * s.x; // w component
    // q.x = s.y * c.z * c.x - c.y * s.z * s.x; // x component
    // q.y = c.y * s.z * c.x + s.y * c.z * s.x; // y component
    // q.z = c.y * c.z * s.x - s.y * s.z * c.x; // z component

    return q;
}

inline float RotationPitch(const quaternion& q)
{
    //return T(atan(T(2) * (q.y * q.z + q.w * q.x), q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z));
    const float y = 2.0f * (q.y * q.z + q.w * q.x);
    const float x = q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z;
    if (y == 0.0f && x == 0.0f) //avoid atan2(0,0) - handle singularity
    {
        return RadianToDegree(2.0f * atan2(q.x, q.w));
    }

    return RadianToDegree(atan2(y, x));
}

inline float RotationYaw(const quaternion& q)
{
    return RadianToDegree(asin(clamp(-2.0f * (q.x * q.z - q.w * q.y), -1.0f, 1.0f)));
}

inline float RotationRoll(const quaternion& q)
{
    return RadianToDegree(atan2(2.0f * (q.x * q.y + q.w * q.z), q.w * q.w + q.x * q.x - q.y * q.y - q.z * q.z));
}

inline float3 RotationAngles(const quaternion& q)
{
    return float3(RotationPitch(q), RotationYaw(q), RotationRoll(q));
}

inline quaternion RotationSlerp(const quaternion& a, quaternion b, float interpolationValue)
{
    float dotProduct = dot(a, b);

    //make sure we take the shortest path in case dot Product is negative
    if (dotProduct < 0.0)
    {
        b = -b;
        dotProduct = -dotProduct;
    }

    //if the two quaternions are too close to each other, just linear interpolate between the 4D vector
    if (dotProduct > 0.9995)
    {
        return normalize(a + interpolationValue * (b - a));
    }

    //perform the spherical linear interpolation
    float theta_0 = acos(dotProduct);
    float theta = interpolationValue * theta_0;
    float sin_theta = sin(theta);
    float sin_theta_0 = sin(theta_0);

    float scalePreviousQuat = cos(theta) - dotProduct * sin_theta / sin_theta_0;
    float scaleNextQuat = sin_theta / sin_theta_0;
    return scalePreviousQuat * a + scaleNextQuat * b;
}

inline float4x4 OrthoMatrix(float l, float r, float b, float t, float n, float f)
{
    //https://docs.microsoft.com/en-us/windows/win32/direct3d10/d3d10-d3dxmatrixorthooffcenterlh
    return { {2 / (r - l), 0, 0, 0}, {0, 2 / (t - b), 0, 0}, {0, 0, 1 / (f - n), 0}, {(l + r) / (l - r), (t + b) / (b - t), n / (n - f), 1} };
}

inline void Decompose(const float4x4& matrix, float3& translation, quaternion& rotation, float3& scale)
{
    translation = matrix[3].xyz();

    float3 right = matrix[0].xyz();
    float3 up = matrix[1].xyz();
    float3 dir = matrix[2].xyz();

    scale[0] = length(right);
    scale[1] = length(up);
    scale[2] = length(dir);

    float3x3 rotationMat = float3x3(right / scale[0], up / scale[1], dir / scale[2]);
    rotation = rotation_quat(rotationMat);
}

inline void Decompose(const float4x4& matrix, float3& translation, float3& rotation, float3& scale)
{
    quaternion q;
    Decompose(matrix, translation, q, scale);

    rotation = RotationAngles(q);
}

inline float Sign(float x)
{
    return float((x > 0) - (x < 0));
}

inline bool NearlyEqual(float a, float b)
{
    return std::abs(a - b) < FLT_EPSILON;
}

template<class T>
inline bool NearlyEqual(const T& a, const T& b)
{
    return NearlyEqual(a.x, b.x) && NearlyEqual(a.y, b.y) && NearlyEqual(a.z, b.z) && NearlyEqual(a.w, b.w);
}

inline float4 NormalizePlane(const float4& plane)
{
    float length = std::sqrt(plane.x * plane.x + plane.y * plane.y + plane.z * plane.z);
    return plane * (1.0f / length);
}

inline bool FrustumCull(const float4* planes, uint32_t plane_count, float3 center, float radius)
{
    for (uint32_t i = 0; i < plane_count; i++)
    {
        if (dot(center, planes[i].xyz()) + planes[i].w + radius < 0)
        {
            return false;
        }
    }

    return true;
}

inline uint32_t DivideRoudingUp(uint32_t a, uint32_t b)
{
    return (a + b - 1) / b;
}

inline bool IsPow2(uint32_t x)
{
    return (x & (x - 1)) == 0;
}

inline uint32_t RoundUpPow2(uint32_t a, uint32_t b)
{
    assert(IsPow2(b));
    return (a + b - 1) & ~(b - 1);
}

union FP32
{
    uint u;
    float f;
    struct
    {
        uint Mantissa : 23;
        uint Exponent : 8;
        uint Sign : 1;
    };
};

union FP16
{
    uint16_t u;
    struct
    {
        uint Mantissa : 10;
        uint Exponent : 5;
        uint Sign : 1;
    };
};

//https://gist.github.com/rygorous/2156668, float_to_half_fast3
inline uint16_t FloatToHalf(float value)
{
    FP32 f;
    f.f = value;
    FP32 f32infty = { 255 << 23 };
    FP32 f16infty = { 31 << 23 };
    FP32 magic = { 15 << 23 };
    uint sign_mask = 0x80000000u;
    uint round_mask = ~0xfffu;
    FP16 o = { 0 };

    uint sign = f.u & sign_mask;
    f.u ^= sign;

    // NOTE all the integer compares in this function can be safely
    // compiled into signed compares since all operands are below
    // 0x80000000. Important if you want fast straight SSE2 code
    // (since there's no unsigned PCMPGTD).

    if (f.u >= f32infty.u) // Inf or NaN (all exponent bits set)
    {
        o.u = (f.u > f32infty.u) ? 0x7e00 : 0x7c00; // NaN->qNaN and Inf->Inf
    }
    else // (De)normalized number or zero
    {
        f.u &= round_mask;
        f.f *= magic.f;
        f.u -= round_mask;
        if (f.u > f16infty.u) f.u = f16infty.u; // Clamp to signed infinity if overflowed

        o.u = f.u >> 13; // Take the bits!
    }

    o.u |= sign >> 16;
    return o.u;
}

inline float HalfToFloat(uint16_t value)
{
    FP16 h = { value };
    static const FP32 magic = { 113 << 23 };
    static const uint shifted_exp = 0x7c00 << 13; // exponent mask after shift
    FP32 o;

    o.u = (h.u & 0x7fff) << 13;     // exponent/mantissa bits
    uint exp = shifted_exp & o.u;   // just the exponent
    o.u += (127 - 15) << 23;        // exponent adjust

    // handle exponent special cases
    if (exp == shifted_exp) // Inf/NaN?
    {
        o.u += (128 - 16) << 23;    // extra exp adjust
    }
    else if (exp == 0) // Zero/Denormal?
    {
        o.u += 1 << 23;             // extra exp adjust
        o.f -= magic.f;             // renormalize
    }

    o.u |= (h.u & 0x8000) << 16;    // sign bit
    return o.f;
}