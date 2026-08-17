#include "AssetManager/ModelLoaderECS.hpp"
#include "Scene/ECS/EntitySubsystem.hpp"
#include "Scene/ECS/ProcessorBase.hpp"
#include "Scene/ECS/Components.hpp"
#include "Scene/ECS/SceneSerializer.hpp"
#include "Scene/ECS/Processors/HierarchyProcessor.hpp"
#include "Scene/ECS/Processors/CullingProcessor.hpp"
#include "Scene/ECS/Processors/RenderSubmissionProcessor.hpp"
#include "Scene/ECS/Processors/AnimationProcessor.hpp"
#include "Scene/ECS/Processors/LightProcessor.hpp"
#include "Scene/ECS/RenderSink.hpp"
#include "Scene/ECS/EntityFactory.hpp"
#include "Editor/Commands/EntityEditableTarget.hpp"
#include "Editor/Commands/CommandHistory.hpp"
#include "Editor/Commands/TransformCommand.hpp"

#include "Tests/FakeAssetRegistry.hpp"
#include "Tests/TestUtilities.hpp"

#include <gtest/gtest.h>
#include <entt/entt.hpp>
#include <cstdio>

using namespace Scene::ECS;

// ============================================================
//  Helpers
// ============================================================
class FTestProcessor : public FProcessorBase
{
public:
    explicit FTestProcessor(EProcessingPhase phase, int& counter) : m_Phase(phase), m_Counter(counter) {}
    EProcessingPhase GetProcessingPhase() const override { return m_Phase; }
    void Process(FEntitySubsystem&, float) override { ++m_Counter; }
    const char* GetName() const override { return "TestProcessor"; }

private:
    EProcessingPhase m_Phase;
    int& m_Counter;
};

static entt::entity CreateTransformEntity(FEntitySubsystem& ecs, float3 pos, float4 rot, float3 scale)
{
    entt::entity e = ecs.CreateEntity();
    ecs.GetRegistry().emplace<FTransformComponent>(e, pos, rot, scale);
    ecs.GetRegistry().emplace<FWorldTransformComponent>(e);
    return e;
}

// ============================================================
//  Entity Creation / Destruction / Generation
// ============================================================
TEST(ECSTest, CreateEntity_ReturnsValidEntity)
{
    FEntitySubsystem ecs;
    entt::entity e = ecs.CreateEntity();
    EXPECT_TRUE(ecs.GetRegistry().valid(e));
    EXPECT_EQ(ecs.GetEntityCount(), 1);
}

TEST(ECSTest, CreateEntity_AssignsPickID)
{
    FEntitySubsystem ecs;
    entt::entity e = ecs.CreateEntity();
    auto *pickID = ecs.GetRegistry().try_get<FPickIDComponent>(e);
    ASSERT_NE(pickID, nullptr);
    EXPECT_GT(pickID->PickID, 0u);
    EXPECT_LT(pickID->PickID, 10000000u); // MAX_PICK_ID = 9999999
}

TEST(ECSTest, DestroyEntity_InvalidatesEntity)
{
    FEntitySubsystem ecs;
    entt::entity e = ecs.CreateEntity();
    uint32_t pickID = ecs.GetPickID(e);
    ecs.DestroyEntityDeferred(e);
    ecs.Tick(0.0f);
    EXPECT_FALSE(ecs.GetRegistry().valid(e));
    EXPECT_TRUE(ecs.ResolvePickID(pickID) == entt::null);
}

TEST(ECSTest, EntityGeneration_ReuseSlot)
{
    FEntitySubsystem ecs;
    entt::entity e1 = ecs.CreateEntity();
    ecs.DestroyEntityDeferred(e1);
    ecs.Tick(0.0f);
    entt::entity e2 = ecs.CreateEntity();
    // Slot (entity index) is reused, but version is incremented
    EXPECT_EQ(entt::to_entity(e1), entt::to_entity(e2));
    EXPECT_NE(e1, e2);
    EXPECT_TRUE(ecs.GetRegistry().valid(e2));
}

// ============================================================
//  PickID
// ============================================================
TEST(ECSTest, PickID_Monotonic)
{
    FEntitySubsystem ecs;
    entt::entity e1 = ecs.CreateEntity();
    entt::entity e2 = ecs.CreateEntity();
    entt::entity e3 = ecs.CreateEntity();
    EXPECT_LT(ecs.GetPickID(e1), ecs.GetPickID(e2));
    EXPECT_LT(ecs.GetPickID(e2), ecs.GetPickID(e3));
}

TEST(ECSTest, PickID_Resolve_OK)
{
    FEntitySubsystem ecs;
    entt::entity e = ecs.CreateEntity();
    uint32_t pickID = ecs.GetPickID(e);
    EXPECT_EQ(ecs.ResolvePickID(pickID), e);
}

TEST(ECSTest, PickID_Resolve_AfterDestroy_ReturnsNull)
{
    FEntitySubsystem ecs;
    entt::entity e = ecs.CreateEntity();
    uint32_t pickID = ecs.GetPickID(e);
    ecs.DestroyEntityDeferred(e);
    ecs.Tick(0.0f);
    EXPECT_TRUE(ecs.ResolvePickID(pickID) == entt::null);
}

TEST(ECSTest, PickID_Resolve_InvalidID_ReturnsNull)
{
    FEntitySubsystem ecs;
    EXPECT_TRUE(ecs.ResolvePickID(99999999) == entt::null);
}

// ============================================================
//  View
// ============================================================
TEST(ECSTest, View_All_ReturnsMatchedEntities)
{
    FEntitySubsystem ecs;
    entt::entity e1 = ecs.CreateEntity();
    entt::entity e2 = ecs.CreateEntity();
    ecs.GetRegistry().emplace<FNameComponent>(e1);
    int count = 0;
    for (auto e : ecs.GetRegistry().view<FNameComponent>())
        ++count;
    EXPECT_EQ(count, 1);
}

// ============================================================
//  Empty Scene / Deferred Destroy
// ============================================================
TEST(ECSTest, EmptyScene_Tick_NoCrash)
{
    FEntitySubsystem ecs;
    EXPECT_NO_THROW(ecs.Tick(0.0f));
}

TEST(ECSTest, DeferredDestroy_StillAliveBeforeTick)
{
    FEntitySubsystem ecs;
    entt::entity e = ecs.CreateEntity();
    ecs.DestroyEntityDeferred(e);
    EXPECT_TRUE(ecs.GetRegistry().valid(e));
}

TEST(ECSTest, DeferredDestroy_DeadAfterTick)
{
    FEntitySubsystem ecs;
    entt::entity e = ecs.CreateEntity();
    ecs.DestroyEntityDeferred(e);
    ecs.Tick(0.0f);
    EXPECT_FALSE(ecs.GetRegistry().valid(e));
}

// ============================================================
//  Processor Execution Order
// ============================================================
TEST(ECSTest, Processor_RegisteredAndCalled)
{
    FEntitySubsystem ecs;
    int pre = 0, upd = 0;
    ecs.RegisterProcessor(eastl::make_unique<FTestProcessor>(EProcessingPhase::PreUpdate, pre));
    ecs.RegisterProcessor(eastl::make_unique<FTestProcessor>(EProcessingPhase::Update, upd));
    ecs.Tick(1.0f);
    EXPECT_EQ(pre, 1);
    EXPECT_EQ(upd, 1);
}

TEST(ECSTest, Processor_PhaseOrder)
{
    FEntitySubsystem ecs;
    eastl::vector<int> order;
    auto make = [&](EProcessingPhase p, int id) -> eastl::unique_ptr<FProcessorBase>
    {
        struct FP : FProcessorBase
        {
            EProcessingPhase m_Phase;
            int Id;
            eastl::vector<int>& O;

            FP(EProcessingPhase p, int id, eastl::vector<int>& o) : m_Phase(p), Id(id), O(o) {}
            EProcessingPhase GetProcessingPhase() const override { return m_Phase; }
            void Process(FEntitySubsystem&, float) override { O.push_back(Id); }
            const char* GetName() const override { return "P"; }
        };
        return eastl::make_unique<FP>(p, id, order);
    };
    ecs.RegisterProcessor(make(EProcessingPhase::PreUpdate, 1));
    ecs.RegisterProcessor(make(EProcessingPhase::Update, 2));
    ecs.RegisterProcessor(make(EProcessingPhase::PostUpdate, 3));
    ecs.RegisterProcessor(make(EProcessingPhase::Render, 4));
    ecs.Tick(0.0f);
    ASSERT_EQ(order.size(), 4u);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
    EXPECT_EQ(order[2], 3);
    EXPECT_EQ(order[3], 4);
}

// ============================================================
//  Undo/Redo
// ============================================================
TEST(ECSTest, EntityEditableTarget_GetSetTransform_Roundtrip)
{
    FEntitySubsystem ecs;
    entt::entity e = ecs.CreateEntity();
    ecs.GetRegistry().emplace<FTransformComponent>(e);
    Editor::Commands::FEntityEditableTarget target(ecs, e);
    Editor::Commands::FTransformSnapshot newVal;
    newVal.Position = float3(10.0f, 20.0f, 30.0f);
    newVal.Rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    newVal.Scale = float3(2.0f, 2.0f, 2.0f);
    target.SetTransform(newVal);
    auto readBack = target.GetTransform();
    EXPECT_EQ(readBack.Position.x, 10.0f);
}

