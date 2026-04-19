// MIT License - See header file for license text

#include "graph/ThreadPool.hpp"
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <log4cxx/logger.h>
#include "core/ActiveQueue.hpp"

namespace graph
{
    // Static assertions: Verify atomic lock-free guarantees (Phase 1 enhancement)
    // These ensure the platform supports true lock-free atomics for high-performance
    // synchronization without mutex acquisition.
    static_assert(std::atomic<bool>::is_always_lock_free,
                  "Critical: std::atomic<bool> must be lock-free for correct synchronization");
    static_assert(std::atomic<size_t>::is_always_lock_free,
                  "Critical: std::atomic<size_t> must be lock-free for correct synchronization");
    static_assert(std::atomic<uint64_t>::is_always_lock_free,
                  "Critical: std::atomic<uint64_t> must be lock-free for correct synchronization");
    // Static logger initialization
    log4cxx::LoggerPtr ThreadPool::logger_ = log4cxx::Logger::getLogger("graph.threadpool");

    ThreadPool::ThreadPool(size_t num_threads) noexcept
        : ThreadPool(num_threads, DeadlockConfig())
    {
    }

    ThreadPool::ThreadPool(size_t num_threads, const DeadlockConfig &config) noexcept
        : num_threads_(num_threads > 0 ? num_threads : std::thread::hardware_concurrency()),
          config_(config)
    {
        LOG4CXX_TRACE(logger_, "ThreadPool created with " << num_threads_ << " threads");
        LOG4CXX_TRACE(logger_, "Deadlock detection: " << (config_.enable_detection ? "enabled" : "disabled"));
    }

    ThreadPool::~ThreadPool() noexcept
    {
        LOG4CXX_TRACE(logger_, "ThreadPool destructor called " << active_tasks_.load() << " active tasks " << running_.load() << " running");
        LOG4CXX_TRACE(logger_, "ThreadPool destructor - queued tasks: " << task_queue_.Size());
        LOG4CXX_TRACE(logger_, "ThreadPool destructor - stats: " << stats_.tasks_completed
                                                             << " completed, " << stats_.tasks_failed << " failed");
        LOG4CXX_TRACE(logger_, "ThreadPool destructor - stop_requested: " << GetStopRequested());

        if( running_.load()) {
            Stop();
            // C++26: [[nodiscard]] requires explicit void cast for ignored return values
            // Intentionally ignoring timeout result - destructor will wait with best-effort
            static_cast<void>(JoinWithTimeout(std::chrono::milliseconds(5000)));
        }
    }

    bool ThreadPool::Init() noexcept
    {
        LOG4CXX_TRACE(logger_, "ThreadPool initializing with " << num_threads_ << " worker threads");
        // Configure the queue with capacity limit from config
        task_queue_.SetCapacity(config_.max_queue_size);
        return true;
    }

    bool ThreadPool::Start() noexcept
    {

        LOG4CXX_TRACE(logger_, "ThreadPool starting " << num_threads_ << " worker threads");

        try
        {
            running_.store(true);
            // Spawn worker threads
            for (size_t i = 0; i < num_threads_; ++i)
            {
                worker_threads_.emplace_back([this]
                                             { WorkerMain(); });
            }

            // Spawn deadlock watchdog if enabled
            if (config_.enable_detection)
            {
                watchdog_ = std::thread([this]
                                        { WatchdogMain(); });
            }

            LOG4CXX_TRACE(logger_, "ThreadPool started successfully");
            return true;
        }
        catch (const std::exception &e)
        {
            LOG4CXX_ERROR(logger_, "Failed to start ThreadPool: " << e.what());
            // Disable queue to wake up any workers that started
            task_queue_.Disable();
            // running_.store(false);
            return false;
        }
    }

    void ThreadPool::Stop() noexcept
    {
        LOG4CXX_TRACE(logger_, "ThreadPool stopping");

        SetStopRequested(true);
        // Disable the queue to wake up any workers blocked in Dequeue()
        // Workers will switch to DequeueNonBlocking to drain remaining tasks
        task_queue_.Disable();
    }

