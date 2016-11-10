/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#ifndef SIBYL_UTIL_THREADPOOL_H_
#define SIBYL_UTIL_THREADPOOL_H_

#include <atomic>
#include <mutex>
#include <thread>
#include <future>
#include <vector>
#include <functional>
#include <condition_variable>
#include <queue>

namespace sibyl
{

// Thread pool for running generic Callable operations in parallel 
// Member function pointers can also be used (see Callable/INVOKE specification)
// Assuming a Worker class with a Work() member function and an instance worker:
//     pool.Add(&Worker::Work, &worker);
class ThreadPool
{
public:
    // Starts threadCount threads, waiting in paused state
    // May throw a std::system_error if a thread could not be started
    ThreadPool(std::size_t threadCount = 4);

    // Clears job queue, then blocks until all threads finished executing their current job
    ~ThreadPool();
    
    // Rule of five
    ThreadPool           (const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool           (ThreadPool&&)      = default;
    ThreadPool& operator=(ThreadPool&&)      = default;

    // Store a function to be executed with its arguments as a bind expression
    // Can be called both while paused or running
    // If called while running, it will attempt to wake a thread (which may or may not succeed)
    template<typename Func, typename... Args>
    auto Add(Func&& func, Args&&... args) -> std::future<typename std::result_of<Func(Args...)>::type>;

    std::size_t ThreadCount() const;
    std::size_t WaitingJobs() const;

    void Start(); // blocks until at least one job stated (if applicable)
    void Pause(); // nonblocking, does not affect currently running jobs
    void Clear(); // nonblocking, does not affect currently running jobs

    // Blocks calling thread until job queue is empty or until pause is completed
    void Wait();

private:
    std::vector<std::thread> threads;

    // Query pool for jobs, wait/pause/terminate
    void ThreadMain();

    std::atomic<bool> paused;
    std::atomic<bool> terminate;

    mutable std::mutex jobsMutex; // mutable allows const intantiation
    std::condition_variable jobsAvailable; // Add(...), Start(), ~ThreadPool() -> ThreadMain()
    std::queue<std::function<void()>> jobs;

    std::condition_variable starting; // ThreadMain() -> Start()
    std::atomic<bool> started;

    std::condition_variable allWaiting; // ThreadMain() -> Wait()
    std::atomic<bool> waitDone;

    std::atomic<std::size_t> threadsWaiting;
};

template<typename Func, typename... Args>
auto ThreadPool::Add(Func&& func, Args&&... args) -> std::future<typename std::result_of<Func(Args...)>::type>
{
    using Task = std::packaged_task<typename std::result_of<Func(Args...)>::type()>;

    // Store as a Callable object (0 arguments) with arguments pre-stored in a bind expression
    // Task pointed to here will be destroyed later when job is popped and destroyed
    auto task_ptr = std::make_shared<Task>(std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
    auto ret = task_ptr->get_future();

    {
        std::lock_guard<std::mutex> lock(jobsMutex);
        jobs.emplace([task_ptr]() { (*task_ptr)(); }); // Note: capture by value

        // std::shared_ptr is captured by value and stored as if by
        // struct job {
        //     std::shared_ptr<Task> ptr;
        //     void operator() () { (*ptr)(); }
        // };
    }

    // Let a waiting thread know there is a job available
    // Note: notify_one()'s effect is not immediate, so calling Pause() immediately
    //       after Add(...) may prevent the thread from actually waking up 
    //       This behavior does not invalidate the state of the class
    if (paused == false)
        jobsAvailable.notify_one();

    return ret;
}

}

/*

Modified code from https://github.com/dabbertorres/ThreadPool

Original license:

    The MIT License (MIT)

    Copyright (c) 2015 Alec Iverson

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

*/

#endif /* SIBYL_UTIL_THREADPOOL_H_ */