TEST(ECSTest, FSetTransformCommand_UndoRedo)
{
    FEntitySubsystem ecs;
    entt::entity e = ecs.CreateEntity();
    ecs.GetRegistry().emplace<FTransformComponent>(e);
    auto target = eastl::make_unique<Editor::Commands::FEntityEditableTarget>(ecs, e);
    auto before = target->GetTransform();
    Editor::Commands::FTransformSnapshot after;
    after.Position = float3(5.0f, 5.0f, 5.0f);
    auto cmd = eastl::make_unique<Editor::Commands::FSetTransformCommand>(eastl::move(target), before, after);
    cmd->Redo();
    EXPECT_EQ(ecs.GetRegistry().get<FTransformComponent>(e).Position.x, 5.0f);
    cmd->Undo();
    EXPECT_EQ(ecs.GetRegistry().get<FTransformComponent>(e).Position.x, 0.0f);
}

TEST(ECSTest, FEntitySnapshot_SnapAndRestore)
{
    FEntitySubsystem ecs;
    entt::entity e = ecs.CreateEntity();
    ecs.GetRegistry().emplace<FTransformComponent>(e, float3(1, 2, 3), float4(0, 0, 0, 1), float3(1, 1, 1));
    ecs.GetRegistry().emplace<FNameComponent>(e);
    ecs.GetRegistry().get<FNameComponent>(e).SetName("Test");
    auto snap = ecs.SnapEntity(e);
    EXPECT_TRUE(snap.Transform.has_value());
    ecs.DestroyEntityDeferred(e);
    ecs.Tick(0.0f);
    EXPECT_FALSE(ecs.GetRegistry().valid(e));
    ecs.ApplyEntitySnapshot(snap);
    EXPECT_TRUE(ecs.GetRegistry().valid(e));
    EXPECT_EQ(ecs.GetRegistry().get<FTransformComponent>(e).Position.x, 1.0f);
}

TEST(ECSTest, ResurrectEntity_AfterDestroy)
{
    FEntitySubsystem ecs;
    entt::entity e = ecs.CreateEntity();
    ecs.DestroyEntityDeferred(e);
    ecs.Tick(0.0f);
    EXPECT_FALSE(ecs.GetRegistry().valid(e));
    ecs.ResurrectEntity(e);
    EXPECT_TRUE(ecs.GetRegistry().valid(e));
}

// ============================================================
//  Hierarchy / WorldTransform / Bounding Sphere / Culling
// ============================================================
TEST(ECSTest, Hierarchy_SingleRoot_WorldTransform)
{
    FEntitySubsystem ecs;
    entt::entity e = CreateTransformEntity(ecs, float3(10, 0, 0), float4(0, 0, 0, 1), float3(1, 1, 1));
    ecs.RegisterProcessor(eastl::make_unique<FHierarchyProcessor>());
    ecs.Tick(0.0f);
    auto* w = ecs.GetRegistry().try_get<FWorldTransformComponent>(e);
    ASSERT_NE(w, nullptr);
    EXPECT_EQ(w->MtxWorld[3].x, 10.0f);
}

TEST(ECSTest, Hierarchy_ParentChild_L2W)
{
    FEntitySubsystem ecs;
    entt::entity p = CreateTransformEntity(ecs, float3(10, 0, 0), float4(0, 0, 0, 1), float3(1, 1, 1));
    entt::entity c = CreateTransformEntity(ecs, float3(5, 0, 0), float4(0, 0, 0, 1), float3(1, 1, 1));
    ecs.SetParent(c, p);
    ecs.RegisterProcessor(eastl::make_unique<FHierarchyProcessor>());
    ecs.Tick(0.0f);
    EXPECT_NEAR(ecs.GetRegistry().get<FWorldTransformComponent>(c).MtxWorld[3].x, 15.0f, 0.001f);
}

TEST(ECSTest, Hierarchy_NonUniformScale_Bounds)
{
    FEntitySubsystem ecs;
    entt::entity e = CreateTransformEntity(ecs, float3(0, 0, 0), float4(0, 0, 0, 1), float3(2, 1, 1));
    ecs.GetRegistry().emplace<FBoundingSphereComponent>(e, float3(0, 0, 0), 5.0f);
    ecs.GetRegistry().emplace<FWorldBoundingSphereComponent>(e);
    ecs.RegisterProcessor(eastl::make_unique<FHierarchyProcessor>());
    ecs.Tick(0.0f);
    EXPECT_NEAR(ecs.GetRegistry().get<FWorldBoundingSphereComponent>(e).Radius, 10.0f, 0.001f);
}

TEST(ECSTest, Hierarchy_AncestorScale_PropagatesToWorldBounds)
{
    FEntitySubsystem ecs;
    entt::entity root = CreateTransformEntity(ecs, float3(0, 0, 0), float4(0, 0, 0, 1), float3(100, 100, 100));
    entt::entity mesh = CreateTransformEntity(ecs, float3(0, 0, 0), float4(0, 0, 0, 1), float3(1, 1, 1));
    ecs.SetParent(mesh, root);
    ecs.GetRegistry().emplace<FBoundingSphereComponent>(mesh, float3(0, 0, 0), 2.0f);
    ecs.GetRegistry().emplace<FWorldBoundingSphereComponent>(mesh);

    ecs.RegisterProcessor(eastl::make_unique<FHierarchyProcessor>());
    ecs.Tick(0.0f);

    // Before the fix: local scale (1,1,1) => radius 2.0 (100x too small).
    // After: world column length == 100 => radius 200.
    EXPECT_NEAR(ecs.GetRegistry().get<FWorldBoundingSphereComponent>(mesh).Radius, 200.0f, 0.01f);
}

TEST(ECSTest, Hierarchy_RotatedScale_BoundsRadiusIgnoresRotation)
{
    FEntitySubsystem ecs;
    entt::entity e = CreateTransformEntity(ecs, float3(0, 0, 0), RotationQuat(float3(90, 90, 0)), float3(3, 3, 3));
    ecs.GetRegistry().emplace<FBoundingSphereComponent>(e, float3(0, 0, 0), 1.0f);
    ecs.GetRegistry().emplace<FWorldBoundingSphereComponent>(e);

    ecs.RegisterProcessor(eastl::make_unique<FHierarchyProcessor>());
    ecs.Tick(0.0f);

    EXPECT_NEAR(ecs.GetRegistry().get<FWorldBoundingSphereComponent>(e).Radius, 3.0f, 0.001f);
}

TEST(ECSTest, Hierarchy_InvalidParent_Removed)
{
    FEntitySubsystem ecs;
    entt::entity c = CreateTransformEntity(ecs, float3(0, 0, 0), float4(0, 0, 0, 1), float3(1, 1, 1));
    ecs.GetRegistry().emplace<FParentComponent>(c, entt::entity{9999});
    ecs.RegisterProcessor(eastl::make_unique<FHierarchyProcessor>());
    ecs.Tick(0.0f);
    EXPECT_EQ(ecs.GetRegistry().try_get<FParentComponent>(c), nullptr);
}

TEST(ECSTest, Hierarchy_SelfReference_Rejected)
{
    FEntitySubsystem ecs;
    entt::entity e = CreateTransformEntity(ecs, float3(0, 0, 0), float4(0, 0, 0, 1), float3(1, 1, 1));
    ecs.SetParent(e, e);
    EXPECT_EQ(ecs.GetRegistry().try_get<FParentComponent>(e), nullptr);
}

TEST(ECSTest, Hierarchy_ParentDeleted_OrphanBecomesRoot)
{
    FEntitySubsystem ecs;
    entt::entity p = CreateTransformEntity(ecs, float3(10, 0, 0), float4(0, 0, 0, 1), float3(1, 1, 1));
    entt::entity c = CreateTransformEntity(ecs, float3(5, 0, 0), float4(0, 0, 0, 1), float3(1, 1, 1));
    ecs.SetParent(c, p);
    ecs.RegisterProcessor(eastl::make_unique<FHierarchyProcessor>());
    ecs.DestroyEntityDeferred(p);
    ecs.Tick(0.0f);
    EXPECT_EQ(ecs.GetRegistry().try_get<FParentComponent>(c), nullptr);
}

TEST(ECSTest, Hierarchy_IsolatedCycle_BrokenAndProcessed)
{
    FEntitySubsystem ecs;
    entt::entity a = CreateTransformEntity(ecs, float3(1, 0, 0), float4(0, 0, 0, 1), float3(1, 1, 1));
    entt::entity b = CreateTransformEntity(ecs, float3(2, 0, 0), float4(0, 0, 0, 1), float3(1, 1, 1));
    ecs.SetParent(b, a);
    ecs.GetRegistry().emplace<FParentComponent>(a, b);

    ecs.RegisterProcessor(eastl::make_unique<FHierarchyProcessor>());
    ecs.Tick(0.0f);

    // The cycle is broken: at least one entity no longer has a parent.
    const bool aHasParent = ecs.GetRegistry().try_get<FParentComponent>(a) != nullptr;
    const bool bHasParent = ecs.GetRegistry().try_get<FParentComponent>(b) != nullptr;
    EXPECT_FALSE(aHasParent && bHasParent);

    EXPECT_NEAR(ecs.GetRegistry().get<FWorldTransformComponent>(a).MtxWorld[3].x, 1.0f, 0.001f);
    EXPECT_NEAR(ecs.GetRegistry().get<FWorldTransformComponent>(b).MtxWorld[3].x, 3.0f, 0.001f);
}