    void ThreadPool::Join() noexcept
    {
        LOG4CXX_TRACE(logger_, "ThreadPool Join");
        running_.store(false);

        // Wait for all workers to complete
        for (auto &worker : worker_threads_)
        {
            if (worker.joinable())
            {
                LOG4CXX_TRACE(logger_, "ThreadPool joining worker thread " << worker.get_id());
                worker.join();
            }
        }

        // Wait for watchdog
        if (watchdog_.joinable())
        {
            LOG4CXX_TRACE(logger_, "ThreadPool joining watchdog thread " << watchdog_.get_id());
            watchdog_.join();
        }

        // Drain any remaining tasks that weren't executed
        // (can happen if Stop() was called while tasks were queued)
        Task remaining_task;
        while (task_queue_.DequeueNonBlocking(remaining_task))
        {
            LOG4CXX_TRACE(logger_, "Discarding remaining queued task during Join");
        }

        LOG4CXX_TRACE(logger_, "ThreadPool joined - stats: " << stats_.tasks_completed
                                                             << " completed, " << stats_.tasks_failed << " failed");
    }

    bool ThreadPool::JoinWithTimeout(std::chrono::milliseconds timeout) noexcept
    {
        LOG4CXX_TRACE(logger_, "ThreadPool JoinWithTimeout " << timeout.count() << "ms");
        running_.store(false);
        std::unique_lock lock(join_mtx_);
        bool completed = join_cv_.wait_for(lock, timeout, [&]
                           { return live_workers_.load() == 0 &&
                                    active_tasks_.load() == 0 &&
                                    task_queue_.Size() == 0; });

        // Always join threads, regardless of timeout result
        for (auto &t : worker_threads_)
            if (t.joinable())
                t.join();

        if (watchdog_.joinable())
            watchdog_.join();

        return completed;
    }

    ThreadPool::QueueResult ThreadPool::QueueTask(Task task) noexcept
    {
        if (!task)
        {
            LOG4CXX_WARN(logger_, "Attempt to queue null task");
            return QueueResult::Error;
        }
        if (GetStopRequested())
        {
            LOG4CXX_TRACE(logger_, "ThreadPool not accepting tasks - stop requested");
            stats_.tasks_rejected++;  // Phase 2: Track rejections
            return QueueResult::Stopped;
        }
        QueueResult result = QueueResult::Error;
        // ActiveQueue handles capacity checking and returns false if disabled or full
        if (task_queue_.Enqueue(std::move(task)))
        {
            stats_.tasks_queued++;
            result = QueueResult::Ok;
        }
        else
        {
            LOG4CXX_TRACE(logger_, "ThreadPool not accepting tasks");
            stats_.tasks_rejected++;  // Phase 2: Track rejections
            // Distinguish between queue disabled vs. queue full
            result = task_queue_.Enabled() ? QueueResult::Full : QueueResult::Stopped;
        }

        return result;
    }

