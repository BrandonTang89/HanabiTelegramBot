#ifndef TELEGRAM_CLIENT_COROUTINE_HPP
#define TELEGRAM_CLIENT_COROUTINE_HPP
#include <coroutine>
#include <iostream>
#include <queue>

#include "pch.h"

// Lazily Started Task
class [[nodiscard]] Task {
   public:
    struct FinalAwaiter {  // awaiter that is called when the final suspend point is reached
        bool await_ready() const noexcept { return false; }
        template <typename P>
        auto await_suspend(std::coroutine_handle<P> handle) noexcept {
            return handle.promise().continuation;  // symmetric transfer to resume the caller's coroutine
        }
        void await_resume() const noexcept {}
    };

    struct Promise {
        std::coroutine_handle<> continuation = nullptr;
        Task get_return_object() {
            return Task{std::coroutine_handle<Promise>::from_promise(*this)};
        }

        void unhandled_exception() noexcept {}
        void return_void() noexcept {}
        std::suspend_always initial_suspend() noexcept { return {}; }  // suspend immediately, the caller should put the coroutine on the stack to allow message passing
        FinalAwaiter final_suspend() noexcept { return {}; }
    };
    using promise_type = Promise;
    std::coroutine_handle<Promise> handle_ = nullptr;

    struct Awaiter {  // used when we call co_await on a task
                      // The way we use this, since our task doesn't do co_yield, this should only run once
        std::coroutine_handle<Promise> handle;
        bool await_ready() const noexcept { return false; }
        auto await_suspend(std::coroutine_handle<> calling) noexcept {
            handle.promise().continuation = calling;  // store the caller's coroutine handle in the promise
            return handle;                            // symmetric transfer to start the calee coroutine
        }
        void await_resume() const noexcept {}
    };

    // Task is awaitable since it has a co_await operator that returns an awaiter
    auto operator co_await() noexcept {
        if (!handle_) {
            BOOST_LOG_TRIVIAL(fatal) << "Task is not properly initialised!";
        }
        return Awaiter{handle_};
    }

    // Tasks are not copyable, only movable
    Task(const Task&) = delete;
    Task&(operator=(const Task&)) = delete;
    Task(Task&& other) noexcept : handle_(other.handle_) {
        other.handle_ = nullptr;
    }
    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }

    ~Task() {  // destroy the coroutine when the task is destroyed
        if (handle_) {
            BOOST_LOG_TRIVIAL(trace) << "Destroying coroutine";
            handle_.destroy();
        }
    }

   private:
    explicit Task(std::coroutine_handle<Promise> handle)
        : handle_(handle) {
    }
};

template <typename T>
class MessageQueue {
   public:
    class AwaiterForMessage {
       public:
        AwaiterForMessage(MessageQueue<T>& msgQueue) : msgQueue_{msgQueue} {}

        bool await_ready() const noexcept {
            return !msgQueue_.queue_.empty();
        }

        void await_suspend(std::coroutine_handle<> handle) {
            msgQueue_.m_awaiter = handle;
        }

        T await_resume() {
            T value = std::move(msgQueue_.queue_.front());
            msgQueue_.queue_.pop();
            return value;
        }

       private:
        MessageQueue<T>& msgQueue_;
    };

    auto operator co_await() noexcept {
        return AwaiterForMessage(*this);
    }

    MessageQueue() = default;
    std::queue<T> queue_{};                      // the queue of messages
    std::coroutine_handle<> m_awaiter{nullptr};  // the coroutine handle of the current awaiter

    void pushMessage(T message) {
        queue_.push(message);
        if (m_awaiter && !m_awaiter.done()) {  // if there is a coroutine waiting for a message
            m_awaiter.resume();
        }
    }
};

#endif