TEST(ECSTest, Hierarchy_CycleBreak_BrokenEntityHasIdentityParent)
{
    FEntitySubsystem ecs;
    entt::entity a = CreateTransformEntity(ecs, float3(1, 0, 0), float4(0, 0, 0, 1), float3(1, 1, 1));
    entt::entity b = CreateTransformEntity(ecs, float3(2, 0, 0), float4(0, 0, 0, 1), float3(1, 1, 1));
    ecs.SetParent(b, a);
    ecs.GetRegistry().emplace<FParentComponent>(a, b);
    ecs.RegisterProcessor(eastl::make_unique<FHierarchyProcessor>());

    ecs.Tick(0.0f);        // the breaking frame

    ASSERT_NE(ecs.GetRegistry().try_get<FWorldTransformComponent>(a), nullptr);
    ASSERT_NE(ecs.GetRegistry().try_get<FWorldTransformComponent>(b), nullptr);
    const float aTx = ecs.GetRegistry().get<FWorldTransformComponent>(a).MtxWorld[3].x;
    const float bTx = ecs.GetRegistry().get<FWorldTransformComponent>(b).MtxWorld[3].x;
    EXPECT_NEAR(aTx, 1.0f, 0.001f);    // identity*local
    EXPECT_NEAR(bTx, 3.0f, 0.001f);    // T(b)*T(a)
}

TEST(ECSTest, Hierarchy_RepeatedTick_IsDeterministic)
{
    FEntitySubsystem ecs;
    entt::entity p = CreateTransformEntity(ecs, float3(10, 0, 0), float4(0, 0, 0, 1), float3(1, 1, 1));
    entt::entity c = CreateTransformEntity(ecs, float3(5, 0, 0), float4(0, 0, 0, 1), float3(1, 1, 1));
    ecs.SetParent(c, p);
    ecs.RegisterProcessor(eastl::make_unique<FHierarchyProcessor>());

    ecs.Tick(0.0f);
    const float first = ecs.GetRegistry().get<FWorldTransformComponent>(c).MtxWorld[3].x;
    for (int i = 0; i < 10; ++i) ecs.Tick(0.0f);
    const float last = ecs.GetRegistry().get<FWorldTransformComponent>(c).MtxWorld[3].x;

    EXPECT_NEAR(first, 15.0f, 0.001f);
    EXPECT_NEAR(first, last, 0.0001f);
}

TEST(ECSTest, Culling_InsideFrustum_Visible)
{
    FEntitySubsystem ecs;
    entt::entity e = ecs.CreateEntity();
    ecs.GetRegistry().emplace<FWorldBoundingSphereComponent>(e, float3(0, 0, 0), 1.0f);
    ecs.GetRegistry().emplace<FVisibilityComponent>(e);
    float4 fp[6] = {{0, 0, 1, 0.99f}, {0, 0, -1, 999}, {1, 0, 0, 1}, {-1, 0, 0, 1}, {0, 1, 0, 1}, {0, -1, 0, 1}};
    ecs.RegisterProcessor(eastl::make_unique<FCullingProcessor>(eastl::span<const float4>{fp, 6}, Tests::ParallelForFunc));
    ecs.Tick(0.0f);
    EXPECT_TRUE(ecs.GetRegistry().get<FVisibilityComponent>(e).bFrustumVisible);
}

TEST(ECSTest, Culling_OutsideFrustum_NotVisible)
{
    FEntitySubsystem ecs;
    entt::entity e = ecs.CreateEntity();
    ecs.GetRegistry().emplace<FWorldBoundingSphereComponent>(e, float3(1000, 1000, 1000), 1.0f);
    ecs.GetRegistry().emplace<FVisibilityComponent>(e);
    float4 fp[6] = {{0, 0, 1, 0.99f}, {0, 0, -1, 999}, {1, 0, 0, 1}, {-1, 0, 0, 1}, {0, 1, 0, 1}, {0, -1, 0, 1}};
    ecs.RegisterProcessor(eastl::make_unique<FCullingProcessor>(eastl::span<const float4>{fp, 6}, Tests::ParallelForFunc));
    ecs.Tick(0.0f);
    EXPECT_FALSE(ecs.GetRegistry().get<FVisibilityComponent>(e).bFrustumVisible);
}

TEST(ECSTest, SetParent_GetChildren_Consistency)
{
    FEntitySubsystem ecs;
    entt::entity p = ecs.CreateEntity(), c1 = ecs.CreateEntity(), c2 = ecs.CreateEntity();
    ecs.SetParent(c1, p);
    ecs.SetParent(c2, p);
    auto children = ecs.GetChildren(p);
    EXPECT_EQ(children.size(), 2u);
}

TEST(ECSTest, Hierarchy_MultipleLevels_L2W)
{
    FEntitySubsystem ecs;
    entt::entity gp = CreateTransformEntity(ecs, float3(10, 0, 0), float4(0, 0, 0, 1), float3(1, 1, 1));
    entt::entity p = CreateTransformEntity(ecs, float3(5, 0, 0), float4(0, 0, 0, 1), float3(1, 1, 1));
    entt::entity c = CreateTransformEntity(ecs, float3(3, 0, 0), float4(0, 0, 0, 1), float3(1, 1, 1));
    ecs.SetParent(p, gp);
    ecs.SetParent(c, p);
    ecs.RegisterProcessor(eastl::make_unique<FHierarchyProcessor>());
    ecs.Tick(0.0f);
    EXPECT_NEAR(ecs.GetRegistry().get<FWorldTransformComponent>(c).MtxWorld[3].x, 18.0f, 0.001f);
}

// ============================================================
//  Render Submission
// ============================================================

class FFakeRenderSink : public Scene::ECS::ISceneRenderSink
{
public:
    eastl::vector<uint32_t> AddedInstances;
    eastl::vector<FInstanceData> AddedInstanceData;
    eastl::vector<Scene::ECS::FRenderBatchInput> BaseBatches;
    uint32_t NextPickID = 0;
    bool bMouseHitTest = false;

    uint32_t AddInstance(const FInstanceData& instanceData) override
    {
        AddedInstances.push_back(instanceData.ObjectID);
        AddedInstanceData.push_back(instanceData);
        return static_cast<uint32_t>(AddedInstances.size() - 1);
    }

    void AddBasePassBatch(const Scene::ECS::FRenderBatchInput& input) override { BaseBatches.push_back(input); }
    void AddObjectIDBatch(const Scene::ECS::FRenderBatchInput&) override {}
    void AddOutlineBatch(const Scene::ECS::FRenderBatchInput&) override {}
    uint32_t AllocateMaterialConstants(const void*, uint32_t) override { return 0x1000u; }
    uint32_t GetSelectedPickID() const override { return NextPickID; }
    bool IsEnableMouseHitTest() const override { return bMouseHitTest; }
    void AddAnimationBatch(const Scene::ECS::FAnimationBatchInput& input) override
    {
        ++AnimationBatchCount;
        LastSkinInput = input;
    }

public:
    int AnimationBatchCount = 0;
    Scene::ECS::FAnimationBatchInput LastSkinInput;
};

TEST(ECSTest, AssetRegistry_ImportModel)
{
    FFakeAssetRegistry reg;
    auto h = reg.ImportModel("test.gltf");
    EXPECT_TRUE(h.IsValid());
    auto h2 = reg.ImportModel("test.gltf");
    EXPECT_EQ(h2.Index, h.Index);
}

TEST(ECSTest, AssetRegistry_AcquireMesh)
{
    FFakeAssetRegistry reg;
    auto model = reg.ImportModel("test.gltf");
    auto mesh = reg.AcquireMesh(model, 0, 0);
    EXPECT_TRUE(mesh.IsValid());
    EXPECT_EQ(reg.GetMeshAcquireCount(0), 1u);
}

TEST(ECSTest, AssetRegistry_InvalidHandle)
{
    Scene::ECS::FMeshResourceHandle invalid;
    EXPECT_FALSE(invalid.IsValid());
}

TEST(ECSTest, AssetRegistry_GetModelInfo)
{
    FFakeAssetRegistry reg;
    auto h = reg.ImportModel("test.gltf");
    auto info = reg.GetModelInfo(h);
    EXPECT_EQ(info.ModelPath, "test.gltf");
}

