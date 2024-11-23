#ifndef TELEGRAM_CLIENT_COROUTINE_HPP
#define TELEGRAM_CLIENT_COROUTINE_HPP
#include <coroutine>
#include <queue>
#include <iostream>
#include "pch.h"

class [[nodiscard]] Task {
   public:
    struct FinalAwaiter {
        bool await_ready() const noexcept { return false; }
        template <typename P>
        void await_suspend(std::coroutine_handle<P> handle) noexcept {
            handle.promise().continuation.resume();  // resume the caller's coroutine
            // return handle.promise().continuation; // symmetric
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

    struct Awaiter {
        std::coroutine_handle<Promise> handle;

        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> calling) noexcept {
            handle.promise().continuation = calling;  // store the caller's coroutine handle in the promise
            handle.resume();
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


    ~Task() {  // destroy the coroutine when the task is destroyed
        if (handle_) {
            BOOST_LOG_TRIVIAL(trace) << "Destroying coroutine";
            handle_.destroy();
        }
    }

    Task(const Task&) = delete;
    Task(Task&& other) noexcept : handle_(other.handle_) {
        other.handle_ = nullptr;
    }

   private:
    explicit Task(std::coroutine_handle<Promise> handle)
        : handle_(handle) {
    }
};

template <typename T>
class Awaitable {
   public:
    Awaitable(std::queue<T>& queue) : queue_(queue) {}

    bool await_ready() const noexcept {
        return !queue_.empty();
    }

    void await_suspend(std::coroutine_handle<> handle) {}

    T await_resume() {
        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }

   private:
    std::queue<T>& queue_;
};

struct ClientCoroutine {
    struct promise_type {
        ClientCoroutine get_return_object() { return ClientCoroutine{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };
    std::coroutine_handle<promise_type> handle;

    ClientCoroutine(const ClientCoroutine&) = delete;
    ClientCoroutine& operator=(const ClientCoroutine&) = delete;
    ClientCoroutine(ClientCoroutine&& other) noexcept : handle(other.handle) {
        other.handle = nullptr;
    }
    ClientCoroutine& operator=(ClientCoroutine&& other) noexcept {
        if (this != &other) {
            if (handle) handle.destroy();
            handle = other.handle;
            other.handle = nullptr;
        }
        return *this;
    }

    ClientCoroutine(std::coroutine_handle<promise_type> h) : handle(h) {}
    ~ClientCoroutine() {
        if (handle) {
            handle.destroy();
        }
    }
};

#endif