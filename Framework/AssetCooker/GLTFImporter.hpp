#pragma once

#include "AssetCooker/Importer.hpp"
#include "AssetCooker/AssetPathUtils.hpp"

#include <EASTL/string.h>
#include <EASTL/vector.h>

struct cgltf_data;
struct cgltf_node;

namespace Assets
{
    // Cgltf-based importer (migrated from ModelLoader.cpp's LoadGLTF family).
    // Produces FImportResult (neutral in-memory form); no GPU work, no disk writes.
    // Cgltf implementation lives in CGLTFImpl.cpp.
    class FGLTFImporter final : public IImporter
    {
    public:
        bool Import(const eastl::string& srcPath, FImportResult& out) override;

    private:
        // Import is split into one stage per FImportResult section; each stage only reads
        // the parsed cgltf data and appends to out (keeps the stages small and independently
        // reviewable, at the cost of passing `data` around).
        void LoadMaterials(cgltf_data* data, const eastl::string& srcDir, FImportResult& out);
        void LoadNodes(cgltf_data* data, FImportResult& out);
        // Returns the traversal roots (default scene, else all scenes, else parentless nodes).
        eastl::vector<const cgltf_node*> LoadRootNodes(cgltf_data* data, FImportResult& out);
        // DFS over the roots; one cgltf_mesh cooks to one range of primitives shared by
        // every referencing node (meshPrimCache).
        void LoadMeshPrimitives(cgltf_data* data, const eastl::vector<const cgltf_node*>& roots, FImportResult& out);
        void LoadSkeleton(cgltf_data* data, FImportResult& out);
        void LoadAnimations(cgltf_data* data, FImportResult& out);
    };
}