TEST(ECSTest, RenderSubmission_NoMeshRenderer_NoInstance)
{
    // Entities without FMeshRendererComponent never enter the GPU scene (view mismatch).
    Scene::ECS::FEntitySubsystem ecs;
    FFakeAssetRegistry fakeReg;
    FFakeRenderSink fakeSink;

    entt::entity e = ecs.CreateEntity();
    ecs.GetRegistry().emplace<Scene::ECS::FWorldTransformComponent>(e);
    ecs.GetRegistry().emplace<Scene::ECS::FWorldBoundingSphereComponent>(e);

    auto processor = eastl::make_unique<Scene::ECS::FRenderSubmissionProcessor>(fakeSink, fakeReg);
    processor->Process(ecs, 0.0f);   // the original test forgot this call, making the assert trivially true

    EXPECT_EQ(fakeSink.AddedInstances.size(), 0u);
    EXPECT_EQ(fakeSink.BaseBatches.size(), 0u);
}

TEST(ECSTest, RenderSubmission_VisibleOnlyAddInstance)
{
    Scene::ECS::FEntitySubsystem ecs;
    FFakeAssetRegistry fakeReg;
    FFakeRenderSink fakeSink;

    auto model = fakeReg.ImportModel("test.gltf");

    auto makeMesh = [&](bool bVisible)
    {
        auto meshHandle = fakeReg.AcquireMesh(model, 0, 0);
        entt::entity e = ecs.CreateEntity();
        ecs.GetRegistry().emplace<Scene::ECS::FWorldTransformComponent>(e);
        ecs.GetRegistry().emplace<Scene::ECS::FWorldBoundingSphereComponent>(e);
        ecs.GetRegistry().emplace<Scene::ECS::FVisibilityComponent>(e, bVisible);
        ecs.GetRegistry().emplace<Scene::ECS::FMeshRendererComponent>(e, meshHandle, Scene::ECS::FMaterialResourceHandle{}, false);
        return e;
    };
    makeMesh(true);
    makeMesh(false);
    makeMesh(true);

    auto processor = eastl::make_unique<Scene::ECS::FRenderSubmissionProcessor>(fakeSink, fakeReg);
    processor->Process(ecs, 0.0f);

    EXPECT_EQ(fakeSink.AddedInstances.size(), 2u);   // visible entities only
    ASSERT_EQ(fakeSink.BaseBatches.size(), 2u);      // visible ones only
    for (size_t i = 0; i < fakeSink.BaseBatches.size(); ++i)
        EXPECT_EQ(fakeSink.BaseBatches[i].InstanceIndex, static_cast<uint32_t>(i));
}

TEST(ECSTest, RenderSubmission_MissingRequiredComponents_Excluded)
{
    Scene::ECS::FEntitySubsystem ecs;
    FFakeAssetRegistry fakeReg;
    FFakeRenderSink fakeSink;

    auto model = fakeReg.ImportModel("test.gltf");
    auto meshHandle = fakeReg.AcquireMesh(model, 0, 0);

    // F8: the submission view now requires PickID + Visibility. Malformed
    // entities are excluded by view semantics instead of tripping registry.get.
    entt::entity eNoPick = ecs.CreateEntity();
    ecs.GetRegistry().emplace<Scene::ECS::FWorldTransformComponent>(eNoPick);
    ecs.GetRegistry().emplace<Scene::ECS::FWorldBoundingSphereComponent>(eNoPick);
    ecs.GetRegistry().emplace<Scene::ECS::FVisibilityComponent>(eNoPick, true);
    ecs.GetRegistry().remove<Scene::ECS::FPickIDComponent>(eNoPick);
    ecs.GetRegistry().emplace<Scene::ECS::FMeshRendererComponent>(eNoPick, meshHandle, Scene::ECS::FMaterialResourceHandle{}, false);

    entt::entity eNoVisibility = ecs.CreateEntity();
    ecs.GetRegistry().emplace<Scene::ECS::FWorldTransformComponent>(eNoVisibility);
    ecs.GetRegistry().emplace<Scene::ECS::FWorldBoundingSphereComponent>(eNoVisibility);
    ecs.GetRegistry().emplace<Scene::ECS::FMeshRendererComponent>(eNoVisibility, meshHandle, Scene::ECS::FMaterialResourceHandle{}, false);

    auto processor = eastl::make_unique<Scene::ECS::FRenderSubmissionProcessor>(fakeSink, fakeReg);
    processor->Process(ecs, 0.0f);

    EXPECT_EQ(fakeSink.AddedInstances.size(), 0u);
    EXPECT_EQ(fakeSink.BaseBatches.size(), 0u);
}

TEST(ECSTest, RenderSubmission_VisibleEntityGetsBaseBatch)
{
    Scene::ECS::FEntitySubsystem ecs;
    FFakeAssetRegistry fakeReg;
    FFakeRenderSink fakeSink;

    auto model = fakeReg.ImportModel("test.gltf");
    auto meshHandle = fakeReg.AcquireMesh(model, 0, 0);

    entt::entity e = ecs.CreateEntity();
    ecs.GetRegistry().emplace<Scene::ECS::FWorldTransformComponent>(e);
    ecs.GetRegistry().emplace<Scene::ECS::FWorldBoundingSphereComponent>(e);
    ecs.GetRegistry().emplace<Scene::ECS::FVisibilityComponent>(e, true);
    ecs.GetRegistry().emplace<Scene::ECS::FMeshRendererComponent>(e, meshHandle, Scene::ECS::FMaterialResourceHandle{}, false);

    auto processor = eastl::make_unique<Scene::ECS::FRenderSubmissionProcessor>(fakeSink, fakeReg);
    processor->Process(ecs, 0.0f);

    EXPECT_EQ(fakeSink.AddedInstances.size(), 1u);
    EXPECT_EQ(fakeSink.BaseBatches.size(), 1u);
}

TEST(ECSTest, EntityFactory_CreateEmpty)
{
    Scene::ECS::FEntitySubsystem ecs;
    FFakeAssetRegistry fakeReg;
    Scene::ECS::FEntityFactory factory(ecs, fakeReg);

    entt::entity e = factory.CreateEmpty("TestEntity");
    EXPECT_TRUE(ecs.GetRegistry().valid(e));
    auto* name = ecs.GetRegistry().try_get<Scene::ECS::FNameComponent>(e);
    ASSERT_NE(name, nullptr);
    EXPECT_EQ(strcmp(name->Name, "TestEntity"), 0);
}

TEST(ECSTest, EntityFactory_CreateModelInstance)
{
    Scene::ECS::FEntitySubsystem ecs;
    FFakeAssetRegistry fakeReg;
    Scene::ECS::FEntityFactory factory(ecs, fakeReg);

    auto model = fakeReg.ImportModel("test.gltf");
    entt::entity root = factory.CreateModelInstance(model, "test.gltf");
    EXPECT_TRUE(ecs.GetRegistry().valid(root));
    EXPECT_TRUE(ecs.GetRegistry().all_of<Scene::ECS::FModelInstanceComponent>(root));
    EXPECT_TRUE(ecs.GetRegistry().all_of<Scene::ECS::FTransformComponent>(root));
}

TEST(ECSTest, EntityFactory_AnimationDefaultsToPaused)
{
    Scene::ECS::FEntitySubsystem ecs;
    FFakeAssetRegistry fakeReg;
    fakeReg.AnimClips.emplace_back();

    auto model = fakeReg.ImportModel("anim.gltf");
    const Scene::ECS::FModelAssetInfo info = fakeReg.GetModelInfo(model);
    Scene::ECS::FEntityFactory factory(ecs, fakeReg);

    entt::entity root = factory.CreateModelInstance(model, "anim.gltf");
    factory.CreateModelHierarchy(model, info, root, float3(0.0f), quaternion(0.0f, 0.0f, 0.0f, 1.0f), float3(1.0f));

    const auto* anim = ecs.GetRegistry().try_get<Scene::ECS::FAnimationComponent>(root);
    ASSERT_NE(anim, nullptr);
    EXPECT_FALSE(anim->bPlaying);
}

TEST(ECSTest, RenderSubmission_RotatedTransform_ScaleCorrect)
{
    Scene::ECS::FEntitySubsystem ecs;
    FFakeAssetRegistry fakeReg;
    FFakeRenderSink fakeSink;

    auto model = fakeReg.ImportModel("test.gltf");
    auto meshHandle = fakeReg.AcquireMesh(model, 0, 0);

    const float4x4 T = translation_matrix(float3(0.0f));
    const float4x4 R = rotation_matrix(RotationQuat(float3(90.0f, 90.0f, 0.0f)));
    const float4x4 S = scaling_matrix(float3(2.0f, 2.0f, 2.0f));
    const float4x4 world = mul(T, mul(R, S));

    entt::entity e = ecs.CreateEntity();
    auto& wt = ecs.GetRegistry().emplace<Scene::ECS::FWorldTransformComponent>(e);
    wt.MtxWorld = world;
    wt.MtxWorldInvTranspose = transpose(inverse(world));
    ecs.GetRegistry().emplace<Scene::ECS::FWorldBoundingSphereComponent>(e, float3(0, 0, 0), 1.0f);
    ecs.GetRegistry().emplace<Scene::ECS::FVisibilityComponent>(e, true);
    ecs.GetRegistry().emplace<Scene::ECS::FMeshRendererComponent>(e, meshHandle, Scene::ECS::FMaterialResourceHandle{}, false);

    auto processor = eastl::make_unique<Scene::ECS::FRenderSubmissionProcessor>(fakeSink, fakeReg);
    processor->Process(ecs, 0.0f);

    ASSERT_EQ(fakeSink.AddedInstanceData.size(), 1u);
    EXPECT_NEAR(fakeSink.AddedInstanceData[0].Scale, 2.0f, 0.001f);   // ~0 before the fix
}

