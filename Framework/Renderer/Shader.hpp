#pragma once

#include <vector>
#include <unordered_map>
#include <utility>
#include <tuple>
#include <functional>

#include "RHI/RHICommon.hpp"
#include "RHI/RHIDevice.hpp"
#include "RHI/RHIShaderModule.hpp"
#include "RHI/RHIBindGroupLayout.hpp"

#include "Utilities/Hash.hpp"
#include "Utilities/FilePaths.hpp"
#include "Utilities/String.hpp"

namespace Renderer
{
    class Shader {};

    using ShaderTypeKey = uint64_t;
    using VariantKey = uint64_t;
    using ShaderByteCode = std::vector<uint8_t>;
    using ShaderStage = RHI::RHIShaderStageBits;

    struct ShaderReflectionData
    {
        using LayoutIndex = uint8_t;
        std::unordered_map<std::string, std::pair<LayoutIndex, RHI::ResourceBinding>> ResourceBindings;
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
            return Utility::HashUtils::CityHash(values.data(), values.size() * sizeof(size_t));
        }

        RHI::RHIShaderModule* ShaderHandle = nullptr;
        ShaderTypeKey TypeKey;
        VariantKey VariantKey;
        ShaderReflectionData ReflectionData;
    };

    class ShaderByteCodeStorage
    {
    public:
        NOCOPY(ShaderByteCodeStorage);
        static ShaderByteCodeStorage& Get();
        ShaderByteCodeStorage() = default;
        ~ShaderByteCodeStorage() = default;
        
        void UpdateByteCodePackage(IShaderType* shaderTypeKey, std::unordered_map<VariantKey, ShaderByteCode>&& byteCodes);
        const std::unordered_map<VariantKey, ShaderByteCode>& GetByteCodePackage(IShaderType* shaderTypeKey);

    private:
        std::unordered_map<IShaderType*, std::unordered_map<VariantKey, ShaderByteCode>> mShaderCodePackages;
    };

    class GlobalShader : public Shader {};

    template <typename ShaderType>
    class GlobalShaderType : public IShaderType
    {
    public:
        NOCOPY(GlobalShaderType);
        static GlobalShaderType& Get();
        GlobalShaderType();
        ~GlobalShaderType() = default;

        std::string GetName() override { return ShaderType::Name; }
        ShaderTypeKey GetTypeHash() override { return Utility::HashUtils::CityHash(ShaderType::Name, sizeof(ShaderType::Name)); }
        std::string GetCode() override;
        std::vector<VariantKey> GetVariants() override;
        std::vector<std::string> GetDefinitions(VariantKey variant) override;

    private:
        void ComputeVariantDefinitions();

    private:
        std::unordered_map<VariantKey, std::vector<std::string>> mVariantDefinitions;
    };

    template <typename T>
    class GlobalShaderMap
    {
    public:
        NOCOPY(GlobalShaderMap);
        static GlobalShaderMap& Get(RHI::RHIDevice& inDevice);
        ~GlobalShaderMap() = default;

        void InValidate();
        ShaderInstance GetShaderInstance(const typename T::VariantSet& variantSet);

    private:
        explicit GlobalShaderMap(RHI::RHIDevice& inDevice);
        [[nodiscard]] const ShaderByteCode& GetByteCode(const typename T::VariantSet& variantSet) const;

    private:
        RHI::RHIDevice& mDevice;
        std::unordered_map<VariantKey, std::unique_ptr<RHI::RHIShaderModule>> mShaderModules;
    };

    class GlobalShaderRegistry
    {
    public:
        NOCOPY(GlobalShaderRegistry);

        static GlobalShaderRegistry& Get()
        {
            static GlobalShaderRegistry instance;
            return instance;
        }

        GlobalShaderRegistry() = default;
        ~GlobalShaderRegistry() = default;

        template <typename ShaderType>
        void RegisterShaderType() { mShaderTypes.emplace_back(&GlobalShaderType<ShaderType>::Get()); }

        template <typename ShaderType>
        std::vector<ShaderType>&& GetShaderType() { return std::move(mShaderTypes); }
    
    private:
        std::vector<IShaderType*> mShaderTypes;
    };

    class BoolShaderVariantFieldImpl
    {
    public:
        using ValueType = bool;

        static constexpr std::pair<uint32_t, uint32_t> ValueRange = { 0, 1 };
        static constexpr ValueType DefaultValue = false;

        BoolShaderVariantFieldImpl() : mValue(static_cast<uint32_t>(DefaultValue)) {}
        BoolShaderVariantFieldImpl(BoolShaderVariantFieldImpl&& other) noexcept : mValue(other.mValue) {}
        ~BoolShaderVariantFieldImpl() = default;

        void Set(ValueType inValue) { mValue = inValue ? 1 : 0; }
        [[nodiscard]] ValueType Get() const { return mValue == 1; }
        [[nodiscard]] uint32_t GetNumberValue() const { return mValue; }

    private:
        uint32_t mValue;
    };

    template <uint32_t From, uint32_t To>
    class RangedIntShaderVariantFieldImpl
    {
    public:
        using ValueType = uint32_t;

        static constexpr std::pair<uint32_t, uint32_t> ValueRange = { From, To };
        static constexpr ValueType DefaultValue = From;

        RangedIntShaderVariantFieldImpl() : mValue(DefaultValue) {}
        RangedIntShaderVariantFieldImpl(RangedIntShaderVariantFieldImpl&& other) noexcept : mValue(other.mValue) {}
        ~RangedIntShaderVariantFieldImpl() = default;

        void Set(ValueType inValue)
        {
            assert(From <= inValue && inValue <= To);
            mValue = inValue;
        }

        [[nodiscard]] ValueType Get() const { return mValue; }
        [[nodiscard]] uint32_t GetNumberValue() const { return mValue; }

    private:
        uint32_t mValue;
    };

    template <typename... Variants>
    class VariantSetImpl
    {
    public:
        VariantSetImpl();
        VariantSetImpl(const VariantSetImpl& other);
        VariantSetImpl(VariantSetImpl&& other) noexcept : mVariants(std::move(other.mVariants)) {}
        ~VariantSetImpl() = default;

        static uint32_t VariantNum();

        template <typename F>
        static void TraverseAll(F&& func);

        template <typename T>
        void Set(typename T::ValueType inValue) { std::get<T>(mVariants).Set(inValue); }

        template <typename T>
        typename T::ValueType Get() const { return std::get<T>(mVariants).Get(); }

        [[nodiscard]] std::vector<std::string> ComputeDefinitions() const;
        [[nodiscard]] VariantKey Hash() const;

    private:
        std::tuple<Variants...> mVariants;
    };
}

