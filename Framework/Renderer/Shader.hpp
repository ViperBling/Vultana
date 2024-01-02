#pragma once

#include <vector>
#include <unordered_map>
#include <utility>
#include <tuple>
#include <functional>

#include "RHI/RHICommon.hpp"
#include "RHI/RHIShaderModule.hpp"
#include "RHI/RHIBindGroupLayout.hpp"

#include "Utilities/Hash.hpp"

namespace Vultana
{
    class Shader {};

    using ShaderTypeKey = uint64_t;
    using VariantKey = uint64_t;
    using ShaderByteCode = std::vector<uint8_t>;
    using ShaderStage = RHIShaderStageBits;

    struct ShaderReflectionData
    {
        using LayoutIndex = uint8_t;
        std::unordered_map<std::string, std::pair<LayoutIndex, ResourceBinding>> ResourceBindings;
    };

    class IShaderType
    {
        virtual std::string GetName() = 0;
        virtual ShaderTypeKey GetTypeHash() = 0;
        virtual std::string GetCode() = 0;
        virtual std::vector<VariantKey> GetVariants() = 0;
        virtual std::vector<std::string> GetDefinitions(VariantKey variant) = 0;
    };

    struct ShaderInstance
    {
        bool IsValid() const { return ShaderHandle != nullptr; }

        size_t Hash() const
        {
            if (!IsValid()) { return 0; }
            std::vector<size_t> values = {
                TypeKey,
                VariantKey,
            };
            return HashUtils::CityHash(values.data(), values.size() * sizeof(size_t));
        }

        RHIShaderModule* ShaderHandle = nullptr;
        ShaderTypeKey TypeKey;
        VariantKey VariantKey;
        ShaderReflectionData ReflectionData;
    };

    class ShaderByteCodeStorage
    {
    public:
        NOCOPY(ShaderByteCodeStorage);
        static ShaderByteCodeStorage& Get();
        ShaderByteCodeStorage();
        ~ShaderByteCodeStorage();
        
        void UpdateByteCodePackage(IShaderType* shaderTypeKey, std::unordered_map<VariantKey, ShaderByteCode>&& byteCodes);
        const std::unordered_map<VariantKey, ShaderByteCode>& GetByteCodePackage(IShaderType* shaderTypeKey);

    private:
        std::unordered_map<IShaderType*, std::unordered_map<VariantKey, ShaderByteCode>> mShaderCodePackages;
    };

    class GlobalShader : public Shader {};

    
}