TEST(ECSTest, EntityFactory_InvalidMaterialHandle_NoMeshEntity)
{
    Scene::ECS::FEntitySubsystem ecs;
    FFakeAssetRegistry fakeReg;
    fakeReg.bFailMaterialAcquire = true;
    Scene::ECS::FEntityFactory factory(ecs, fakeReg);

    auto model = fakeReg.ImportModel("test.gltf");
    entt::entity root = factory.CreateModelInstance(model, "test.gltf");

    Scene::ECS::FModelAssetInfo info;
    Scene::ECS::FModelNodeInfo node;
    node.Name = "N0";
    node.NodeIndex = 0;
    node.Primitives.push_back(0);
    info.Nodes.push_back(node);

    factory.CreateModelHierarchy(model, info, root, float3(0.0f), quaternion(0, 0, 0, 1), float3(1.0f));

    // Node entities may exist, but there must be no FMeshRendererComponent at all.
    int meshCount = 0;
    for ([[maybe_unused]] auto e : ecs.GetRegistry().view<Scene::ECS::FMeshRendererComponent>())
        ++meshCount;
    EXPECT_EQ(meshCount, 0);
}

TEST(ECSTest, EntityFactory_InvalidMeshHandle_ReleasesMaterial)
{
    Scene::ECS::FEntitySubsystem ecs;
    FFakeAssetRegistry fakeReg;
    fakeReg.bFailMeshAcquire = true;
    Scene::ECS::FEntityFactory factory(ecs, fakeReg);

    auto model = fakeReg.ImportModel("test.gltf");
    entt::entity root = factory.CreateModelInstance(model, "test.gltf");

    Scene::ECS::FModelAssetInfo info;
    Scene::ECS::FModelNodeInfo node;
    node.Name = "N0";
    node.NodeIndex = 0;
    node.Primitives.push_back(0);
    info.Nodes.push_back(node);

    factory.CreateModelHierarchy(model, info, root, float3(0.0f), quaternion(0, 0, 0, 1), float3(1.0f));

    int meshCount = 0;
    for ([[maybe_unused]] auto e : ecs.GetRegistry().view<Scene::ECS::FMeshRendererComponent>())
        ++meshCount;
    EXPECT_EQ(meshCount, 0);
    EXPECT_EQ(fakeReg.MaterialReleaseCount, 1);   // acquired once, rolled back once
    EXPECT_EQ(fakeReg.MeshReleaseCount, 0);       // mesh never acquired, nothing to release
}

TEST(ECSTest, AssetRegistry_ReacquireAfterRelease_KeepsHandleAlive)
{
    FFakeAssetRegistry reg;
    auto model = reg.ImportModel("test.gltf");
    auto mesh = reg.AcquireMesh(model, 0, 0);
    ASSERT_NE(reg.FindMeshData(mesh), nullptr);

    reg.Release(mesh);                                // RefCount 0 -> queued for delayed release
    auto revived = reg.AcquireMesh(model, 0, 0);      // re-acquired within the window
    reg.FrameFencePassed(1000);                               // window matures

    EXPECT_NE(reg.FindMeshData(revived), nullptr) << "handle was silently invalidated after re-acquire";
}

TEST(ECSTest, AssetRegistry_ReleaseWithoutReacquire_InvalidatesHandle)
{
    FFakeAssetRegistry reg;
    auto model = reg.ImportModel("test.gltf");
    auto mesh = reg.AcquireMesh(model, 0, 0);

    reg.Release(mesh);
    reg.FrameFencePassed(1000);

    EXPECT_EQ(reg.FindMeshData(mesh), nullptr);
}

TEST(ECSTest, FindModelInstanceRootWalksParentChain)
{
    using namespace Assets;
    FEntitySubsystem ecs;
    auto& reg = ecs.GetRegistry();

    entt::entity root = ecs.CreateEntity();
    reg.emplace<FModelInstanceComponent>(root, FModelAssetHandle{0u, 1u}, eastl::string("m.gltf"));
    entt::entity child = ecs.CreateEntity();
    ecs.SetParent(child, root);
    entt::entity mesh = ecs.CreateEntity();
    reg.emplace<FMeshRendererComponent>(mesh);
    ecs.SetParent(mesh, child);

    EXPECT_EQ(FModelLoaderECS::FindModelInstanceRoot(reg, mesh), root);
    EXPECT_EQ(FModelLoaderECS::FindModelInstanceRoot(reg, child), root);
    EXPECT_EQ(FModelLoaderECS::FindModelInstanceRoot(reg, root), root);
    EXPECT_TRUE(FModelLoaderECS::FindModelInstanceRoot(reg, ecs.CreateEntity()) == entt::null);
}

// ============================================================
//  Fix F3: cycle rejection at the source + snapshot Parent/Mesh completeness
// ============================================================
TEST(ECSTest, SetParent_DescendantRejected)
{
    FEntitySubsystem ecs;
    entt::entity parent = ecs.CreateEntity();
    entt::entity child = ecs.CreateEntity();
    ecs.SetParent(child, parent);

    // parent -> child would form parent -> child -> parent cycle; must be rejected.
    ecs.SetParent(parent, child);

    EXPECT_EQ(ecs.GetRegistry().try_get<FParentComponent>(parent), nullptr);
    const auto* childComp = ecs.GetRegistry().try_get<FParentComponent>(child);
    ASSERT_NE(childComp, nullptr);
    EXPECT_EQ(childComp->Parent, parent);
}

TEST(ECSTest, Snapshot_Restore_ParentAndMesh_Completeness)
{
    FEntitySubsystem ecs;
    entt::entity parent = ecs.CreateEntity();
    entt::entity mesh = ecs.CreateEntity();
    ecs.SetParent(mesh, parent);

    FMeshResourceHandle meshHandle{7, 2};
    FMaterialResourceHandle matHandle{3, 1};
    ecs.GetRegistry().emplace<FMeshRendererComponent>(mesh, meshHandle, matHandle, false);

    auto snap = ecs.SnapEntity(mesh);
    ASSERT_TRUE(snap.Parent.has_value());
    ASSERT_TRUE(snap.Mesh.has_value());
    EXPECT_EQ(snap.Mesh.value().MeshHandle, meshHandle);
    EXPECT_EQ(snap.Mesh.value().MaterialHandle, matHandle);

    ecs.DestroyEntityDeferred(mesh);
    ecs.FlushDeferred();
    EXPECT_FALSE(ecs.GetRegistry().valid(mesh));

    ecs.ApplyEntitySnapshot(snap);

    EXPECT_TRUE(ecs.GetRegistry().valid(mesh));
    const auto* restoredMesh = ecs.GetRegistry().try_get<FMeshRendererComponent>(mesh);
    ASSERT_NE(restoredMesh, nullptr);
    EXPECT_EQ(restoredMesh->MeshHandle, meshHandle);
    EXPECT_EQ(restoredMesh->MaterialHandle, matHandle);

    const auto* restoredParent = ecs.GetRegistry().try_get<FParentComponent>(mesh);
    ASSERT_NE(restoredParent, nullptr);
    EXPECT_EQ(restoredParent->Parent, parent);

    // Hierarchy restored through SetParent maintains the parent's child list.
    const auto* children = ecs.FindChildren(parent);
    ASSERT_NE(children, nullptr);
    EXPECT_TRUE(eastl::find(children->begin(), children->end(), mesh) != children->end());
}

TEST(ECSTest, Snapshot_Restore_CyclicParents_NoCycle)
{
    FEntitySubsystem ecs;
    entt::entity a = ecs.CreateEntity();
    entt::entity b = ecs.CreateEntity();

    // A <-> B built through raw registry edits, simulating a legacy/deserialization
    // path where the SetParent guard was bypassed.
    ecs.GetRegistry().emplace<FParentComponent>(a, b);
    ecs.GetRegistry().emplace<FParentComponent>(b, a);

    auto snapA = ecs.SnapEntity(a);
    auto snapB = ecs.SnapEntity(b);

    ecs.DestroyEntityDeferred(a);
    ecs.DestroyEntityDeferred(b);
    ecs.FlushDeferred();

    // Restore in arbitrary order; the SetParent ancestor guard must break one link
    // (and the invalid-parent guard must not crash on the freshly resurrected entity).
    ecs.ApplyEntitySnapshot(snapA);
    ecs.ApplyEntitySnapshot(snapB);

    const auto* aParent = ecs.GetRegistry().try_get<FParentComponent>(a);
    const auto* bParent = ecs.GetRegistry().try_get<FParentComponent>(b);
    const bool bCycle = (aParent != nullptr && aParent->Parent == b) && (bParent != nullptr && bParent->Parent == a);
    EXPECT_FALSE(bCycle);
    // The restored snapshot link survives in at least one direction.
    EXPECT_TRUE((aParent != nullptr && aParent->Parent == b) || (bParent != nullptr && bParent->Parent == a));
}

