#include "SharedQueue.h"

void ThreadPool::RunThread(ThreadPool* const tp) {
    if (!tp) {
        throw std::runtime_error("No such threadpool exists!!");
    }

    ++tp->threadsInWait_; // thread enters wait state

    while (!tp->terminate_) {
        ThreadTask task;

        if (tp->queue_.PopWaitFor(task, std::chrono::milliseconds(100))) {
            --tp->threadsInWait_; // thread leaves wait state

            try {
                task();
            } catch (const std::exception& e) {
                std::cerr << "Exception in thread task: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Unknown exception in thread task." << std::endl;
            }

            ++tp->threadsInWait_; // thread returns to wait state
        }
    }

    --tp->threadsInWait_; // thread exits
}


