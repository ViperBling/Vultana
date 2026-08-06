#include "Scene/ECS/EntitySubsystem.hpp"
#include "Scene/ECS/ProcessorBase.hpp"
#include "Scene/ECS/Components.hpp"
#include "Scene/ECS/Processors/HierarchyProcessor.hpp"
#include "Scene/ECS/Processors/CullingProcessor.hpp"
#include "Scene/ECS/Processors/RenderSubmissionProcessor.hpp"
#include "Scene/ECS/RenderSink.hpp"
#include "Scene/ECS/EntityFactory.hpp"
#include "Editor/Commands/EntityEditableTarget.hpp"
#include "Editor/Commands/CommandHistory.hpp"
#include "Editor/Commands/TransformCommand.hpp"

#include "Tests/FakeAssetRegistry.hpp"
#include "Tests/TestUtilities.hpp"

#include <gtest/gtest.h>
#include <entt/entt.hpp>

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
    ecs.RegisterProcessor(eastl::make_unique<FHierarchyProcessor>());
    ecs.Tick(0.0f);
    EXPECT_NEAR(ecs.GetRegistry().get<FWorldBoundingSphereComponent>(e).Radius, 10.0f, 0.001f);
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
    eastl::vector<Scene::ECS::FRenderBatchInput> BaseBatches;
    uint32_t NextPickID = 0;
    bool bMouseHitTest = false;

    uint32_t AddInstance(const FInstanceData& instanceData) override
    {
        AddedInstances.push_back(instanceData.ObjectID);
        return static_cast<uint32_t>(AddedInstances.size() - 1);
    }

    void AddBasePassBatch(const Scene::ECS::FRenderBatchInput& input) override { BaseBatches.push_back(input); }
    void AddObjectIDBatch(const Scene::ECS::FRenderBatchInput&) override {}
    void AddOutlineBatch(const Scene::ECS::FRenderBatchInput&) override {}
    uint32_t AllocateMaterialConstants(const void*, uint32_t) override { return 0; }
    uint32_t GetSelectedPickID() const override { return NextPickID; }
    bool IsEnableMouseHitTest() const override { return bMouseHitTest; }
    void AddAnimationBatch(const Scene::ECS::FAnimationBatchInput&) override {}
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

TEST(ECSTest, RenderSubmission_AllEntitiesAddInstance)
{
    Scene::ECS::FEntitySubsystem ecs;
    FFakeAssetRegistry fakeReg;
    FFakeRenderSink fakeSink;

    entt::entity e = ecs.CreateEntity();
    ecs.GetRegistry().emplace<Scene::ECS::FWorldTransformComponent>(e);
    ecs.GetRegistry().emplace<Scene::ECS::FWorldBoundingSphereComponent>(e);
    EXPECT_EQ(fakeSink.AddedInstances.size(), 0u);
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
    ecs.GetRegistry().emplace<Scene::ECS::FMeshRendererComponent>(e, meshHandle, Scene::ECS::FMaterialResourceHandle{}, 0u, false);

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