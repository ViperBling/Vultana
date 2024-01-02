#pragma once

#include <vector>
#include <string>

#include "RHI/RHICommon.hpp"
#include "RHI/RHIBindGroupLayout.hpp"
#include "Shader.hpp"
#include "Utilities/Concurrent.hpp"

namespace Renderer
{
    enum class ShaderByteCodeType
    {
        DXIL,
        SPIRV,
        Count,
    };

    struct ShaderCompileInput
    {
        std::string Source;
        std::string EntryPoint;
        RHI::RHIShaderStageBits Stage;
    };

    struct ShaderCompileOptions
    {
        ShaderByteCodeType ByteCodeType = ShaderByteCodeType::Count;
        bool bWithDebugInfo = false;
        std::vector<std::string> Definitions;
        std::vector<std::string> IncludePaths;
    };

    struct ShaderCompileOutput
    {
        bool bSuccess;
        std::vector<uint8_t> ByteCode;
        ShaderReflectionData ReflectionData;
        std::string ErrorMsg;
    };

    class ShaderCompiler
    {
    public:
        static ShaderCompiler& Get();
        ~ShaderCompiler() = default;
        std::future<ShaderCompileOutput> Compile(const ShaderCompileInput& input, const ShaderCompileOptions& options);

    private:
        ShaderCompiler();

    private:
        Utility::ThreadPool mThreadPool;
    };
}