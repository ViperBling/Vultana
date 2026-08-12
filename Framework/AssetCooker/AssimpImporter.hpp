#pragma once

#include "AssetCooker/Importer.hpp"

#include <EASTL/string.h>

namespace Assets
{
    class FAssimpImporter final : public IImporter
    {
    public:
        virtual bool Import(const eastl::string& srcPath, FImportResult& out) override;
    };
}