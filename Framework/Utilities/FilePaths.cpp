#include "FilePaths.hpp"

namespace Utility
{
    std::filesystem::path FFilePaths::m_WorkingDir = std::filesystem::path();
    std::filesystem::path FFilePaths::m_CurrentProjectFile = std::filesystem::path();

    std::filesystem::path FFilePaths::WorkingDir()
    {
        if (m_WorkingDir.empty())
        {
            m_WorkingDir = std::filesystem::current_path();
        }
        return m_WorkingDir;
    }

    std::filesystem::path FFilePaths::EngineRoot()
    {
        return WorkingDir().parent_path();
    }

    std::filesystem::path FFilePaths::EngineShaderPath()
    {
        return EngineAssetPath() / "Shaders";
    }

    std::filesystem::path FFilePaths::EngineAssetPath()
    {
        return EngineRoot() / "Assets";
    }

    std::filesystem::path FFilePaths::EngineBinariesPath()
    {
        return EngineRoot() / "Binary";
    }

    std::filesystem::path FFilePaths::EnginePluginPath()
    {
        return EngineRoot() / "Plugins";
    }

    std::filesystem::path FFilePaths::EnginePluginAssetPath(const std::string& pluginName)
    {
        return EnginePluginPath() / pluginName / "Assets";
    }

    std::filesystem::path FFilePaths::ProjectFile()
    {
        return m_CurrentProjectFile;
    }

    std::filesystem::path FFilePaths::ProjectRoot()
    {
        return m_CurrentProjectFile.parent_path();
    }

    std::filesystem::path FFilePaths::ProjectAssetPath()
    {
        return ProjectRoot() / "Asset";
    }

    std::filesystem::path FFilePaths::ProjectBinariesPath()
    {
        return ProjectRoot() / "Binaries";
    }

    std::filesystem::path FFilePaths::ProjectPluginPath()
    {
        return ProjectRoot() / "Plugin";
    }

    std::filesystem::path FFilePaths::ProjectPluginAssetPath(const std::string& pluginName)
    {
        return ProjectPluginPath() / pluginName / "Asset";
    }
}