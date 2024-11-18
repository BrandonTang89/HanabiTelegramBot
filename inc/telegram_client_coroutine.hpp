#ifndef TELEGRAM_CLIENT_COROUTINE_HPP
#define TELEGRAM_CLIENT_COROUTINE_HPP
#include <coroutine>
#include <iostream>
#include <queue>
#include <string>
#include <tgbot/tgbot.h>

template <typename T>
class Awaitable {
   public:
    Awaitable(std::queue<T>& queue) : queue_(queue) {}

    bool await_ready() const noexcept {
        if (queue_.empty()) {
            return false;
        }
        return true;
    }

    void await_suspend(std::coroutine_handle<> handle) {
        continuation_ = handle;
    }
    T await_resume() {
        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }

    void resume() {
        if (continuation_) {
            continuation_.resume();
        }
    }

   private:
    std::queue<T>& queue_;
    std::coroutine_handle<> continuation_;
};

struct ClientCoroutine {
    struct promise_type {
        ClientCoroutine get_return_object() { return ClientCoroutine{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };

    std::coroutine_handle<promise_type> handle;

    ClientCoroutine(std::coroutine_handle<promise_type> h) : handle(h) {}
    ~ClientCoroutine() {
        if (handle) handle.destroy();
    }
};

#endif