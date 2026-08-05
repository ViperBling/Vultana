#pragma once

#include <cstdint>
#include <functional>

#include <enkiTS/TaskScheduler.h>

namespace Tests
{
    // Singleton task scheduler for test use
    inline enki::TaskScheduler* GetTestTaskScheduler()
    {
        static enki::TaskScheduler sched;
        static bool initialized = false;
        if (!initialized)
        {
            sched.Initialize();
            initialized = true;
        }
        return &sched;
    }

    // ParallelFor for test use — uses a standalone enki::TaskScheduler, no engine dependency
    template <typename F>
    inline void ParallelFor(uint32_t count, F func)
    {
        auto* ts = GetTestTaskScheduler();
        enki::TaskSet taskSet(count, [&](enki::TaskSetPartition range, uint32_t /*threadNum*/)
        {
            for (uint32_t i = range.start; i != range.end; ++i)
                func(i);
        });
        ts->AddTaskSetToPipe(&taskSet);
        ts->WaitforTask(&taskSet);
    }

    // Type-erased wrapper matching FCullingProcessor::FParallelForFunc signature
    inline void ParallelForFunc(uint32_t count, const std::function<void(uint32_t)>& func)
    {
        ParallelFor(count, func);
    }
} // namespace Test