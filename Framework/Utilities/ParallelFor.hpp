#pragma once

#include "Core/VultanaEngine.hpp"
#include <enkiTS/TaskScheduler.h>

#define ENKI_MIN_GRAIN 512

namespace Utilities
{
    template <typename F>
    inline void ParallelFor(uint32_t begin, uint32_t end, F func, uint32_t minGrain = ENKI_MIN_GRAIN)
    {
        const uint32_t count = end - begin + 1;
        if (count <= minGrain)
        {
            for (uint32_t i = begin; i <= end; ++i)
                func(i);
            return;
        }
        enki::TaskScheduler* ts = Core::FVultanaEngine::GetEngineInstance()->GetTaskScheduler();
        enki::TaskSet taskSet(count, [&](enki::TaskSetPartition range, uint32_t threadNum)
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
    inline void ParallelFor(uint32_t count, F func, uint32_t minGrain = ENKI_MIN_GRAIN)
    {
        if (count == 0) return;
        ParallelFor(0, count - 1, func, minGrain);
    }
}
