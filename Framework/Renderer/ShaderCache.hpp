#pragma once

#include "RHI/RHICommon.hpp"

#include <unordered_map>

namespace std
{
    template<>
    struct hash<RHI::RHIShaderDesc>
    {
        size_t operator()(const RHI::RHIShaderDesc& desc) const
        {
            std::string s = desc.File + desc.EntryPoint;
            for (size_t i = 0; i < desc.Defines.size(); i++)
            {
                s += desc.Defines[i];
            }
            return std::hash<std::string>{}(s);
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

        RHI::RHIShader* GetShader(const std::string& file, const std::string& entryPoint, RHI::ERHIShaderType type, const std::vector<std::string>& defines, RHI::ERHIShaderCompileFlags flags);
        std::string GetCachedFileContent(const std::string& file);

        void ReloadShaders();

    private:
        RHI::RHIShader* CreateShader(const std::string& file, const std::string& entryPoint, RHI::ERHIShaderType type, const std::vector<std::string>& defines, RHI::ERHIShaderCompileFlags flags);
        void RecompileShader(RHI::RHIShader* shader);

        std::vector<RHI::RHIShader*> GetShaderList(const std::string& file);
        bool IsFileIncluded(const RHI::RHIShader* shader, const std::string& file);
    
    private:
        RendererBase* mpRenderer = nullptr;
        std::unordered_map<RHI::RHIShaderDesc, std::unique_ptr<RHI::RHIShader>> mCachedShaders;
        std::unordered_map<std::string, std::string> mCachedFile;
    };
}