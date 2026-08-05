#include "Scene/ECS/EntitySubsystem.hpp"
#include "Scene/ECS/ProcessorBase.hpp"
#include "Scene/ECS/Components.hpp"
#include "Scene/ECS/Processors/HierarchyProcessor.hpp"
#include "Scene/ECS/Processors/CullingProcessor.hpp"
#include "Editor/Commands/EntityEditableTarget.hpp"
#include "Editor/Commands/CommandHistory.hpp"
#include "Editor/Commands/TransformCommand.hpp"

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