#include "ShaderCache.hpp"
#include "RendererBase.hpp"
#include "PipelineStateCache.hpp"
#include "ShaderCompiler.hpp"

#include "Core/VultanaEngine.hpp"
#include "Utilities/Log.hpp"

#include <fstream>
#include <filesystem>
#include <regex>

namespace RHI
{
    inline bool operator==(const RHI::RHIShaderDesc& lhs, const RHI::RHIShaderDesc& rhs)
    {
        if (lhs.File != rhs.File || lhs.EntryPoint != rhs.EntryPoint || lhs.Type != rhs.Type)
        {
            return false;
        }
        if (lhs.Defines.size() != rhs.Defines.size())
        {
            return false;
        }
        for (size_t i = 0; i < lhs.Defines.size(); i++)
        {
            if (lhs.Defines[i] != rhs.Defines[i])
            {
                return false;
            }
        }
        return true;
    }
}

namespace Renderer
{
    static inline eastl::string LoadFile(const eastl::string& path)
    {
        std::ifstream is;
        is.open(path.c_str(), std::ios::binary);
        if (is.fail())
        {
            return "";
        }

        is.seekg(0, std::ios::end);
        uint32_t length = (uint32_t)is.tellg();
        is.seekg(0, std::ios::beg);

        eastl::string content;
        content.resize(length);

        is.read((char*)content.data(), length);
        is.close();

        return content;
    }

    ShaderCache::ShaderCache(RendererBase *renderer)
    {
        m_pRenderer = renderer;
    }

    RHI::RHIShader *ShaderCache::GetShader(const eastl::string &file, const eastl::string &entryPoint, RHI::ERHIShaderType type, const eastl::vector<eastl::string> &defines, RHI::ERHIShaderCompileFlags flags)
    {
        eastl::string filePath = Core::VultanaEngine::GetEngineInstance()->GetShaderPath() + file;
        eastl::string absolutePath = std::filesystem::absolute(filePath.c_str()).string().c_str();

        RHI::RHIShaderDesc desc;
        desc.Type = type;
        desc.File = absolutePath;
        desc.EntryPoint = entryPoint;
        desc.Defines = defines;
        desc.CompileFlags = flags;

        auto iter = m_CachedShaders.find(desc);
        if (iter != m_CachedShaders.end())
        {
            return iter->second.get();
        }

        RHI::RHIShader* shader = CreateShader(absolutePath, entryPoint, type, defines, flags);
        if (shader != nullptr)
        {
            m_CachedShaders.insert(eastl::make_pair(desc, eastl::unique_ptr<RHI::RHIShader>(shader)));
        }
        return shader;
    }

    eastl::string ShaderCache::GetCachedFileContent(const eastl::string &file)
    {
        auto iter = m_CachedFile.find(file);
        if (iter != m_CachedFile.end())
        {
            return iter->second;
        }

        eastl::string source = LoadFile(file);
        m_CachedFile.insert(eastl::make_pair(file, source));

        return source;
    }

    void ShaderCache::ReloadShaders()
    {
        for (auto iter = m_CachedFile.begin(); iter != m_CachedFile.end(); iter++)
        {
            const eastl::string& path = iter->first;
            const eastl::string& source = iter->second;

            eastl::string newSource = LoadFile(path);

            if (source != newSource)
            {
                m_CachedFile[path] = newSource;

                eastl::vector<RHI::RHIShader*> changedShaders = GetShaderList(path);
                for (size_t i = 0; i < changedShaders.size(); i++)
                {
                    RecompileShader(changedShaders[i]);
                }
            }
        }
    }

    RHI::RHIShader *ShaderCache::CreateShader(const eastl::string &file, const eastl::string &entryPoint, RHI::ERHIShaderType type, const eastl::vector<eastl::string> &defines, RHI::ERHIShaderCompileFlags flags)
    {
        eastl::string source = GetCachedFileContent(file);

        eastl::vector<uint8_t> shaderBlob;
        if (!m_pRenderer->GetShaderCompiler()->Compile(source, file, entryPoint, type, defines, flags, shaderBlob))
        {
            return nullptr;
        }
        RHI::RHIShaderDesc desc;
        desc.Type = type;
        desc.File = file;
        desc.EntryPoint = entryPoint;
        desc.Defines = defines;

        eastl::string name = file + " : " + entryPoint;
        RHI::RHIShader* shader = m_pRenderer->GetDevice()->CreateShader(desc, shaderBlob, name);
        return shader;
    }

    void ShaderCache::RecompileShader(RHI::RHIShader *shader)
    {
        const RHI::RHIShaderDesc& desc = shader->GetDesc();
        VTNA_LOG_INFO("Recompiling shader: {}", desc.File);

        eastl::string source = GetCachedFileContent(desc.File);

        eastl::vector<uint8_t> shaderBlob;
        if (!m_pRenderer->GetShaderCompiler()->Compile(source, desc.File, desc.EntryPoint, desc.Type, desc.Defines, desc.CompileFlags, shaderBlob))
        {
            return;
        }

        shader->Create(shaderBlob);

        PipelineStateCache* psoCache = m_pRenderer->GetPipelineStateCache();
        psoCache->RecreatePSO(shader);
    }

    eastl::vector<RHI::RHIShader *> ShaderCache::GetShaderList(const eastl::string &file)
    {
        eastl::vector<RHI::RHIShader*> shaders;

        for (auto iter = m_CachedShaders.begin(); iter != m_CachedShaders.end(); iter++)
        {
            if (IsFileIncluded(iter->second.get(), file))
            {
                shaders.push_back(iter->second.get());
            }
        }
        return shaders;
    }

    bool ShaderCache::IsFileIncluded(const RHI::RHIShader *shader, const eastl::string &file)
    {
        const RHI::RHIShaderDesc& desc = shader->GetDesc();
        if (desc.File == file)
        {
            return true;
        }

        eastl::string extension = std::filesystem::path(file.c_str()).extension().string().c_str();
        bool isHeader = extension == ".h" || extension == ".hlsli";

        if (isHeader)
        {
            std::string source = GetCachedFileContent(desc.File).c_str();
            std::regex r("#include\\s*\"\\s*\\S+.\\S+\\s*\"");

            std::smatch result;
            while (std::regex_search(source, result, r))
            {
                eastl::string include = result[0].str().c_str();

                size_t first = include.find_first_of('\"');
                size_t last = include.find_last_of('\"');
                eastl::string header = include.substr(first + 1, last - first - 1);

                std::filesystem::path path(desc.File.c_str());
                std::filesystem::path headerPath = path.parent_path() / header.c_str();

                if (std::filesystem::absolute(headerPath).string().c_str() == file)
                {
                    return true;
                }
                source = result.suffix();
            }
        }
        return false;
    }
}