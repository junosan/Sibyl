/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#include "ThreadPool.h"

namespace sibyl
{

ThreadPool::ThreadPool(std::size_t threadCount)
:	paused(true),
    terminate(false),
    started(false),
    waitDone(false),
    threadsWaiting(0)
{
    threads.reserve(threadCount);

    for(auto i = 0u; i < threadCount; ++i)
        threads.emplace_back(&ThreadPool::ThreadMain, this);
    
    Wait(); // wait until all threads are ready so that Start() can function properly
}

ThreadPool::~ThreadPool()
{
    Clear();

    // Tell threads to stop when they can
    terminate = true;
    jobsAvailable.notify_all();

    // Wait for all threads to finish
    for(auto &t : threads)
    {
        if(t.joinable())
            t.join();
    }
}

std::size_t ThreadPool::ThreadCount() const
{
    return threads.size();
}

std::size_t ThreadPool::WaitingJobs() const
{
    std::lock_guard<std::mutex> lock(jobsMutex);
    return jobs.size();
}

void ThreadPool::Clear()
{
    std::lock_guard<std::mutex> lock(jobsMutex);

    while (jobs.empty() == false)
        jobs.pop();
}

void ThreadPool::Start()
{
    if (paused == true)
    {
        paused = false;

        if (WaitingJobs() > 0)
        {
            started = false;
            jobsAvailable.notify_all();

            // Block until at least one job started so that Wait() can function properly
            std::mutex startMutex;
            std::unique_lock<std::mutex> startLock(startMutex);
            starting.wait(startLock, [&]() {
                return started == true;
            });
        }
    }
}

void ThreadPool::Pause()
{
    paused = true;
}

void ThreadPool::Wait()
{
    std::mutex waitMutex;
    std::unique_lock<std::mutex> waitLock(waitMutex);

    allWaiting.wait(waitLock, [&]() {
        return waitDone == true;
    });
}

void ThreadPool::ThreadMain()
{
    while (true)
    {
        if(terminate == true)
            break;

        std::unique_lock<std::mutex> jobsLock(jobsMutex);

        if(jobs.empty() == true || paused == true)
        {
            ++threadsWaiting;
            if (threadsWaiting == threads.size())
            {
                waitDone = true;
                allWaiting.notify_all();
            }

            jobsAvailable.wait(jobsLock, [&]() { // unlocked upon entering wait
                return terminate == true || !(jobs.empty() == true || paused == true);
            }); // relocked

            --threadsWaiting;
            waitDone = false;

            if (started == false)
            {
                started = true;
                starting.notify_all();
            }
        }

        if(terminate == true) // check again since wait can take time
            break;

        // Take the next job out of the queue
        auto job = std::move(jobs.front());
        jobs.pop();

        jobsLock.unlock();

        // Actually perform the job (time consuming)
        job();

    } // task (a std::shared_ptr reference captured by job) destroyed here
}

}