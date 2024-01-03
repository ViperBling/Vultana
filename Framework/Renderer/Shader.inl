#pragma once

#include "Shader.hpp"

namespace Renderer
{
    

    template <typename ShaderType>
    inline GlobalShaderType<ShaderType>::GlobalShaderType()
    {
        ComputeVariantDefinitions();
    }

    template <typename ShaderType>
    inline std::string GlobalShaderType<ShaderType>::GetCode()
    {
        static std::unordered_map<std::string, std::string> pathMap = {
            { "/Engine/Shader", Utility::FilePaths::EngineShaderPath().string() }
        };

        std::string sourceFiles = ShaderType::SourceFiles;
        for (const auto& it : pathMap)
        {
            if (sourceFiles.starts_with(it.first))
            {
                return Utility::FileUtils::ReadTextFile(Utility::StringUtils::Replace(sourceFiles, it.first, it.second));
            }
        }
        assert(false && "Shader source file not found");
        return "";
    }

    template <typename ShaderType>
    inline std::vector<VariantKey> GlobalShaderType<ShaderType>::GetVariants()
    {
        std::vector<VariantKey> keys;
        {
            keys.reserve(mVariantDefinitions.size());
            for (const auto& it : mVariantDefinitions)
            {
                keys.emplace_back(it.first);
            }
        }
        return keys;
    }

    template <typename ShaderType>
    inline std::vector<std::string> GlobalShaderType<ShaderType>::GetDefinitions(VariantKey variant)
    {
        auto it = mVariantDefinitions.find(variant);
        assert(it != mVariantDefinitions.end());
        return it->second;
    }

    template <typename ShaderType>
    inline void GlobalShaderType<ShaderType>::ComputeVariantDefinitions()
    {
        mVariantDefinitions.reserve(ShaderType::VariantSet::VariantNum());
        ShaderType::VariantSet::TraverseAll([this](auto&& variantSetImpl) -> void
        {
            const auto* variantSet = static_cast<typename ShaderType::VariantSet*>(&variantSetImpl);
            if (!ShaderType::VariantFilter(*variantSet)) { return; }
            mVariantDefinitions[variantSet->Hash()] = variantSet->ComputeDefinitions();
        });
    }

    template <typename T>
    inline GlobalShaderMap<T> &GlobalShaderMap<T>::Get(RHI::RHIDevice& inDevice)
    {
        static std::unordered_map<RHI::RHIDevice*, std::unique_ptr<GlobalShaderMap<T>>> sGlobalShaderMaps;
        auto it = sGlobalShaderMaps.find(&inDevice);
        if (it == sGlobalShaderMaps.end())
        {
            sGlobalShaderMaps[&inDevice] = std::unique_ptr<GlobalShaderMap<T>>(new GlobalShaderMap<T>(inDevice));
        }
        return *sGlobalShaderMaps[&inDevice];
    }

    template <typename T>
    inline void GlobalShaderMap<T>::InValidate()
    {
        auto variantNum = T::VariantSet::VariantNum();
        mShaderModules.clear();
        mShaderModules.reserve(variantNum);
    }

    template <typename T>
    inline ShaderInstance GlobalShaderMap<T>::GetShaderInstance(const typename T::VariantSet &variantSet)
    {
        auto variantKey = variantSet.Hash();
        auto it = mShaderModules.find(variantKey);
        if (it != mShaderModules.end())
        {
            const auto& shaderCode = GetByteCode(variantSet);
            RHI::ShaderModuleCreateInfo smCI {};
            smCI.Code = shaderCode.data();
            smCI.CodeSize = shaderCode.size();
            mShaderModules[variantKey] = mDevice.CreateShaderModule(smCI);
        }
        ShaderInstance result;
        result.ShaderHandle = mShaderModules[variantKey].get();
        result.TypeKey = GlobalShaderType<T>::Get().GetTypeHash();
        result.VariantKey = variantKey;
        return result;
    }

    template <typename T>
    inline GlobalShaderMap<T>::GlobalShaderMap(RHI::RHIDevice &inDevice)
        : mDevice(inDevice)
    {
        auto variantNum = T::VariantSet::VariantNum();
        mShaderModules.reserve(variantNum);
    }

    template <typename T>
    inline const ShaderByteCode &GlobalShaderMap<T>::GetByteCode(const typename T::VariantSet &variantSet) const
    {
        const auto& byteCodePack = ShaderByteCodeStorage::Get().GetByteCodePackage(&GlobalShaderType<T>::Get());
        auto it = byteCodePack.find(variantSet.Hash());
        assert(it != byteCodePack.end());
        return it->second;
    }

    template <typename... Variants>
    inline VariantSetImpl<Variants...>::VariantSetImpl()
    {
        (void) std::initializer_list<int> 
        {
            ([this]() -> void 
            {
                std::get<Variants>(mVariants).Set(Variants::DefaultValue);
            }(), 0)...
        };
    }
    template <typename... Variants>
    inline VariantSetImpl<Variants...>::VariantSetImpl(const VariantSetImpl &other)
    {
        (void) std::initializer_list<int> 
        {
            ([this, &other]() -> void 
            {
                std::get<Variants>(mVariants).Set(std::get<Variants>(other.mVariants).Get());
            }(), 0)...
        };
    }

    template <typename... Variants>
    inline uint32_t VariantSetImpl<Variants...>::VariantNum()
    {
        uint32_t res = 1;
        (void) std::initializer_list<int> 
        {
            ([&res]() -> void 
            {
                auto valueRange = Variants::ValueRange;
                assert(valueRange.first <= valueRange.second);
                res *= (valueRange.second - valueRange.first + 1);
            }(), 0)...
        };
        return res;
    }
    
    template <typename... Variants>
    template <typename F>
    inline void VariantSetImpl<Variants...>::TraverseAll(F &&func)
    {
        std::vector<VariantSetImpl<Variants...>> variantSets;
        variantSets.reserve(VariantNum());
        variantSets.emplace_back(VariantSetImpl<Variants...>());

        (void) std::initializer_list<int> 
        {
            ([&variantSets]() -> void 
            {
                auto valueRange = Variants::ValueRange;
                auto variantSetSize = variantSets.size();
                for (auto i = valueRange.first; i <= valueRange.second; ++i)
                {
                    if (i == valueRange.first)
                    {
                        for (auto j = 0; j < variantSetSize; ++j)
                        {
                            variantSets[j].template Set<Variants>(static_cast<typename Variants::ValueType>(i));
                        }
                    }
                    else
                    {
                        for (auto j = 0; j < variantSetSize; ++j)
                        {
                            variantSets.emplace_back(variantSets[j]);
                            variantSets.back().template Set<Variants>(static_cast<typename Variants::ValueType>(i));
                        }
                    }
                }
            }(), 0)... 
        };
        for (auto& set : variantSets) { func(set); }
    }

    template <typename... Variants>
    inline std::vector<std::string> VariantSetImpl<Variants...>::ComputeDefinitions() const
    {
        std::vector<std::string> res;
        res.reserve(sizeof...(Variants));
        (void) std::initializer_list<int> 
        {
            ([&res, this]() -> void 
            {
                res.emplace_back(std::string(Variants::Marco) + "=" + std::to_string(std::get<Variants>(mVariants).GetNumberValue()));
            }(), 0)...
        };
        return res;
    }

    template <typename... Variants>
    inline VariantKey VariantSetImpl<Variants...>::Hash() const
    {
        return Utility::HashUtils::CityHash(&mVariants, sizeof(std::tuple<Variants...>));
    }
}