    ThreadPool::QueueResult ThreadPool::QueueTaskWithTimeout(Task task, std::chrono::milliseconds timeout) noexcept
    {
        if (!task)
        {
            LOG4CXX_WARN(logger_, "Attempt to queue null task");
            return QueueResult::Error;
        }
        if (GetStopRequested())
        {
            LOG4CXX_TRACE(logger_, "ThreadPool not accepting tasks - stop requested");
            stats_.tasks_rejected++;  // Phase 2: Track rejections
            return QueueResult::Stopped;
        }

        auto start = std::chrono::steady_clock::now();
        auto deadline = start + timeout;

        // Try repeatedly with timeout
        Task local = std::move(task);
        while (std::chrono::steady_clock::now() < deadline)
        {
            if (task_queue_.Enqueue(local))
            {
                stats_.tasks_queued++;
                return QueueResult::Ok;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        stats_.enqueue_timeouts++;  // Phase 2: Track timeouts
        return QueueResult::Timeout;
    }

    size_t ThreadPool::GetQueueDepth() const noexcept
    {
        return task_queue_.Size();
    }

    size_t ThreadPool::GetThreadCount() const noexcept
    {
        return num_threads_;
    }

    bool ThreadPool::IsDeadlockDetected() const noexcept
    {
        return deadlock_detected_.load();
    }

    void ThreadPool::ClearDeadlockFlag() noexcept
    {
        deadlock_detected_.store(false);
        LOG4CXX_TRACE(logger_, "Deadlock flag cleared");
    }

    // Phase 2 Enhancement: Calculate average task execution time
    double ThreadPool::GetAverageTaskTimeMs() const noexcept
    {
        size_t completed = stats_.tasks_completed.load();
        if (completed == 0) return 0.0;
        uint64_t total_ns = stats_.total_task_time_ns.load();
        return (total_ns / 1'000'000.0) / completed;
    }

    const ThreadPoolStats& ThreadPool::GetStats() const noexcept
    {
        return stats_;
    }

    void ThreadPool::WorkerMain()
    {
        try
        {
            LOG4CXX_TRACE(logger_, "Worker thread " << std::this_thread::get_id() << " started");
            live_workers_++;

            while (true)
            {
                Task task;

                // Try blocking dequeue if stop not requested
                if (!GetStopRequested())
                {
                    LOG4CXX_TRACE(logger_, "Worker thread " << std::this_thread::get_id() << " waiting for task (blocking)");
                    if (!task_queue_.Dequeue(task))
                    {
                        // Queue was disabled - switch to draining mode with nonblocking
                        LOG4CXX_TRACE(logger_, "Worker thread " << std::this_thread::get_id() << " blocking dequeue failed, switching to drain mode");
                        // Continue to drain loop below
                    }
                    else
                    {
                        // Got a task, execute it
                        LOG4CXX_TRACE(logger_, "Worker thread " << std::this_thread::get_id() << " executing task");
                        ExecuteTask(task);
                        continue;
                    }
                }

                // Drain remaining tasks using non-blocking dequeue
                // (either stop was requested or blocking dequeue failed)
                if (!task_queue_.DequeueNonBlocking(task))
                {
                    // No more items - we're done
                    LOG4CXX_TRACE(logger_, "Worker thread " << std::this_thread::get_id() << " no more tasks, exiting");
                    break;
                }

                LOG4CXX_TRACE(logger_, "Worker thread " << std::this_thread::get_id() << " executing drained task");
                ExecuteTask(task);
            }
        }
        catch (const std::exception &e)
        {
            LOG4CXX_ERROR(logger_, "Worker thread threw exception: " << e.what());
        }
        catch (...)
        {
            LOG4CXX_ERROR(logger_, "Worker thread threw unknown exception");
        }

        live_workers_--;
        join_cv_.notify_all();
        LOG4CXX_TRACE(logger_, "Worker thread " << std::this_thread::get_id() << " exiting");
    }

    void ThreadPool::WatchdogMain()
    {
        try
        {
            while (running_.load())
            {
                std::this_thread::sleep_for(config_.watchdog_interval);
                if (!running_.load())
                    break;

                auto now_ns = std::chrono::steady_clock::now().time_since_epoch().count();
                auto last_ns = last_task_start_ns_.load(std::memory_order_acquire);
                auto elapsed_ns = now_ns - last_ns;

                if (active_tasks_.load() > 0 && elapsed_ns > static_cast<size_t>(config_.task_timeout.count()) * 1'000'000)
                {
                    deadlock_detected_.store(true);
                    stats_.deadlock_detections++;
                }
            }
            // Phase 1 Enhancement: Lock-free queue flushing during shutdown
            // Note: Intentionally non-blocking to avoid deadlock if workers are stuck.
            // Size() may race with Dequeue(), but loop terminates when queue drains due to
            // FIFO ordering and single watchdog thread. This is safe by design.
            while(task_queue_.Size() > 0) {
                Task task;  
                // C++26: [[nodiscard]] return value intentionally unused (queue flush-only)
                static_cast<void>(task_queue_.DequeueNonBlocking(task));
            }
        }
        catch (const std::exception &e)
        {
            LOG4CXX_ERROR(logger_, "Watchdog thread threw exception: " << e.what());
        }
        catch (...)
        {
            LOG4CXX_ERROR(logger_, "Watchdog thread threw unknown exception");
        }
    }

    bool ThreadPool::ExecuteTask(const Task &task)
    {
        try
        {
            active_tasks_.fetch_add(1, std::memory_order_acq_rel);
            auto start_time = std::chrono::steady_clock::now();
            auto start_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                start_time.time_since_epoch()).count();
            last_task_start_ns_.store(start_ns, std::memory_order_release);

            LOG4CXX_TRACE(logger_, "Executing task, " << active_tasks_.load() << " active");

            task();

            // Phase 2: Track task execution time
            auto end_time = std::chrono::steady_clock::now();
            auto end_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                end_time.time_since_epoch()).count();
            auto task_duration_ns = end_ns - start_ns;
            stats_.total_task_time_ns += task_duration_ns;

            stats_.tasks_completed++;
            active_tasks_.fetch_sub(1, std::memory_order_acq_rel);

            return true;
        }
        catch (const std::exception &e)
        {
            LOG4CXX_ERROR(logger_, "Task execution failed: " << e.what());
            stats_.tasks_failed++;
            active_tasks_.fetch_sub(1, std::memory_order_acq_rel);
            return false;
        }
    }

} // namespace graph
