#include "ShaderCompiler.hpp"
#include "RendererBase.hpp"
#include "ShaderCache.hpp"

#include "Core/VultanaEngine.hpp"
#include "Utilities/Log.hpp"

#include <filesystem>
#include <atlbase.h>
#include <dxcapi.h>

namespace Renderer
{
    class DXCIncludeHandler : public IDxcIncludeHandler
    {
    public:
        DXCIncludeHandler(ShaderCache* pShaderCache, IDxcUtils* pDxcUtils)
            : mpShaderCache(pShaderCache)
            , mpDxcUtils(pDxcUtils)
        {}

        HRESULT STDMETHODCALLTYPE LoadSource(LPCWSTR pFilename, IDxcBlob** ppIncludeSource) override
        {
            std::string absPath = std::filesystem::absolute(pFilename).string();
            std::string source = mpShaderCache->GetCachedFileContent(absPath);

            *ppIncludeSource = nullptr;
            return mpDxcUtils->CreateBlob(source.data(), (UINT32)source.size(), CP_UTF8, reinterpret_cast<IDxcBlobEncoding**>(ppIncludeSource));
        }

        ULONG STDMETHODCALLTYPE AddRef() override
        {
            ++mRef;
            return mRef;
        }

        ULONG STDMETHODCALLTYPE Release() override
        {
            --mRef;
            ULONG result = mRef;
            if (result == 0)
            {
                delete this;
            }
            return result;
        }

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override
        {
            if (IsEqualIID(riid, __uuidof(IDxcIncludeHandler)))
            {
                *ppvObject = dynamic_cast<IDxcIncludeHandler*>(this);
                this->AddRef();
                return S_OK;
            }
            else if (IsEqualIID(riid, __uuidof(IUnknown)))
            {
                *ppvObject = dynamic_cast<IUnknown*>(this);
                this->AddRef();
                return S_OK;
            }
            else
            {
                return E_NOINTERFACE;
            }
        }
    
    private:
        ShaderCache* mpShaderCache = nullptr;
        IDxcUtils* mpDxcUtils = nullptr;
        std::atomic<ULONG> mRef = 0;
    };

    inline const wchar_t* GetShaderProfile(RHI::ERHIShaderType type)
    {
        switch (type)
        {
        case RHI::ERHIShaderType::AS:
            return L"as_6_6";
        case RHI::ERHIShaderType::MS:
            return L"ms_6_6";
        case RHI::ERHIShaderType::VS:
            return L"vs_6_6";
        case RHI::ERHIShaderType::PS:
            return L"ps_6_6";
        case RHI::ERHIShaderType::CS:
            return L"cs_6_6";
        default:
            return L"";
        }
    }

