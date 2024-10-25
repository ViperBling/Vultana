#pragma once

#include "RHI/RHICommon.hpp"

#include <EASTL/hash_map.h>
#include <EASTL/unique_ptr.h>

namespace eastl
{
    template<>
    struct hash<RHI::RHIShaderDesc>
    {
        size_t operator()(const RHI::RHIShaderDesc& desc) const
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
    class RendererBase;

    class ShaderCache
    {
    public:
        ShaderCache(RendererBase* renderer);

        RHI::RHIShader* GetShader(const eastl::string& file, const eastl::string& entryPoint, RHI::ERHIShaderType type, const eastl::vector<eastl::string>& defines, RHI::ERHIShaderCompileFlags flags);
        eastl::string GetCachedFileContent(const eastl::string& file);

        void ReloadShaders();

    private:
        RHI::RHIShader* CreateShader(const eastl::string& file, const eastl::string& entryPoint, RHI::ERHIShaderType type, const eastl::vector<eastl::string>& defines, RHI::ERHIShaderCompileFlags flags);
        void RecompileShader(RHI::RHIShader* shader);

        eastl::vector<RHI::RHIShader*> GetShaderList(const eastl::string& file);
        bool IsFileIncluded(const RHI::RHIShader* shader, const eastl::string& file);
    
    private:
        RendererBase* mpRenderer = nullptr;
        eastl::hash_map<RHI::RHIShaderDesc, eastl::unique_ptr<RHI::RHIShader>> mCachedShaders;
        eastl::hash_map<eastl::string, eastl::string> mCachedFile;
    };
}