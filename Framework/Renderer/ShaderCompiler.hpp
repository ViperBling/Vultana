#pragma once

#include "RHI/RHICommon.hpp"

struct IDxcCompiler3;
struct IDxcUtils;
struct IDxcIncludeHandler;
struct IRCompiler;
struct IRRootSignature;

namespace Renderer
{
    class RendererBase;

    class ShaderCompiler
    {
    public:
        ShaderCompiler(RendererBase* renderer);
        ~ShaderCompiler();

        bool Compile(
            const eastl::string& source, 
            const eastl::string& file, 
            const eastl::string& entryPoint, 
            RHI::ERHIShaderType type, 
            const eastl::vector<eastl::string>& defines, 
            RHI::ERHIShaderCompileFlags flags, 
            eastl::vector<uint8_t>& output);

    private:
        RendererBase* mpRenderer = nullptr;
        IDxcCompiler3* mpDxcCompiler = nullptr;
        IDxcUtils* mpDxcUtils = nullptr;
        IDxcIncludeHandler* mpDxcIncludeHandler = nullptr;
    };
}