#if (WIN32)
#define PLATFORM_WINDOWS
#endif

#include <unordered_map>

#ifdef PLATFORM_WINDOWS
#include <Windows.h>
#include <d3d12shader.h>
#include <wrl/client.h>
using namespace Microsoft::WRL;
#define ComPtrGet(name) name.Get() 
#undef min
#undef max
#endif

#include <dxcapi.h>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_hlsl.hpp>

#include "ShaderCompiler.hpp"
#include "Utilities/String.hpp"

namespace Renderer
{
    static std::wstring GetDXCTargetProfile(RHI::RHIShaderStageBits stage)
    {
        static const std::unordered_map<RHI::RHIShaderStageBits, std::wstring> stageToProfile =
        {
            { RHI::RHIShaderStageBits::Vertex, L"vs" },
            { RHI::RHIShaderStageBits::Pixel, L"ps" },
            { RHI::RHIShaderStageBits::Compute, L"cs" },
        };
        auto it = stageToProfile.find(stage);
        assert(it != stageToProfile.end());
        return it->second + L"_6_2";
    }

    static std::vector<LPCWSTR> GetDXCBaseArguments(const ShaderCompileOptions& options)
    {
        static std::vector<LPCWSTR> baseArgs = {
            DXC_ARG_WARNINGS_ARE_ERRORS,
            DXC_ARG_PACK_MATRIX_ROW_MAJOR
        };

        std::vector<LPCWSTR> result = baseArgs;
        if (options.bWithDebugInfo)
        {
            result.emplace_back(L"-Qembed_debug");
            result.emplace_back(DXC_ARG_DEBUG);
        }
        if (options.ByteCodeType != ShaderByteCodeType::DXIL)
        {
            result.emplace_back(L"-spirv");
        }
        return result;
    }

    static std::vector<std::wstring> GetEntryPointArguments(const ShaderCompileInput& input)
    {
        return { L"-E", Utility::StringUtils::ToWideString(input.EntryPoint) };
    }

    static std::vector<std::wstring> GetTargetProfileArguments(const ShaderCompileInput& input)
    {
        return { L"-T", GetDXCTargetProfile(input.Stage) };
    }

    static std::vector<std::wstring> GetIncludePathArguments(const ShaderCompileOptions& option)
    {
        std::vector<std::wstring> result;
        for (const auto& includePath : option.IncludePaths)
        {
            result.emplace_back(L"-I");
            result.emplace_back(Utility::StringUtils::ToWideString(includePath));
        }
        return result;
    }

    static std::vector<std::wstring> GetInternalPreDefinition(const ShaderCompileOptions& option)
    {
        std::vector<std::wstring> result { L"-D" };
        auto def = option.ByteCodeType == ShaderByteCodeType::SPIRV ? std::wstring(L"VULKAN=1") : std::wstring(L"VULKAN=0");
        result.emplace_back(def);
        return result;
    }

    static std::vector<std::wstring> GetDefinitionArguments(const ShaderCompileOptions& options)
    {
        std::vector<std::wstring> result;

        auto preDef = GetInternalPreDefinition(options);
        result.insert(result.end(), preDef.begin(), preDef.end());

        for (const auto& definition : options.Definitions) 
        {
            result.emplace_back(L"-D");
            result.emplace_back(Utility::StringUtils::ToWideString(definition));
        }
        return result;
    }

    static void FillArguments(std::vector<LPCWSTR>& result, const std::vector<std::wstring>& arguments)
    {
        for (const auto& argument : arguments) 
        {
            result.emplace_back(argument.c_str());
        }
    }

    static void CompileDxilOrSpirv(
        const ShaderCompileInput &input,
        const ShaderCompileOptions &options,
        ShaderCompileOutput &output
    )
    {
        ComPtr<IDxcLibrary> library;
        assert(SUCCEEDED(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library))));

        ComPtr<IDxcCompiler3> compiler;
        assert(SUCCEEDED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler))));

        ComPtr<IDxcUtils> utils;
        assert(SUCCEEDED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils))));

        ComPtr<IDxcIncludeHandler> includeHandler;
        assert(SUCCEEDED(utils->CreateDefaultIncludeHandler(&includeHandler)));

        ComPtr<IDxcBlobEncoding> sourceBlob;
        utils->CreateBlobFromPinned(input.Source.c_str(), std::strlen(input.Source.c_str()), CP_UTF8, &sourceBlob);

        std::vector<LPCWSTR> arguments = GetDXCBaseArguments(options);
        auto entryPointArgs = GetEntryPointArguments(input);
        auto targetProfileArgs = GetTargetProfileArguments(input);
        auto includePathArgs = GetIncludePathArguments(options);
        auto definitionArgs = GetDefinitionArguments(options);
        FillArguments(arguments, entryPointArgs);
        FillArguments(arguments, targetProfileArgs);
        FillArguments(arguments, includePathArgs);
        FillArguments(arguments, definitionArgs);

        DxcBuffer srcBuffer;
        srcBuffer.Ptr = sourceBlob->GetBufferPointer();
        srcBuffer.Size = sourceBlob->GetBufferSize();
        srcBuffer.Encoding = 0u;

        ComPtr<IDxcResult> result;
        const HRESULT operationResult = compiler->Compile(
            &srcBuffer,
            arguments.data(),
            static_cast<uint32_t>(arguments.size()),
            ComPtrGet(includeHandler),
            IID_PPV_ARGS(&result)
        );

        ComPtr<IDxcBlobEncoding> errorBlob;
        assert(SUCCEEDED(result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errorBlob), nullptr)));

        if (FAILED(operationResult) || errorBlob->GetBufferSize() > 0)
        {
            output.bSuccess = false;
            output.ErrorMsg.resize(errorBlob->GetBufferSize());
            memcpy(output.ErrorMsg.data(), errorBlob->GetBufferPointer(), errorBlob->GetBufferSize());
            return;
        }

        ComPtr<IDxcBlob> codeBlob;
        assert(SUCCEEDED(result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&codeBlob), nullptr)));
        output.bSuccess = true;
        const auto* codeStart = static_cast<const uint8_t*>(codeBlob->GetBufferPointer());
        const auto* codeEnd = codeStart + codeBlob->GetBufferSize();
        output.ByteCode = std::vector<uint8_t>(codeStart, codeEnd);
    }

    ShaderCompiler &ShaderCompiler::Get()
    {
        static ShaderCompiler instance;
        return instance;
    }

    std::future<ShaderCompileOutput> ShaderCompiler::Compile(const ShaderCompileInput& inInput, const ShaderCompileOptions& inOptions)
    {
        return mThreadPool.EmplaceTask([](const ShaderCompileInput& input, const ShaderCompileOptions& options) -> ShaderCompileOutput
        {
            ShaderCompileOutput output;
            switch (options.ByteCodeType)
            {
            case ShaderByteCodeType::DXIL:
            case ShaderByteCodeType::SPIRV:
                CompileDxilOrSpirv(input, options, output);
                break;
            default:
                assert(false);
                break;
            }
            return output;
        }, inInput, inOptions);
    }

    ShaderCompiler::ShaderCompiler()
        : mThreadPool("ShaderCompiler", 16)
    {
    }
}