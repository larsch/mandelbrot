#ifndef _semaphore_hpp
#define _semaphore_hpp

#include <mutex>
#include <condition_variable>
#include <iostream>

class semaphore {
 public:
  semaphore() : count(0) {}
  void acquire() {
    std::unique_lock<std::mutex> lock(mtx);
    while (count <= 0) {
      cv.wait(lock);
    }
    count--;
  }
  void release(int n) {
    count += n;
    while (n--) {
      cv.notify_one();
    }
  }
  bool try_acquire() {
    std::unique_lock<std::mutex> lock(mtx);
    if (count > 0) {
      count--;
      return true;
    } else {
      return false;
    }
  }

 private:
  std::condition_variable cv;
  std::mutex mtx;
  std::atomic_int count;
};

#endif  // _semaphore_hpp
