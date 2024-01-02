#include "Shader.hpp"

namespace Renderer
{
    ShaderByteCodeStorage &ShaderByteCodeStorage::Get()
    {
        static ShaderByteCodeStorage instance;
        return instance;
    }

    void ShaderByteCodeStorage::UpdateByteCodePackage(IShaderType *shaderTypeKey, std::unordered_map<VariantKey, ShaderByteCode> &&byteCodes)
    {
        mShaderCodePackages[shaderTypeKey] = byteCodes;
    }

    const std::unordered_map<VariantKey, ShaderByteCode> &ShaderByteCodeStorage::GetByteCodePackage(IShaderType *shaderTypeKey)
    {
        auto it = mShaderCodePackages.find(shaderTypeKey);
        assert(it != mShaderCodePackages.end());
        return it->second;
    }

} // namespace Renderer


