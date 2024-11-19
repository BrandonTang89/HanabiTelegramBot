#ifndef TELEGRAM_CLIENT_COROUTINE_HPP
#define TELEGRAM_CLIENT_COROUTINE_HPP
#include <coroutine>
#include <queue>

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