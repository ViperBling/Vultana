#include "Renderer/Shader.hpp"

class TestGlobalShaderPS : public Renderer::GlobalShader
{
public:
    ShaderInfo(
        "TestGlobalShader",
        "/Engine/Shader/HLSL/TestGlobalShader.hlsl",
        "PSMain",
        Renderer::ShaderStage::Pixel
    );

    // BoolShaderVariantField(TestBoolVar, "TEST_BOOL");
    // RangedIntShaderVariantField(TestRangedInVar, "TEST_RANGED_INT", 0, 3);
    // VariantSet(TestBoolVar, TestRangedInVar);

    // DefaultVariantFilter
};

RegisterGlobalShaders(TestGlobalShaderPS);