TEST(ECSTest, Snapshot_Restore_DetachParent_RestoresRoot)
{
    FEntitySubsystem ecs;
    entt::entity a = ecs.CreateEntity();
    entt::entity b = ecs.CreateEntity();

    // Snapshot while isolated (no parent, no mesh).
    auto snap = ecs.SnapEntity(a);
    EXPECT_FALSE(snap.Parent.has_value());
    EXPECT_FALSE(snap.Mesh.has_value());

    // User attaches a to b, then the edit is undone: applying the old snapshot
    // must detach a again, keeping m_ChildrenMap coherent (symmetric branch of
    // ApplyEntitySnapshot).
    ecs.SetParent(a, b);
    EXPECT_NE(ecs.GetRegistry().try_get<FParentComponent>(a), nullptr);

    ecs.ApplyEntitySnapshot(snap);

    EXPECT_EQ(ecs.GetRegistry().try_get<FParentComponent>(a), nullptr);
    const auto* bChildren = ecs.FindChildren(b);
    if (bChildren != nullptr)
    {
        EXPECT_TRUE(eastl::find(bChildren->begin(), bChildren->end(), a) == bChildren->end());
    }
}

TEST(ECSTest, Hierarchy_AABB_WorldTransform)
{
    FEntitySubsystem ecs;
    entt::entity e = ecs.CreateEntity();
    // Local [0,1]^3, transform: translate(10,0,0) * scale(2,2,2).
    ecs.GetRegistry().emplace<FTransformComponent>(e, float3(10, 0, 0), float4(0, 0, 0, 1), float3(2, 2, 2));
    ecs.GetRegistry().emplace<FWorldTransformComponent>(e);
    ecs.GetRegistry().emplace<FBoundingBoxComponent>(e, float3(0, 0, 0), float3(1, 1, 1));
    ecs.RegisterProcessor(eastl::make_unique<FHierarchyProcessor>());
    ecs.Tick(0.0f);

    const auto* worldAABB = ecs.GetRegistry().try_get<FWorldBoundingBoxComponent>(e);
    ASSERT_NE(worldAABB, nullptr);
    EXPECT_NEAR(worldAABB->Min.x, 10.0f, 0.001f);
    EXPECT_NEAR(worldAABB->Max.x, 12.0f, 0.001f);
    EXPECT_NEAR(worldAABB->Min.y, 0.0f, 0.001f); // local min.y = 0, scaled 2×
    EXPECT_NEAR(worldAABB->Max.y, 2.0f, 0.001f);
}

TEST(ECSTest, Hierarchy_AABB_Rotation_EnclosesOBB)
{
    FEntitySubsystem ecs;
    entt::entity e = ecs.CreateEntity();
    // Unit cube centered at origin, rotated 45° about Z (quaternion storage is
    // (x,y,z,w): k*sin(22.5°), cos(22.5°)). OBB corners push the enclosing AABB
    // out to ±cos45°=±0.7071 in x/y.
    const float c = 0.70710678f;
    // NOTE: float3(0) is ambiguous in linalg (scalar-broadcast vs pointer ctor, C2668) —
    // the trailing .0f is REQUIRED here and matches repo style (Components.hpp:32-34).
    ecs.GetRegistry().emplace<FTransformComponent>(e, float3(0.0f), float4(0, 0, 0.3826834f, 0.9238795f), float3(1.0f));
    ecs.GetRegistry().emplace<FWorldTransformComponent>(e);
    ecs.GetRegistry().emplace<FBoundingBoxComponent>(e, float3(-0.5f), float3(0.5f));
    ecs.RegisterProcessor(eastl::make_unique<FHierarchyProcessor>());
    ecs.Tick(0.0f);

    const auto* worldAABB = ecs.GetRegistry().try_get<FWorldBoundingBoxComponent>(e);
    ASSERT_NE(worldAABB, nullptr);
    EXPECT_NEAR(worldAABB->Max.x, c, 0.001f);
    EXPECT_NEAR(worldAABB->Max.y, c, 0.001f);
    EXPECT_NEAR(worldAABB->Min.x, -c, 0.001f);
    EXPECT_NEAR(worldAABB->Min.y, -c, 0.001f);
    EXPECT_NEAR(worldAABB->Max.z, 0.5f, 0.001f); // z axis untouched by the rotation
}

// ============================================================
//  Slice 3: FLightProcessor snapshots
// ============================================================
TEST(ECSTest, Light_TaggedEntity_ProducesSnapshot)
{
    FEntitySubsystem ecs;
    Scene::ECS::FSceneLightSnapshot out;
    ecs.RegisterProcessor(eastl::make_unique<FLightProcessor>(&out));

    entt::entity light = ecs.CreateEntity();
    ecs.GetRegistry().emplace<FLightComponent>(light, float3(0.2f, 0.4f, 0.6f), float3(0.0f, -1.0f, 0.0f), 7.0f);
    ecs.GetRegistry().emplace<FMainLightTag>(light);

    ecs.Tick(0.0f);

    EXPECT_TRUE(out.bValid);
    EXPECT_NEAR(out.Color.x, 0.2f, 0.001f);
    EXPECT_NEAR(out.Color.y, 0.4f, 0.001f);
    EXPECT_NEAR(out.Direction.y, -1.0f, 0.001f);
    EXPECT_NEAR(out.Radius, 7.0f, 0.001f);
}

TEST(ECSTest, Light_NoTaggedEntity_Invalid)
{
    FEntitySubsystem ecs;
    Scene::ECS::FSceneLightSnapshot out;
    out.bValid = true;
    ecs.RegisterProcessor(eastl::make_unique<FLightProcessor>(&out));

    entt::entity plain = ecs.CreateEntity();
    ecs.GetRegistry().emplace<FLightComponent>(plain, float3(0.9f, 0.9f, 0.9f), float3(0.0f, 1.0f, 0.0f), 1.0f);

    ecs.Tick(0.0f);

    EXPECT_FALSE(out.bValid);
}

// ============================================================
//  Slice 3.5
// ============================================================
TEST(ECSTest, Animation_ChannelWritesNodeTransform)
{
    FEntitySubsystem ecs;
    FFakeAssetRegistry fake;
    fake.AnimClips.emplace_back();
    fake.AnimClips[0].Channels.push_back({0, Scene::ECS::EAnimChannelMode::Translation, {{0.0f, float4(0, 0, 0, 1)}, {1.0f, float4(10, 0, 0, 1)}}});
    fake.AnimClips[0].TimeDuration = 1.0f;
    ecs.RegisterProcessor(eastl::make_unique<FAnimationProcessor>(&fake));

    entt::entity root = ecs.CreateEntity();
    ecs.GetRegistry().emplace<FModelInstanceComponent>(root, FModelAssetHandle{0, 0}, "test");
    ecs.GetRegistry().emplace<FModelNodeTableComponent>(root);
    entt::entity node0 = ecs.CreateEntity();
    ecs.GetRegistry().emplace<FTransformComponent>(node0);
    ecs.SetParent(node0, root);
    ecs.GetRegistry().get<FModelNodeTableComponent>(root).NodeEntities.push_back(node0);
    ecs.GetRegistry().emplace<FAnimationComponent>(root, 0u, 0.0f, true);

    ecs.Tick(0.5f); // advance-then-evaluate: t advances 0.5 → pos.x = 5

    ASSERT_NE(ecs.GetRegistry().try_get<FTransformComponent>(node0), nullptr);
    EXPECT_NEAR(ecs.GetRegistry().get<FTransformComponent>(node0).Position.x, 5.0f, 0.001f);
    EXPECT_NEAR(ecs.GetRegistry().get<FTransformComponent>(node0).Position.y, 0.0f, 0.001f);
}

TEST(ECSTest, Animation_ClipIndexSelectsRequestedClip)
{
    FEntitySubsystem ecs;
    FFakeAssetRegistry fake;

    fake.AnimClips.emplace_back();
    fake.AnimClips[0].Channels.push_back({0, Scene::ECS::EAnimChannelMode::Translation, {{0.0f, float4(0.0f, 0.0f, 0.0f, 1.0f)}, {1.0f, float4(10.0f, 0.0f, 0.0f, 1.0f)}}});
    fake.AnimClips[0].TimeDuration = 1.0f;
    fake.AnimClips.emplace_back();
    fake.AnimClips[1].Channels.push_back({0, Scene::ECS::EAnimChannelMode::Translation, {{0.0f, float4(0.0f, 0.0f, 0.0f, 1.0f)}, {1.0f, float4(20.0f, 0.0f, 0.0f, 1.0f)}}});
    fake.AnimClips[1].TimeDuration = 1.0f;

    ecs.RegisterProcessor(eastl::make_unique<FAnimationProcessor>(&fake));
    entt::entity root = ecs.CreateEntity();
    ecs.GetRegistry().emplace<FModelInstanceComponent>(root, FModelAssetHandle{0, 0}, "test");
    ecs.GetRegistry().emplace<FModelNodeTableComponent>(root);
    entt::entity node0 = ecs.CreateEntity();
    ecs.GetRegistry().emplace<FTransformComponent>(node0);
    ecs.SetParent(node0, root);
    ecs.GetRegistry().get<FModelNodeTableComponent>(root).NodeEntities.push_back(node0);
    ecs.GetRegistry().emplace<FAnimationComponent>(root, 1u, 0.0f, true); // play clip 1

    ecs.Tick(0.5f);

    EXPECT_NEAR(ecs.GetRegistry().get<FTransformComponent>(node0).Position.x, 10.0f, 0.001f);
}

