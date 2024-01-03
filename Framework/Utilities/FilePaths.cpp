#include "FilePaths.hpp"

namespace Utility
{
    static std::filesystem::path mWorkingDir = std::filesystem::path();
    static std::filesystem::path mCurrentProjectFile = std::filesystem::path();

    std::filesystem::path FilePaths::WorkingDir()
    {
        if (mWorkingDir.empty())
        {
            mWorkingDir = std::filesystem::current_path();
        }
        return mWorkingDir;
    }

    std::filesystem::path FilePaths::EngineRoot()
    {
        return WorkingDir().parent_path();
    }

    std::filesystem::path FilePaths::EngineShaderPath()
    {
        return EngineAssetPath() / "Shaders";
    }

    std::filesystem::path FilePaths::EngineAssetPath()
    {
        return EngineRoot() / "Assets";
    }

    std::filesystem::path FilePaths::EngineBinariesPath()
    {
        return EngineRoot() / "Binary";
    }

    std::filesystem::path FilePaths::EnginePluginPath()
    {
        return EngineRoot() / "Plugins";
    }

    std::filesystem::path FilePaths::EnginePluginAssetPath(const std::string& pluginName)
    {
        return EnginePluginPath() / pluginName / "Assets";
    }

    std::filesystem::path FilePaths::ProjectFile()
    {
        return mCurrentProjectFile;
    }

    std::filesystem::path FilePaths::ProjectRoot()
    {
        return mCurrentProjectFile.parent_path();
    }

    std::filesystem::path FilePaths::ProjectAssetPath()
    {
        return ProjectRoot() / "Asset";
    }

    std::filesystem::path FilePaths::ProjectBinariesPath()
    {
        return ProjectRoot() / "Binaries";
    }

    std::filesystem::path FilePaths::ProjectPluginPath()
    {
        return ProjectRoot() / "Plugin";
    }

    std::filesystem::path FilePaths::ProjectPluginAssetPath(const std::string& pluginName)
    {
        return ProjectPluginPath() / pluginName / "Asset";
    }
}