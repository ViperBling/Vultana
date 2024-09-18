#include "ShaderCache.hpp"
#include "RendererBase.hpp"
#include "PipelineStateCache.hpp"
#include "ShaderCompiler.hpp"
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
    static inline std::string LoadFile(const std::string& path)
    {
        std::ifstream is;
        is.open(path.c_str(), std::ios::binary);
        if (!is.fail())
        {
            return "";
        }

        is.seekg(0, std::ios::end);
        uint32_t length = (uint32_t)is.tellg();
        is.seekg(0, std::ios::beg);

        std::string content;
        content.resize(length);

        is.read((char*)content.data(), length);
        is.close();

        return content;
    }

    ShaderCache::ShaderCache(RendererBase *renderer)
    {
        mpRenderer = renderer;
    }

    RHI::RHIShader *ShaderCache::GetShader(const std::string &file, const std::string &entryPoint, RHI::ERHIShaderType type, const std::vector<std::string> &defines, RHI::ERHIShaderCompileFlags flags)
    {
        std::string filePath = "./Assets/Shaders/" + file;
        std::string absolutePath = std::filesystem::absolute(filePath.c_str()).string().c_str();

        RHI::RHIShaderDesc desc;
        desc.Type = type;
        desc.File = absolutePath;
        desc.EntryPoint = entryPoint;
        desc.Defines = defines;
        desc.CompileFlags = flags;

        auto iter = mCachedShaders.find(desc);
        if (iter != mCachedShaders.end())
        {
            return iter->second.get();
        }

        RHI::RHIShader* shader = CreateShader(absolutePath, entryPoint, type, defines, flags);
        if (shader != nullptr)
        {
            mCachedShaders.insert(std::make_pair(desc, std::unique_ptr<RHI::RHIShader>(shader)));
        }
        return shader;
    }

    std::string ShaderCache::GetCachedFileContent(const std::string &file)
    {
        auto iter = mCachedFile.find(file);
        if (iter != mCachedFile.end())
        {
            return iter->second;
        }

        std::string source = LoadFile(file);
        mCachedFile.insert(std::make_pair(file, source));

        return source;
    }

    void ShaderCache::ReloadShaders()
    {
        for (auto iter = mCachedFile.begin(); iter != mCachedFile.end(); iter++)
        {
            const std::string& path = iter->first;
            const std::string& source = iter->second;

            std::string newSource = LoadFile(path);

            if (source != newSource)
            {
                mCachedFile[path] = newSource;

                std::vector<RHI::RHIShader*> changedShaders = GetShaderList(path);
                for (size_t i = 0; i < changedShaders.size(); i++)
                {
                    RecompileShader(changedShaders[i]);
                }
            }
        }
    }

    RHI::RHIShader *ShaderCache::CreateShader(const std::string &file, const std::string &entryPoint, RHI::ERHIShaderType type, const std::vector<std::string> &defines, RHI::ERHIShaderCompileFlags flags)
    {
        std::string source = GetCachedFileContent(file);

        std::vector<uint8_t> shaderBlob;
        if (!mpRenderer->GetShaderCompiler()->Compile(source, file, entryPoint, type, defines, flags, shaderBlob))
        {
            return nullptr;
        }
        RHI::RHIShaderDesc desc;
        desc.Type = type;
        desc.File = file;
        desc.EntryPoint = entryPoint;
        desc.Defines = defines;

        std::string name = file + " : " + entryPoint;
        RHI::RHIShader* shader = mpRenderer->GetDevice()->CreateShader(desc, shaderBlob, name);
        return shader;
    }

    void ShaderCache::RecompileShader(RHI::RHIShader *shader)
    {
        const RHI::RHIShaderDesc& desc = shader->GetDesc();
        VTNA_LOG_INFO("Recompiling shader: {}", desc.File);

        std::string source = GetCachedFileContent(desc.File);

        std::vector<uint8_t> shaderBlob;
        if (!mpRenderer->GetShaderCompiler()->Compile(source, desc.File, desc.EntryPoint, desc.Type, desc.Defines, desc.CompileFlags, shaderBlob))
        {
            return;
        }

        shader->Create(shaderBlob);

        PipelineStateCache* psoCache = mpRenderer->GetPipelineStateCache();
        psoCache->RecreatePSO(shader);
    }

    std::vector<RHI::RHIShader *> ShaderCache::GetShaderList(const std::string &file)
    {
        std::vector<RHI::RHIShader*> shaders;

        for (auto iter = mCachedShaders.begin(); iter != mCachedShaders.end(); iter++)
        {
            if (IsFileIncluded(iter->second.get(), file))
            {
                shaders.push_back(iter->second.get());
            }
        }
        return shaders;
    }

    bool ShaderCache::IsFileIncluded(const RHI::RHIShader *shader, const std::string &file)
    {
        const RHI::RHIShaderDesc& desc = shader->GetDesc();
        if (desc.File == file)
        {
            return true;
        }

        std::string extension = std::filesystem::path(file.c_str()).extension().string().c_str();
        bool isHeader = extension == ".h" || extension == ".hlsli";

        if (isHeader)
        {
            std::string source = GetCachedFileContent(desc.File);
            std::regex r("#include\\s*\"\\s*\\S+.\\S+\\s*\"");

            std::smatch result;
            while (std::regex_search(source, result, r))
            {
                std::string include = result[0].str();

                size_t first = include.find_first_of('\"');
                size_t last = include.find_last_of('\"');
                std::string header = include.substr(first + 1, last - first - 1);

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