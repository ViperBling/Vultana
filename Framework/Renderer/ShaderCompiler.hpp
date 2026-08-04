#pragma once

#include "RHI/RHICommon.hpp"

struct IDxcCompiler3;
struct IDxcUtils;
struct IDxcIncludeHandler;
struct IRCompiler;
struct IRRootSignature;

namespace Renderer
{
    class FRendererBase;

    class FShaderCompiler
    {
    public:
        FShaderCompiler(FRendererBase* renderer);
        ~FShaderCompiler();

        bool Compile(
            const eastl::string& source, 
            const eastl::string& file, 
            const eastl::string& entryPoint, 
            RHI::ERHIShaderType type, 
            const eastl::vector<eastl::string>& defines, 
            RHI::ERHIShaderCompileFlags flags, 
            eastl::vector<uint8_t>& output);

    private:
        FRendererBase* m_pRenderer = nullptr;
        IDxcCompiler3* m_pDxcCompiler = nullptr;
        IDxcUtils* m_pDxcUtils = nullptr;
        IDxcIncludeHandler* m_pDxcIncludeHandler = nullptr;
    };
}