    ShaderCompiler::ShaderCompiler(RendererBase *renderer) : mpRenderer(renderer)
    {
        HMODULE dxcModule = LoadLibrary("dxcompiler.dll");

        if (dxcModule)
        {
            DxcCreateInstanceProc dxcCreateInstance = (DxcCreateInstanceProc)GetProcAddress(dxcModule, "DxcCreateInstance");

            DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&mpDxcUtils));
            DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&mpDxcCompiler));

            mpDxcIncludeHandler = new DXCIncludeHandler(renderer->GetShaderCache(), mpDxcUtils);
            mpDxcIncludeHandler->AddRef();
        }
    }

    ShaderCompiler::~ShaderCompiler()
    {
        if (mpDxcIncludeHandler)
        {
            mpDxcIncludeHandler->Release();
        }
        if (mpDxcCompiler)
        {
            mpDxcCompiler->Release();
        }
        if (mpDxcUtils)
        {
            mpDxcUtils->Release();
        }
    }

    bool ShaderCompiler::Compile(const std::string &source, const std::string &file, const std::string &entryPoint, RHI::ERHIShaderType type, const std::vector<std::string> &defines, RHI::ERHIShaderCompileFlags flags, std::vector<uint8_t> &output)
    {
        DxcBuffer sourceBuffer;
        sourceBuffer.Ptr = source.data();
        sourceBuffer.Size = source.length();
        sourceBuffer.Encoding = DXC_CP_ACP;

        std::wstring wFile = std::wstring(file.begin(), file.end());
        std::wstring wEntryPoint = std::wstring(entryPoint.begin(), entryPoint.end());
        std::wstring wProfile = GetShaderProfile(type);

        std::vector<std::wstring> wstrDefines;
        for (size_t i = 0; i < defines.size(); i++)
        {
            wstrDefines.push_back(std::wstring(defines[i].begin(), defines[i].end()));
        }

        std::vector<LPCWSTR> arguments;
        arguments.push_back(wFile.c_str());
        arguments.push_back(L"-E");     arguments.push_back(wEntryPoint.c_str());
        arguments.push_back(L"-T");     arguments.push_back(wProfile.c_str());
        for (size_t i = 0; i < wstrDefines.size(); i++)
        {
            arguments.push_back(L"-D"); arguments.push_back(wstrDefines[i].c_str());
        }
        switch (mpRenderer->GetDevice()->GetDesc().RenderBackend)
        {
        case RHI::ERHIRenderBackend::Vulkan:
            arguments.push_back(L"-D");
            arguments.push_back(L"RHI_BACKEND_VULKAN=1");
            arguments.push_back(L"-spirv");
            arguments.push_back(L"-fspv-target-env=vulkan1.3");
            arguments.push_back(L"-fvk-use-dx-layout");
            // arguments.push_back(L"-fvk-bind-counter-heap");
            // arguments.push_back(L"0");
            // arguments.push_back(L"0");
            arguments.push_back(L"-fvk-bind-resource-heap");
            arguments.push_back(L"0");
            arguments.push_back(L"1");
            // arguments.push_back(L"-fvk-bind-sampler-heap");
            // arguments.push_back(L"0");
            // arguments.push_back(L"2");
            break;
        default:
            break;
        }

        arguments.push_back(L"-HV 2021");
        arguments.push_back(L"-enable-16bit-types");

        #ifdef _DEBUG
            arguments.push_back(L"-Zi");
            arguments.push_back(L"-Qembed_debug");
        #endif

        if (flags & RHI::RHIShaderCompileFlagO3)
        {
            arguments.push_back(L"-O3");
        }
        else if (flags & RHI::RHIShaderCompileFlagO3)
        {
            arguments.push_back(L"-O2");
        }
        else if (flags & RHI::RHIShaderCompileFlagO1)
        {
            arguments.push_back(L"-O1");
        }
        else if (flags & RHI::RHIShaderCompileFlagO0)
        {
            arguments.push_back(L"-O0");
        }
        else
        {
        #ifdef _DEBUG
            arguments.push_back(L"-O0");
        #else
            arguments.push_back(L"-O3");
        #endif
        }

        CComPtr<IDxcResult> pResult;
        mpDxcCompiler->Compile(&sourceBuffer, arguments.data(), (UINT32)arguments.size(), mpDxcIncludeHandler, IID_PPV_ARGS(&pResult));
        
        CComPtr<IDxcBlobUtf8> pError = nullptr;
        pResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pError), nullptr);
        if (pError != nullptr && pError->GetStringLength() != 0)
        {
            VTNA_LOG_ERROR(pError->GetStringPointer());
        }

        HRESULT hr;
        pResult->GetStatus(&hr);
        if (FAILED(hr))
        {
            VTNA_LOG_ERROR("[ShaderCompiler] Failed to compile shader: {}, {}", file, entryPoint);
            return false;
        }

        CComPtr<IDxcBlob> pShader = nullptr;
        if (FAILED(pResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pShader), nullptr)))
        {
            VTNA_LOG_ERROR("[ShaderCompiler] Failed to get shader object: {}, {}", file, entryPoint);
            return false;
        }

        output.resize(pShader->GetBufferSize());
        memcpy(output.data(), pShader->GetBufferPointer(), pShader->GetBufferSize());

        return true;
    }
}