//
// Created by corey on 1/5/22.
//

#ifndef VP8_FULL_DUPLEX_CAPN_THREADSAFEDEQUE_H
#define VP8_FULL_DUPLEX_CAPN_THREADSAFEDEQUE_H

//#pragma once
#include <deque>
#include <mutex>
#include <condition_variable>

template<class T>
class ThreadSafeDeque {
public:
    void pop_front_waiting(T &t) {
        // unique_lock can be unlocked, lock_guard can not
        std::unique_lock<std::mutex> lock{ mutex }; // locks
        while(deque.empty()) {
            condition.wait(lock); // unlocks, sleeps and relocks when woken up
        }
        t = deque.front();
        deque.pop_front();
    } // unlocks as goes out of scope

    T front() {
        return deque.front();
    }

    bool empty() {
        if (deque.empty()) {
            return true;
        } else {
            return false;
        }
    }

    void push_back(const T &t)
    {
        {
            std::lock_guard<std::mutex> lock{ mutex };
            deque.push_back(t);
        }
        condition.notify_one(); // wakes up pop_front_waiting
    }

private:
    std::deque<T>               deque;
    std::mutex                  mutex;
    std::condition_variable condition;
};

#endif //VP8_FULL_DUPLEX_CAPN_THREADSAFEDEQUE_H
