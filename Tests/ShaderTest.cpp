#include "Renderer/Shader.hpp"

#include <iostream>
#include <set>
#include <cassert>

class TestGlobalShaderPS
{
public:
    ShaderInfo(
        "TestGlobalShader",
        "/Engine/Shader/HLSL/TestGlobalShader.hlsl",
        "PSMain",
        Renderer::ShaderStage::Pixel
    );

    BoolShaderVariantField(TestBoolVar, "TEST_BOOL");
    RangedIntShaderVariantField(TestRangedInVar, "TEST_RANGED_INT", 0, 3);
    VariantSet(TestBoolVar, TestRangedInVar);

    DefaultVariantFilter
};
RegisterGlobalShader(TestGlobalShaderPS);

int main()
{
    std::set<std::pair<bool, uint8_t>> expectVars = {
        { false, 0 },
        { false, 1 },
        { false, 2 },
        { false, 3 },
        { true, 0 },
        { true, 1 },
        { true, 2 },
        { true, 3 }
    };

    std::vector<std::pair<bool, uint8_t>> realVars;
    TestGlobalShaderPS::VariantSet::TraverseAll([&realVars](auto&& varSet) -> void
    {
        realVars.emplace_back(
            varSet.template Get<TestGlobalShaderPS::TestBoolVar>(),
            varSet.template Get<TestGlobalShaderPS::TestRangedInVar>()
        );
    });

    assert(expectVars.size() == realVars.size());

    for (const auto& var : realVars)
    {
        assert(expectVars.contains(var));
        
        std::cout << "(" << var.first << ", " << (int)var.second << ")";
    }

    return 0;
}