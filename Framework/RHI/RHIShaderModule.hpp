#pragma once

#include "RHICommon.hpp"

namespace RHI
{
    struct ShaderModuleCreateInfo
    {
        std::string EntryPoint;
        const void* Code;
        size_t CodeSize;

        explicit ShaderModuleCreateInfo(const std::string& entryPoint = "", const void* code = nullptr, size_t codeSize = 0)
            : EntryPoint(entryPoint)
            , Code(code)
            , CodeSize(codeSize)
        {}
        explicit ShaderModuleCreateInfo(const std::string& entryPoint = "", const std::vector<uint8_t>& inCode = {})
            : EntryPoint(entryPoint)
            , Code(inCode.data())
            , CodeSize(inCode.size())
        {}

        ShaderModuleCreateInfo& SetCode(const void* inCode)
        {
            Code = inCode;
            return *this;
        }
        ShaderModuleCreateInfo& SetCodeSize(size_t inCodeSize)
        {
            CodeSize = inCodeSize;
            return *this;
        }
    };

    class RHIShaderModule
    {
    public:
        NOCOPY(RHIShaderModule)
        virtual ~RHIShaderModule() = default;

        virtual const std::string& GetEntryPoint() const = 0;

    protected:
        explicit RHIShaderModule(const ShaderModuleCreateInfo& createInfo) {}
    };
}