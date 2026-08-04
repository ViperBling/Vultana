#include <gtest/gtest.h>

#include "Editor/Commands/CommandHistory.hpp"
#include "Editor/Commands/TransformCommand.hpp"
#include "Editor/Commands/ObjectEditableTarget.hpp"

#include <EASTL/memory.h>

namespace
{
    // Scene::IVisibleObject 最小桩：Create/Tick 是纯虚必须实现，其余继承默认行为
    class FTestVisibleObject : public Scene::IVisibleObject
    {
    public:
        bool Create() override { return true; }
        void Tick(float deltaTime) override { (void)deltaTime; }
    };
}

TEST(UndoTest, UndoRedoRestoresState)
{
    FTestVisibleObject object;
    Editor::Commands::FObjectEditableTarget target(object);

    Editor::Commands::FTransformSnapshot before = target.GetTransform();

    object.SetPosition(float3(10.0f, 20.0f, 30.0f));
    Editor::Commands::FTransformSnapshot after = target.GetTransform();

    Editor::Commands::FCommandHistory history;
    history.Push(eastl::make_unique<Editor::Commands::FSetTransformCommand>(
        eastl::make_unique<Editor::Commands::FObjectEditableTarget>(object), before, after));

    ASSERT_EQ(history.GetUndoCount(), 1u);

    ASSERT_TRUE(history.Undo());
    EXPECT_TRUE(object.GetPosition() == before.Position);
    EXPECT_EQ(history.GetUndoCount(), 0u);
    EXPECT_EQ(history.GetRedoCount(), 1u);

    ASSERT_TRUE(history.Redo());
    EXPECT_TRUE(object.GetPosition() == after.Position);
    EXPECT_EQ(history.GetUndoCount(), 1u);
    EXPECT_EQ(history.GetRedoCount(), 0u);
}

TEST(UndoTest, EmptyStackUndoRedoIsNoOp)
{
    Editor::Commands::FCommandHistory history;
    EXPECT_FALSE(history.Undo());
    EXPECT_FALSE(history.Redo());
    EXPECT_EQ(history.GetUndoCount(), 0u);
    EXPECT_EQ(history.GetRedoCount(), 0u);
}

TEST(UndoTest, PushClearsRedoStack)
{
    FTestVisibleObject object;
    Editor::Commands::FCommandHistory history;

    auto push = [&object, &history](const float3 &pos)
    {
        Editor::Commands::FTransformSnapshot before = Editor::Commands::FObjectEditableTarget(object).GetTransform();
        object.SetPosition(pos);
        Editor::Commands::FTransformSnapshot after = Editor::Commands::FObjectEditableTarget(object).GetTransform();
        history.Push(eastl::make_unique<Editor::Commands::FSetTransformCommand>(
            eastl::make_unique<Editor::Commands::FObjectEditableTarget>(object), before, after));
    };

    push(float3(1.0f, 0.0f, 0.0f));
    push(float3(2.0f, 0.0f, 0.0f)); // 不同 before → 不合并，两条命令
    ASSERT_EQ(history.GetUndoCount(), 2u);

    ASSERT_TRUE(history.Undo());
    EXPECT_TRUE(object.GetPosition() == float3(1.0f, 0.0f, 0.0f));
    ASSERT_EQ(history.GetRedoCount(), 1u);

    push(float3(3.0f, 0.0f, 0.0f)); // 新编辑 → redo 栈被清空
    EXPECT_EQ(history.GetRedoCount(), 0u);
    EXPECT_EQ(history.GetUndoCount(), 2u);
}

TEST(UndoTest, MaxDepthTruncatesOldest)
{
    FTestVisibleObject object;
    Editor::Commands::FCommandHistory history(3);

    for (int i = 0; i < 5; ++i)
    {
        Editor::Commands::FTransformSnapshot before = Editor::Commands::FObjectEditableTarget(object).GetTransform();
        object.SetPosition(float3(float(i), 0.0f, 0.0f));
        Editor::Commands::FTransformSnapshot after = Editor::Commands::FObjectEditableTarget(object).GetTransform();
        history.Push(eastl::make_unique<Editor::Commands::FSetTransformCommand>(
            eastl::make_unique<Editor::Commands::FObjectEditableTarget>(object), before, after));
    }

    EXPECT_EQ(history.GetUndoCount(), 3u);
}

TEST(UndoTest, SameTargetSameBeforeMerges)
{
    FTestVisibleObject object;
    Editor::Commands::FCommandHistory history;

    Editor::Commands::FTransformSnapshot before = Editor::Commands::FObjectEditableTarget(object).GetTransform();

    object.SetPosition(float3(5.0f, 0.0f, 0.0f));
    Editor::Commands::FTransformSnapshot after1 = Editor::Commands::FObjectEditableTarget(object).GetTransform();
    history.Push(eastl::make_unique<Editor::Commands::FSetTransformCommand>(
        eastl::make_unique<Editor::Commands::FObjectEditableTarget>(object), before, after1));

    object.SetPosition(float3(9.0f, 0.0f, 0.0f));
    Editor::Commands::FTransformSnapshot after2 = Editor::Commands::FObjectEditableTarget(object).GetTransform();
    history.Push(eastl::make_unique<Editor::Commands::FSetTransformCommand>(
        eastl::make_unique<Editor::Commands::FObjectEditableTarget>(object), before, after2)); // 同 before → 合并

    ASSERT_EQ(history.GetUndoCount(), 1u); // 合并为一条

    ASSERT_TRUE(history.Undo());
    EXPECT_TRUE(object.GetPosition() == before.Position); // 回到 before
    EXPECT_EQ(history.GetUndoCount(), 0u);

    ASSERT_TRUE(history.Redo());
    EXPECT_TRUE(object.GetPosition() == after2.Position); // 合并后的 after 生效
}

TEST(UndoTest, ObjectEditableTargetRoundTrips)
{
    FTestVisibleObject object;
    Editor::Commands::FObjectEditableTarget target(object);

    const float3 pos = float3(3.0f, -2.0f, 1.0f);
    const quaternion rot = RotationQuat(float3(0.0f, 90.0f, 0.0f));
    const float3 scl = float3(2.0f, 2.0f, 2.0f);

    Editor::Commands::FTransformSnapshot snapshot;
    snapshot.Position = pos;
    snapshot.Rotation = rot;
    snapshot.Scale = scl;

    target.SetTransform(snapshot);
    Editor::Commands::FTransformSnapshot readBack = target.GetTransform();

    EXPECT_TRUE(readBack.Position == pos);
    EXPECT_TRUE(readBack.Rotation == rot);
    EXPECT_TRUE(readBack.Scale == scl);
}