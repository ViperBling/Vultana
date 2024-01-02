#if (WIN32)
#define PLATFORM_WINDOWS
#endif

#pragma once

#include <vector>
#include <cassert>
#include <queue>
#include <mutex>
#include <thread>
#include <future>
#include <memory>
#include <functional>
#include <type_traits>

#ifdef PLATFORM_WINDOWS
#include <Windows.h>
#endif

#include "String.hpp"

namespace Utility
{
    class NamedThread
    {
    public:
        NamedThread() = default;

        template <typename F, typename... Args>
        explicit NamedThread(const std::string& name, F&& task, Args&&... args)
        {
            mThread = std::thread([this, task = std::forward<F>(task), name](Args&&... args) -> void
            {
                SetThreadName(name);
                task(args...);
            }, std::forward<Args>(args)...);
        }

        NamedThread(NamedThread&& other) noexcept : mThread(std::move(other.mThread)) {}
        ~NamedThread() = default;

        NamedThread& operator=(NamedThread&& other) noexcept
        {
            mThread = std::move(other.mThread);
            return *this;
        }

        void Join()
        {
            mThread.join();
        }

    private:
        void SetThreadName(const std::string& name)
        {
        #ifdef PLATFORM_WINDOWS
            SetThreadDescription(mThread.native_handle(), StringUtils::ToWideString(name).c_str());
        #endif
        }

    private:
        std::thread mThread;
    };

    class ThreadPool
    {
    public:
        ThreadPool(const std::string& name, uint8_t threadCount) : mbStop(false)
        {
            mThreads.reserve(threadCount);
            for (auto i = 0; i < threadCount; i++)
            {
                std::string fullName = name + "-" + std::to_string(i);
                mThreads.emplace_back(NamedThread(fullName, [this]() -> void 
                {
                    while (true)
                    {
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> lock(mMutex);
                            mCondition.wait(lock, [this]() -> bool 
                            {
                                return mbStop || !mTasks.empty();
                            });
                            if (mbStop && mTasks.empty()) { return; }
                            task = std::move(mTasks.front());
                            mTasks.pop();
                        }
                        task();
                    }
                }));
            }
        }

        ~ThreadPool()
        {
            {
                std::unique_lock<std::mutex> lock(mMutex);
                mbStop = true;
            }
            mCondition.notify_all();
            for (auto& thread : mThreads)
            {
                thread.Join();
            }
        }

        template <typename F, typename... Args>
        auto EmplaceTask(F&& task, Args&&... args)
        {
            using RetType = std::invoke_result_t<F, Args...>;
            auto packagedTask = std::make_shared<std::packaged_task<RetType()>>(
                std::bind(std::forward<F>(task), std::forward<Args>(args)...)
            );
            auto result = packagedTask->get_future();
            {
                std::unique_lock<std::mutex> lock(mMutex);
                assert(!mbStop);
                mTasks.emplace([packagedTask]() -> void
                {
                    (*packagedTask)();
                });
            }
            mCondition.notify_one();
            return result;
        }

    private:
        bool mbStop;
        std::mutex mMutex;
        std::condition_variable mCondition;
        std::vector<NamedThread> mThreads;
        std::queue<std::function<void()>> mTasks;
    };

    class WorkerThread
    {
    public:
        explicit WorkerThread(const std::string& name) : mbStop(false), mbFlush(false)
        {
            mThread = NamedThread(name, [this]() -> void
            {
                while (true)
                {
                    bool needNotifyMainThread = false;
                    std::vector<std::function<void()>> taskToExecute;
                    {
                        std::unique_lock<std::mutex> lock(mMutex);
                        mTaskCondition.wait(lock, [this]() -> bool
                        {
                            return mbStop || !mTasks.empty() || mbFlush;
                        });

                        if (mbFlush)
                        {
                            taskToExecute.reserve(mTasks.size());
                            while (!mTasks.empty())
                            {
                                taskToExecute.emplace_back(std::move(mTasks.front()));
                                mTasks.pop();
                            }
                            mbFlush = false;
                            needNotifyMainThread = true;
                        }
                        else
                        {
                            taskToExecute.emplace_back(std::move(mTasks.front()));
                            mTasks.pop();
                        }
                    }
                    for (auto & task : taskToExecute)
                    {
                        task();
                    }
                    if (needNotifyMainThread) { mFlushCondition.notify_one(); }
                }
            });
        }

        ~WorkerThread()
        {
            {
                std::unique_lock<std::mutex> lock(mMutex);
                mbStop = true;
            }
            mTaskCondition.notify_all();
            mThread.Join();
        }

        void Flush()
        {
            {
                std::unique_lock<std::mutex> lock(mMutex);
                mbFlush = true;
            }
            mTaskCondition.notify_one();
            {
                std::unique_lock<std::mutex> lock(mMutex);
                mFlushCondition.wait(lock);
            }
        }

        template <typename F, typename... Args>
        auto EmplaceTask(F&& task, Args&&... args)
        {
            using RetType = std::invoke_result_t<F, Args...>;
            auto packagedTask = std::make_shared<std::packaged_task<RetType()>>(
                std::bind(std::forward<F>(task), std::forward<Args>(args)...)
            );
            auto result = packagedTask->get_future();
            {
                std::unique_lock<std::mutex> lock(mMutex);
                assert(!mbStop);
                mTasks.emplace([packagedTask]() -> void { (*packagedTask)(); });
            }
            mTaskCondition.notify_one();
            return result;
        }

    private:
        bool mbStop;
        bool mbFlush;
        std::mutex mMutex;
        std::condition_variable mTaskCondition;
        std::condition_variable mFlushCondition;
        NamedThread mThread;
        std::queue<std::function<void()>> mTasks;
    };
}