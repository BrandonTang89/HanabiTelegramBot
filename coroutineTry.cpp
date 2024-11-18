#include <condition_variable>
#include <coroutine>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>

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

ClientCoroutine printValue(std::queue<std::string>& queue) {
    while (true) {
        std::string value = co_await Awaitable<std::string>(queue);
        std::cout << "Received value: " << value << std::endl;
    }
}

int main() {
    std::queue<std::string> queue;

    // Start the coroutine
    auto coroutine = printValue(queue);

    // Perform computations and push values into the queue
    queue.push("Hello");
    coroutine.handle.resume();
    queue.push("World");
    coroutine.handle.resume();
    queue.push("!");
    coroutine.handle.resume();



    return 0;
}