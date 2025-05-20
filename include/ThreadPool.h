#pragma once
#include <vector>
#include <thread>
#include <future>
#include <coroutine>
#include <atomic>
#include <unordered_map>
#include "concurrentqueue.h"  // Requires the concurrentqueue library

class CoroutineThreadPool {
public:
    struct CoroutineTask {
        struct promise_type {
            CoroutineTask get_return_object() { return {}; }
            std::suspend_never initial_suspend() { return {}; }
            std::suspend_never final_suspend() noexcept { return {}; }
            void return_void() {}
            void unhandled_exception() { std::terminate(); }
        };
    };

    explicit CoroutineThreadPool(size_t threads, size_t max_coroutines_per_thread = 100) 
        : m_max_coroutines(max_coroutines_per_thread),
          m_stop(false) {
        for (size_t i = 0; i < threads; ++i) {
            m_threads.emplace_back([this] { worker_loop(); });
        }
    }

    ~CoroutineThreadPool() {
        m_stop = true;
        m_cv.notify_all();
        for (auto& t : m_threads) {
            if (t.joinable()) t.join();
        }
    }

    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        using ReturnType = decltype(f(args...));

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        auto res = task->get_future();
        
        // Using lock-free queue
        m_tasks.enqueue([this, task]() -> CoroutineTask {
            // Thread-local state tracking
            auto& local_state = get_thread_state();
            local_state.active_coroutines++;
            
            try {
                (*task)(); // Execute the actual task
            } catch (...) {
                local_state.active_coroutines--;
                throw;
            }
            
            local_state.active_coroutines--;
            co_return;
        });
        
        m_cv.notify_one();
        return res;
    }

private:
    struct ThreadState {
        std::atomic<size_t> active_coroutines{0};
    };

    ThreadState& get_thread_state() {
        static thread_local ThreadState* state = nullptr;
        if (!state) {
            std::lock_guard<std::mutex> lock(m_state_mutex);
            auto tid = std::this_thread::get_id();
            state = &m_thread_states[tid]; // Automatically create new entry
        }
        return *state;
    }

    void worker_loop() {
        // Initialize thread-local state
        get_thread_state();

        while (!m_stop) {
            // Try to dequeue a task without blocking first
            std::function<CoroutineTask()> task;
            if (m_tasks.try_dequeue(task)) {
                task(); // Execute the coroutine task
                continue;
            }

            // If no task was available, check if we should wait
            auto& state = get_thread_state();
            if (state.active_coroutines >= m_max_coroutines) {
                std::this_thread::yield();
                continue;
            }

            // Wait for a task with a timeout to periodically check m_stop
            std::unique_lock<std::mutex> lock(m_cv_mutex);
            m_cv.wait_for(lock, std::chrono::milliseconds(100), [&] {
                return m_stop || m_tasks.try_dequeue(task);
            });

            if (m_stop) return;
            if (task) {
                task();
            }
        }
    }

    const size_t m_max_coroutines;
    std::atomic<bool> m_stop;
    std::vector<std::thread> m_threads;
    
    // Lock-free queue for task storage
    moodycamel::ConcurrentQueue<std::function<CoroutineTask()>> m_tasks;
    
    // Thread state management
    std::mutex m_state_mutex;
    std::unordered_map<std::thread::id, ThreadState> m_thread_states;
    
    // Condition variable for worker synchronization
    std::mutex m_cv_mutex;
    std::condition_variable m_cv;
};