TEST(ECSTest, Animation_NoClip_NoCrash)
{
    FEntitySubsystem ecs;
    FFakeAssetRegistry fake; // empty AnimClips
    ecs.RegisterProcessor(eastl::make_unique<FAnimationProcessor>(&fake));
    entt::entity root = ecs.CreateEntity();
    ecs.GetRegistry().emplace<FModelInstanceComponent>(root, FModelAssetHandle{0, 0}, "test");
    ecs.GetRegistry().emplace<FModelNodeTableComponent>(root);
    ecs.GetRegistry().emplace<FAnimationComponent>(root);
    ecs.Tick(0.1f); // clip == nullptr → 零求值，不崩
}

TEST(ECSTest, RenderSubmission_SkinnedEmitsSkinBatch)
{
    FEntitySubsystem ecs;
    FFakeRenderSink sink;
    FFakeAssetRegistry fake;
    fake.SkeletonInfo.JointNodeIndices = {0};
    fake.SkeletonInfo.InverseBindMatrices = {float4x4({identity})};

    const auto model = fake.ImportModel("test.gltf");
    const auto meshHandle = fake.AcquireMesh(model, 0, 0);
    fake.SetFakeSkinnedMesh(meshHandle, true);

    entt::entity root = ecs.CreateEntity();
    ecs.GetRegistry().emplace<FTransformComponent>(root);
    ecs.GetRegistry().emplace<FWorldTransformComponent>(root);
    ecs.GetRegistry().emplace<FModelInstanceComponent>(root, model, "test");
    ecs.GetRegistry().emplace<FModelNodeTableComponent>(root);
    entt::entity node0 = ecs.CreateEntity();
    ecs.GetRegistry().emplace<FTransformComponent>(node0);
    ecs.GetRegistry().emplace<FWorldTransformComponent>(node0);
    ecs.SetParent(node0, root);
    ecs.GetRegistry().get<FModelNodeTableComponent>(root).NodeEntities.push_back(node0);

    entt::entity mesh = ecs.CreateEntity();
    ecs.GetRegistry().emplace<FMeshRendererComponent>(mesh, meshHandle, fake.AcquireMaterial(model, 0, 0), true);
    ecs.GetRegistry().emplace<FTransformComponent>(mesh);
    ecs.GetRegistry().emplace<FWorldTransformComponent>(mesh);
    ecs.GetRegistry().emplace<FBoundingSphereComponent>(mesh, float3(0.0f), 1.0f);
    ecs.GetRegistry().emplace<FWorldBoundingSphereComponent>(mesh);
    ecs.GetRegistry().emplace<FVisibilityComponent>(mesh, true);
    ecs.GetRegistry().emplace<FSkinnedRootComponent>(mesh, root);
    ecs.SetParent(mesh, node0);

    ecs.RegisterProcessor(eastl::make_unique<FHierarchyProcessor>());
    ecs.RegisterProcessor(eastl::make_unique<FRenderSubmissionProcessor>(sink, fake));

    ecs.Tick(0.0f);

    EXPECT_GE(sink.AnimationBatchCount, 1);
    EXPECT_NE(sink.LastSkinInput.JointMatricesAddress, 0);
}

TEST(ECSTest, ComputeLightDirection_MatchesLegacyTick)
{
    const quaternion rot = RotationQuat(float3(15.0f, 0.0f, 0.0f));
    const float3 dir = Scene::ECS::ComputeLightDirection(rot);
    EXPECT_NEAR(length(dir), 1.0f, 0.001f);
    EXPECT_NEAR(dir.y, cos(DegreeToRadian(15.0f)), 0.001f);
    EXPECT_NEAR(dir.z, sin(DegreeToRadian(15.0f)), 0.001f);
}

TEST(ECSTest, Animation_EvalChannel_ZeroInterval_NoNaN)
{
    FEntitySubsystem ecs;
    FFakeAssetRegistry fake;
    fake.AnimClips.emplace_back();
    fake.AnimClips[0].Channels.push_back({0, Scene::ECS::EAnimChannelMode::Translation, {{0.5f, float4(1, 1, 1, 1)}, {0.5f, float4(2, 2, 2, 1)}}}); // duplicate timestamps (D10)
    fake.AnimClips[0].TimeDuration = 1.0f;
    ecs.RegisterProcessor(eastl::make_unique<FAnimationProcessor>(&fake));

    entt::entity root = ecs.CreateEntity();
    ecs.GetRegistry().emplace<FModelInstanceComponent>(root, FModelAssetHandle{0, 0}, "test");
    ecs.GetRegistry().emplace<FModelNodeTableComponent>(root);
    entt::entity node0 = ecs.CreateEntity();
    ecs.GetRegistry().emplace<FTransformComponent>(node0);
    ecs.SetParent(node0, root);
    ecs.GetRegistry().get<FModelNodeTableComponent>(root).NodeEntities.push_back(node0);
    ecs.GetRegistry().emplace<FAnimationComponent>(root, 0u, 0.0f, true);

    ecs.Tick(0.5f);

    const auto* tf = ecs.GetRegistry().try_get<FTransformComponent>(node0);
    ASSERT_NE(tf, nullptr);
    for (float c : {tf->Position.x, tf->Position.y, tf->Position.z})
        EXPECT_FALSE(std::isnan(c));
}

TEST(ECSTest, EntityFactory_SetMeshRenderer_InstallsDerivedComponents)
{
    Scene::ECS::FEntitySubsystem ecs;
    FFakeAssetRegistry fakeReg;

    auto model = fakeReg.ImportModel("test.gltf");
    const Scene::ECS::FMeshResourceHandle mesh = fakeReg.AcquireMesh(model, 0, 0);
    const Scene::ECS::FMaterialResourceHandle mat = fakeReg.AcquireMaterial(model, 0, 0);

    Scene::ECS::FMeshGPUData md = fakeReg.GetMeshData(mesh);
    md.Center = float3(1.0f, 2.0f, 3.0f);
    md.Radius = 4.0f;
    md.bSkinned = true;

    entt::entity e = ecs.CreateEntity();
    entt::entity root = ecs.CreateEntity();
    Scene::ECS::FEntityFactory::SetMeshRenderer(ecs.GetRegistry(), e, mesh, mat, md, model, 2, 5, root);

    auto& reg = ecs.GetRegistry();
    EXPECT_TRUE(reg.all_of<Scene::ECS::FMeshRendererComponent>(e));
    EXPECT_TRUE(reg.all_of<Scene::ECS::FBoundingSphereComponent>(e));
    EXPECT_TRUE(reg.all_of<Scene::ECS::FWorldBoundingSphereComponent>(e));
    EXPECT_TRUE(reg.all_of<Scene::ECS::FVisibilityComponent>(e));
    EXPECT_TRUE(reg.all_of<Scene::ECS::FSkinnedRootComponent>(e));

    const auto& mc = reg.get<Scene::ECS::FMeshRendererComponent>(e);
    EXPECT_EQ(mc.MeshHandle, mesh);
    EXPECT_EQ(mc.MaterialHandle, mat);
    EXPECT_TRUE(mc.Asset == model);
    EXPECT_EQ(mc.NodeIndex, 2);
    EXPECT_EQ(mc.PrimitiveIndex, 5);
    EXPECT_TRUE(mc.bSkinned);

    EXPECT_NEAR(reg.get<Scene::ECS::FBoundingSphereComponent>(e).Center.x, 1.0f, 0.001f);
    EXPECT_NEAR(reg.get<Scene::ECS::FBoundingSphereComponent>(e).Radius, 4.0f, 0.001f);
    EXPECT_TRUE(reg.get<Scene::ECS::FVisibilityComponent>(e).bFrustumVisible);
    EXPECT_EQ(reg.get<Scene::ECS::FSkinnedRootComponent>(e).Root, root);
}

