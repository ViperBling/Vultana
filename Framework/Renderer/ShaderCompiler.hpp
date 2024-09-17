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
            const std::string& source, 
            const std::string& file, 
            const std::string& entryPoint, 
            RHI::ERHIShaderType type, 
            const std::vector<std::string>& defines, 
            RHI::ERHIShaderCompileFlags flags, 
            std::vector<uint8_t>& output);

    private:
        RendererBase* mpRenderer = nullptr;
        IDxcCompiler3* mpDxcCompiler = nullptr;
        IDxcUtils* mpDxcUtils = nullptr;
        IDxcIncludeHandler* mpDxcIncludeHandler = nullptr;
    };
}