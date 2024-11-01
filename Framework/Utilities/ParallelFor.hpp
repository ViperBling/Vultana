#pragma once

#include "Core/VultanaEngine.hpp"
#include <enkiTS/TaskScheduler.h>

namespace Utilities
{
    template <typename F>
    inline void ParallelFor(uint32_t begin, uint32_t end, F func)
    {
        enki::TaskScheduler* ts = Core::VultanaEngine::GetEngineInstance()->GetTaskScheduler();
        enki::TaskSet taskSet(end - begin + 1, [&](enki::TaskSetPartition range, uint32_t threadNum)
        {
            for (uint32_t i = range.start; i != range.end; ++i)
            {
                func(i + begin);
            }
        });
        ts->AddTaskSetToPipe(&taskSet);
        ts->WaitforTask(&taskSet);
    }

    template <typename F>
    inline void ParallelFor(uint32_t count, F func)
    {
        ParallelFor(0, count - 1, func);
    }
}