#define ShaderInfo(inName, inSrcFile, inEntryPoint, inStage)    \
    static constexpr const char* Name = inName;                 \
    static constexpr const char* SrcFile = inSrcFile;           \
    static constexpr const char* EntryPoint = inEntryPoint;     \
    static constexpr RHI::RHIShaderStageBits Stage = inStage;   \

#define DefaultVariantFilter                                                   \
    static bool VariantFilter(const VariantSet& inVariantSet) { return true; } \

#define BoolShaderVariantField(inClass, inMarco)                    \
    struct inClass : public Renderer::BoolShaderVariantFieldImpl    \
    {                                                               \
        static constexpr const char* Name = #inClass;               \
        static constexpr const char* Macro = inMarco;               \
    };                                                              \

#define RangedIntShaderVariantField(inClass, inMarco, inRangeFrom, inRangeTo)                   \
    struct inClass : public Renderer::RangedIntShaderVariantFieldImpl<inRangeFrom, inRangeTo>   \
    {                                                                                           \
        static constexpr const char* Name = #inClass;                                           \
        static constexpr const char* Macro = inMarco;                                           \
    };                                                                                          \

#define VariantSet(...)                                                         \
    class VariantSet : public Renderer::VariantSetImpl<__VA_ARGS__> {};         \

#define RegisterGlobalShader(inClass)                                           \
    static uint8_t _globalShaderRegister_##inClass = []() -> uint8_t            \
    {                                                                           \
        Renderer::GlobalShaderRegistry::Get().RegisterShaderType<inClass>();    \
        return 0;                                                               \
    }();                                                                        \

#include "Shader.inl"