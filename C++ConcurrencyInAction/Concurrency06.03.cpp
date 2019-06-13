#include "Concurrency06.h" 
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
namespace {
    // ����std::shared_ptr<>ʵ�����̰߳�ȫ����
    template<typename T>
    class threadsafe_queue
    {
    private:
        mutable std::mutex mut;
        // queue�ڲ�ʹ��shared_ptr����
        std::queue<std::shared_ptr<T> > data_queue;
        std::condition_variable data_cond;
    public:
        threadsafe_queue()
        {}

        void wait_and_pop(T& value)
        {
            std::unique_lock<std::mutex> lk(mut);
            data_cond.wait(lk, [this] {return !data_queue.empty(); });
            value = std::move(*data_queue.front()); // ��
            data_queue.pop();
        }

        bool try_pop(T& value)
        {
            std::lock_guard<std::mutex> lk(mut);
            if (data_queue.empty())
                return false;
            value = std::move(*data_queue.front()); // ��
            data_queue.pop();
        }

        std::shared_ptr<T> wait_and_pop()
        {
            std::unique_lock<std::mutex> lk(mut);
            data_cond.wait(lk, [this] {return !data_queue.empty(); });
            std::shared_ptr<T> res = data_queue.front(); // ��
            data_queue.pop();
            return res;
        }

        std::shared_ptr<T> try_pop()
        {
            std::lock_guard<std::mutex> lk(mut);
            if (data_queue.empty())
                return std::shared_ptr<T>();
            std::shared_ptr<T> res = data_queue.front(); // ��
            data_queue.pop();
            return res;
        }

        bool empty() const
        {
            std::lock_guard<std::mutex> lk(mut);
            return data_queue.empty();
        }

        void push(T new_value)
        {
            // ��std::shared_ptr<>ʵ���ĳ�ʼ������push������
            std::shared_ptr<T> data(
                std::make_shared<T>(std::move(new_value))); // ��
            std::lock_guard<std::mutex> lk(mut);
            data_queue.push(data);
            data_cond.notify_one();
        }

        size_t size() const
        {
            std::lock_guard<std::mutex> lk(mut);
            return data_queue.size();
        }
    };
}

#include <iostream>
#include <atomic>
#include <chrono>
#include <thread>
void Concurrency06_03() {
    threadsafe_queue<int> rq;
    std::atomic<bool> stop = false;
    std::cout << "06.03 ����std::shared_ptr<>ʵ�����̰߳�ȫ����" << std::endl;
    std::thread t1([&]() {
        int i = 0;
        while (!stop) {
            rq.push(i++);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        };
        });
    std::thread t2([&]() {
        std::shared_ptr<int> value;
        int i = 0;
        while (!stop) {
            value = rq.try_pop();
            if (!value) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            std::cout << i << ", " << *value;
            i++;
        }
        std::cout << std::endl;
        });
    std::thread t3([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        stop.store(true);
        });
    t1.join();
    t2.join();
    t3.join();
    std::cout << "rq.size() = " << rq.size() << std::endl;
}