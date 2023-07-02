#pragma once

#include <cstdint>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <future>

namespace w130205
{

class ThreadPool
{
public:
    explicit ThreadPool(std::size_t thread_num) : thdNum(thread_num)
    {
        thds.reserve(thdNum);
        for (size_t i = 0; i < thdNum; ++i) {
            thds.emplace_back(&ThreadPool::worker, this);
        }
    }
    ThreadPool(const ThreadPool&)                 = delete;
    ThreadPool(ThreadPool &&) noexcept            = delete;
    ThreadPool& operator=(const ThreadPool&)      = delete;
    ThreadPool& operator=(ThreadPool &&) noexcept = delete;

    ~ThreadPool()
    {
        release();
    }

    template<typename F, typename ...Args>
    auto submit(F&& f, Args&&... args)
        -> std::pair<bool, std::future<std::invoke_result_t<F, Args...>>>
    {
        using ReturnType = std::invoke_result_t<F, Args...>;
        std::packaged_task<ReturnType()> packedTask(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        std::future<ReturnType> futureRet = packedTask.get_future();
        {
            std::lock_guard<std::mutex> lg(mtx);
            if (stop) {
                return { false, std::future<ReturnType>{} };
            }
            tasks.emplace([packedTask = std::move(packedTask)]() mutable { packedTask(); });
            ++totalTaskNum;
            ++remainingTaskNum;
        }
        cond.notify_one();
        return { true, std::move(futureRet) };
    }

    void release()
    {
        {
            std::lock_guard<std::mutex> lg(mtx);
            if (stop) {
                return;
            }
            stop = true;
        }
        cond.notify_all(); // 置stop,通知所有任务线程,等待剩余任务全部执行完
        for (auto &thd : thds) {
            if (thd.joinable()) {
                thd.join();
            }
        }
    }

private:
    void worker()
    {
        func_wrapper task;
        while (true) {
            {
                std::unique_lock<std::mutex> lk(mtx);
                cond.wait(lk,
                    [this]() {
                        return !tasks.empty() || stop; 
                    });
                if (!tasks.empty()) {
                    task = std::move(tasks.front());
                    tasks.pop();
                    --remainingTaskNum;
                } else if (stop) {
                    return;
                }
            }
            if (task != nullptr) {
                task();
            }
        }
    }

    class func_wrapper
    {
    public:
        func_wrapper() = default;
        func_wrapper(func_wrapper&& other) noexcept : impl(std::move(other.impl)) {};
        func_wrapper& operator=(func_wrapper&& other) noexcept
        {
            impl = std::move(other.impl);
            return *this;
        }
        func_wrapper(const func_wrapper&) = delete;
        func_wrapper& operator=(const func_wrapper&) = delete;

        template<typename F>
        func_wrapper(F&& f)
            : impl(std::make_unique<impl_type<std::decay_t<F>>>(std::forward<F>(f))) {}

        void operator()() { impl->call(); }
        operator bool() { return static_cast<bool>(impl); }
        bool operator==(std::nullptr_t) { return impl == nullptr; }
        bool operator!=(std::nullptr_t) { return impl != nullptr; }

    private:
        struct impl_base {
            virtual void call() = 0;
            virtual ~impl_base() = default;
        };

        template<typename F>
        struct impl_type : impl_base
        {
            F f;
            impl_type(F&& f_) : f(std::forward<F>(f_)) {}
            void call() override { f(); }
        };

        std::unique_ptr<impl_base> impl;
    };

    bool                                stop { false };
    std::size_t                         thdNum { 0 };
    std::vector<std::thread>            thds;
    std::queue<func_wrapper>            tasks;
    std::size_t                         totalTaskNum { 0 };
    std::size_t                         remainingTaskNum { 0 };
    std::mutex                          mtx;
    std::condition_variable             cond;
};

} // namespace w130205