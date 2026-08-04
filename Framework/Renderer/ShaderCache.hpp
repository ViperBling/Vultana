#pragma once

#include "RHI/RHICommon.hpp"

#include <EASTL/hash_map.h>
#include <EASTL/unique_ptr.h>

namespace eastl
{
    template<>
    struct hash<RHI::FRHIShaderDesc>
    {
        size_t operator()(const RHI::FRHIShaderDesc& desc) const
        {
            eastl::string s = desc.File + desc.EntryPoint;
            for (size_t i = 0; i < desc.Defines.size(); i++)
            {
                s += desc.Defines[i];
            }
            return eastl::hash<eastl::string>{}(s);
        }
    };
}

namespace Renderer
{
    class FRendererBase;

    class FShaderCache
    {
    public:
        FShaderCache(FRendererBase* renderer);

        RHI::FRHIShader* GetShader(const eastl::string& file, const eastl::string& entryPoint, RHI::ERHIShaderType type, const eastl::vector<eastl::string>& defines, RHI::ERHIShaderCompileFlags flags);
        eastl::string GetCachedFileContent(const eastl::string& file);

        void ReloadShaders();

    private:
        RHI::FRHIShader* CreateShader(const eastl::string& file, const eastl::string& entryPoint, RHI::ERHIShaderType type, const eastl::vector<eastl::string>& defines, RHI::ERHIShaderCompileFlags flags);
        void RecompileShader(RHI::FRHIShader* shader);

        eastl::vector<RHI::FRHIShader*> GetShaderList(const eastl::string& file);
        bool IsFileIncluded(const RHI::FRHIShader* shader, const eastl::string& file);
    
    private:
        FRendererBase* m_pRenderer = nullptr;
        eastl::hash_map<RHI::FRHIShaderDesc, eastl::unique_ptr<RHI::FRHIShader>> m_CachedShaders;
        eastl::hash_map<eastl::string, eastl::string> m_CachedFile;
    };
}