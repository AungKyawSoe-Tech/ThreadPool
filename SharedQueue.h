#include <functional>
#include <queue>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <future>
#include <chrono>
#include <string>
#include <iostream>

template <typename T>
class SharedQueue {
public:
    SharedQueue() {}

    SharedQueue(const SharedQueue&) = delete;
    SharedQueue& operator=(const SharedQueue&) = delete;
    SharedQueue(const SharedQueue&&) = delete;
    SharedQueue& operator=(const SharedQueue&&) = delete;

    // Push an item into the queue
    void Push(T item) {
        Emplace(std::move(item));
    }

    // Emplace an item into the queue
    template <typename... Args>
    void Emplace(Args&&... args) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.emplace(std::forward<Args>(args)...);
        condition_.notify_one();
    }

    // Pop an item from the queue (blocking)
    void Pop(T& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return !queue_.empty(); });
        item = std::move(queue_.front());
        queue_.pop();
    }

    // Pop an item from the queue with timeout
    bool PopWaitFor(T& item, const std::chrono::milliseconds wait) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!condition_.wait_for(lock, wait, [this] { return !queue_.empty(); })) {
            return false; // timeout
        }
        item = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    // Clear the queue
    void Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!queue_.empty()) {
            queue_.pop();
        }
    }

    // Check if the queue is empty
    bool IsEmpty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    // Get the size of the queue
    unsigned int Size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return static_cast<unsigned int>(queue_.size());
    }

private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;
};

class ThreadPool {
public:
    using ThreadTask = std::function<void()>;

    ThreadPool()
        : threadsInWait_(0), terminate_(false) {}

    explicit ThreadPool(unsigned int numberOfThreads)
        : ThreadPool() {
        Restart(numberOfThreads);
    }

    ~ThreadPool() {
        terminate_ = true;
        queue_.Clear();
        BusyYield();
        JoinAll();
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(const ThreadPool&&) = delete;
    ThreadPool& operator=(const ThreadPool&&) = delete;

    // Join all threads
    void JoinAll() {
        for (auto& thread : threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

    // Get number of tasks in queue
    unsigned int GetNumerOfTasksInQueue() const {
        return queue_.Size();
    }

    // Check if queue is empty
    bool IsQueueEmpty() const {
        return queue_.IsEmpty();
    }

    // Yield until queue is empty
    bool BusyYield() const {
        const bool mustWait = (GetNumerOfTasksInQueue() > 0);
        while (GetNumerOfTasksInQueue() > 0) {
            std::this_thread::yield();
        }
        return mustWait;
    }

    // Terminate the thread pool
    void Terminate() {
        queue_.Clear();
        terminate_ = true;
        JoinAll();
    }

    // Queue a job and get a future
    template<typename Func, typename... Args>
    auto QueueJob(Func&& func, Args&&... args)
        -> std::future<std::invoke_result_t<Func, Args...>>
    {
        using PackagedTask = std::packaged_task<std::invoke_result_t<Func, Args...>()>;

        std::cout << "QueueJob()-->" << std::endl;

        auto task = std::make_shared<PackagedTask>(
            std::bind(std::forward<Func>(func), std::forward<Args>(args)...)
        );

        auto ret = task->get_future();

        queue_.Emplace([task]() { (*task)(); });

        return ret;
    }

    // Restart (resize) the thread pool
    void Restart(unsigned int numOfThreads) {
        if (numOfThreads > maxAllowableThreads) {
            throw std::runtime_error("Over the limit of the pool!");
        }

        Terminate();
        BusyYield();
        JoinAll();
        threads_.clear();
        threads_.shrink_to_fit();
        threads_.reserve(numOfThreads);

        LaunchThreads(numOfThreads);
    }

    // Get current number of threads
    unsigned int Size() const {
        return static_cast<unsigned int>(threads_.size());
    }

    // Get thread pool capacity
    unsigned int Capacity() const {
        return static_cast<unsigned int>(threads_.capacity());
    }

private:
    static void RunThread(ThreadPool* const tp);

    void LaunchThreads(unsigned int numOfThreads) {
        std::cout << numOfThreads << " threads will be launched!" << std::endl;

        if (numOfThreads > 0) {
            terminate_ = false;

            std::generate_n(
                std::inserter(threads_, threads_.end()),
                numOfThreads,
                [this]() {
                    ++threadsInWait_; // thread enters pool
                    return std::thread(RunThread, this);
                }
            );
        }

        std::cout << "\ncurrent size = " << Size()
                  << " , capacity = " << Capacity() << std::endl;
    }

    std::vector<std::thread> threads_;
    std::atomic<unsigned int> threadsInWait_;
    std::atomic<bool> terminate_;
    SharedQueue<ThreadTask> queue_;

    static constexpr int maxAllowableThreads = 20; // this is arbitrary
};