TEST(ECSTest, EntityFactory_ApplyMeshKey_PreservesSourceIdentity)
{
    Scene::ECS::FEntitySubsystem ecs;
    FFakeAssetRegistry fakeReg;

    auto model = fakeReg.ImportModel("test.gltf");
    const Scene::ECS::FMeshResourceHandle mesh0 = fakeReg.AcquireMesh(model, 0, 0);
    const Scene::ECS::FMaterialResourceHandle mat0 = fakeReg.AcquireMaterial(model, 0, 0);
    fakeReg.AcquireMesh(model, 0, 1);   // second primitive available for the swap
    fakeReg.AcquireMaterial(model, 0, 1);

    Scene::ECS::FMeshGPUData md = fakeReg.GetMeshData(mesh0);
    entt::entity e = ecs.CreateEntity();
    Scene::ECS::FEntityFactory::SetMeshRenderer(ecs.GetRegistry(), e, mesh0, mat0, md, model, 0, 0);

    EXPECT_TRUE(Scene::ECS::FEntityFactory::ApplyMeshKey(ecs, fakeReg, e, model, 0, 1));

    const auto& mc = ecs.GetRegistry().get<Scene::ECS::FMeshRendererComponent>(e);
    EXPECT_EQ(mc.NodeIndex, 0);
    EXPECT_EQ(mc.PrimitiveIndex, 1);
    EXPECT_TRUE(mc.SourceAsset == model);
    EXPECT_EQ(mc.SourceNodeIndex, 0);
    EXPECT_EQ(mc.SourcePrimitiveIndex, 0);
}

TEST(ECSTest, EntityFactory_SetMeshRenderer_NullRootKeepsExistingSkinnedRoot)
{
    Scene::ECS::FEntitySubsystem ecs;
    FFakeAssetRegistry fakeReg;

    auto model = fakeReg.ImportModel("test.gltf");
    const Scene::ECS::FMeshResourceHandle mesh = fakeReg.AcquireMesh(model, 0, 0);
    const Scene::ECS::FMaterialResourceHandle mat = fakeReg.AcquireMaterial(model, 0, 0);

    Scene::ECS::FMeshGPUData md = fakeReg.GetMeshData(mesh);
    md.bSkinned = true;

    auto& reg = ecs.GetRegistry();
    entt::entity e = ecs.CreateEntity();
    entt::entity root = ecs.CreateEntity();
    Scene::ECS::FEntityFactory::SetMeshRenderer(reg, e, mesh, mat, md, model, 0, 0, root);

    // Same-asset skinned swap: skinnedRoot is null on purpose and the previously
    // resolved root must survive, while Source* keeps the original key.
    Scene::ECS::FEntityFactory::SetMeshRenderer(reg, e, mesh, mat, md, model, 0, 1, entt::null);

    ASSERT_TRUE(reg.all_of<Scene::ECS::FSkinnedRootComponent>(e));
    EXPECT_EQ(reg.get<Scene::ECS::FSkinnedRootComponent>(e).Root, root);
    const auto& mc = reg.get<Scene::ECS::FMeshRendererComponent>(e);
    EXPECT_EQ(mc.PrimitiveIndex, 1);
    EXPECT_EQ(mc.SourcePrimitiveIndex, 0);
}

TEST(ECSTest, SceneSerializer_SaveLoad_MeshOverrideRoundtrip)
{
    Scene::ECS::FEntitySubsystem ecs;
    FFakeAssetRegistry fakeReg;

    auto model = fakeReg.ImportModel("test.gltf");
    Scene::ECS::FModelNodeInfo node;
    node.Name = "Node0";
    node.NodeIndex = 0;
    node.Primitives.push_back(0);
    eastl::vector<Scene::ECS::FModelNodeInfo> nodes;
    nodes.push_back(node);
    fakeReg.SetFakeModelNodes(model, nodes);

    Scene::ECS::FEntityFactory factory(ecs, fakeReg);
    const Scene::ECS::FModelAssetInfo info = fakeReg.GetModelInfo(model);
    entt::entity root = factory.CreateModelInstance(model, "test.gltf");
    factory.CreateModelHierarchy(model, info, root, float3(0.0f), quaternion(0.0f, 0.0f, 0.0f, 1.0f), float3(1.0f));

    auto& reg = ecs.GetRegistry();
    const auto& table = reg.get<Scene::ECS::FModelNodeTableComponent>(root);
    ASSERT_FALSE(table.NodeEntities.empty());
    const auto* children = ecs.FindChildren(table.NodeEntities[0]);
    ASSERT_NE(children, nullptr);
    entt::entity meshEntity = entt::null;
    for (entt::entity c : *children)
    {
        if (reg.all_of<Scene::ECS::FMeshRendererComponent>(c)) { meshEntity = c; break; }
    }
    ASSERT_TRUE(meshEntity != entt::null);
    fakeReg.AcquireMesh(model, 0, 1);
    fakeReg.AcquireMaterial(model, 0, 1);
    EXPECT_TRUE(Scene::ECS::FEntityFactory::ApplyMeshKey(ecs, fakeReg, meshEntity, model, 0, 1));

    const char* path = "ECSTest_TmpMeshOverride.xml";
    EXPECT_TRUE(Scene::ECS::FSceneSerializer::Save(ecs, fakeReg, path, ""));

    Scene::ECS::FEntitySubsystem ecs2;
    FFakeAssetRegistry fakeReg2;
    Scene::ECS::FEntityFactory factory2(ecs2, fakeReg2);
    Scene::ECS::FSceneLoadResult result;
    EXPECT_TRUE(Scene::ECS::FSceneSerializer::Load(ecs2, factory2, fakeReg2, path, "", result));
    ASSERT_EQ(result.Models.size(), 1u);
    ASSERT_EQ(result.MeshOverrides.size(), 1u);
    EXPECT_EQ(result.MeshOverrides[0].SourceNodeIndex, 0);
    EXPECT_EQ(result.MeshOverrides[0].SourcePrimitiveIndex, 0);
    EXPECT_EQ(result.MeshOverrides[0].TargetNodeIndex, 0);
    EXPECT_EQ(result.MeshOverrides[0].TargetPrimitiveIndex, 1);

    remove(path);
}

TEST(ECSTest, SceneSerializer_Save_NestedModelInstance_OverrideOnlyOnOwner)
{
    Scene::ECS::FEntitySubsystem ecs;
    FFakeAssetRegistry fakeReg;

    auto model = fakeReg.ImportModel("test.gltf");
    Scene::ECS::FModelNodeInfo node;
    node.Name = "Node0";
    node.NodeIndex = 0;
    node.Primitives.push_back(0);
    eastl::vector<Scene::ECS::FModelNodeInfo> nodes;
    nodes.push_back(node);
    fakeReg.SetFakeModelNodes(model, nodes);

    Scene::ECS::FEntityFactory factory(ecs, fakeReg);
    const Scene::ECS::FModelAssetInfo info = fakeReg.GetModelInfo(model);
    entt::entity rootA = factory.CreateModelInstance(model, "test.gltf");
    factory.CreateModelHierarchy(model, info, rootA, float3(0.0f), quaternion(0.0f, 0.0f, 0.0f, 1.0f), float3(1.0f));
    entt::entity rootB = factory.CreateModelInstance(model, "test.gltf");
    factory.CreateModelHierarchy(model, info, rootB, float3(0.0f), quaternion(0.0f, 0.0f, 0.0f, 1.0f), float3(1.0f));
    ecs.SetParent(rootB, rootA);   // nested model instance

    // Edit only B's mesh entity (prim 0 -> 1).
    auto& reg = ecs.GetRegistry();
    const auto& tableB = reg.get<Scene::ECS::FModelNodeTableComponent>(rootB);
    ASSERT_FALSE(tableB.NodeEntities.empty());
    const auto* childrenB = ecs.FindChildren(tableB.NodeEntities[0]);
    ASSERT_NE(childrenB, nullptr);
    entt::entity meshB = entt::null;
    for (entt::entity c : *childrenB)
    {
        if (reg.all_of<Scene::ECS::FMeshRendererComponent>(c)) { meshB = c; break; }
    }
    ASSERT_TRUE(meshB != entt::null);
    fakeReg.AcquireMesh(model, 0, 1);
    fakeReg.AcquireMaterial(model, 0, 1);
    EXPECT_TRUE(Scene::ECS::FEntityFactory::ApplyMeshKey(ecs, fakeReg, meshB, model, 0, 1));

    const char* path = "ECSTest_TmpNestedOverride.xml";
    EXPECT_TRUE(Scene::ECS::FSceneSerializer::Save(ecs, fakeReg, path, ""));

    Scene::ECS::FEntitySubsystem ecs2;
    FFakeAssetRegistry fakeReg2;
    Scene::ECS::FEntityFactory factory2(ecs2, fakeReg2);
    Scene::ECS::FSceneLoadResult result;
    EXPECT_TRUE(Scene::ECS::FSceneSerializer::Load(ecs2, factory2, fakeReg2, path, "", result));
    remove(path);

    ASSERT_EQ(result.MeshOverrides.size(), 1u);
    bool bOwnedByB = false;
    for (const auto& m : result.Models)
    {
        if (m.Root == result.MeshOverrides[0].SourceRoot) { bOwnedByB = true; break; }
    }
    EXPECT_TRUE(bOwnedByB);
    EXPECT_EQ(result.MeshOverrides[0].SourceNodeIndex, 0);
    EXPECT_EQ(result.MeshOverrides[0].SourcePrimitiveIndex, 0);
    EXPECT_EQ(result.MeshOverrides[0].TargetNodeIndex, 0);
    EXPECT_EQ(result.MeshOverrides[0].TargetPrimitiveIndex, 1);
}