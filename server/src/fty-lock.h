#pragma once

#include <thread>
#include <mutex>
#include <fty_log.h>

namespace fty
{
  class Lock {
    public:
      Lock(std::mutex & m):m_mutex(m) {
        log_debug("Try to get the mutex...");
        m_mutex.lock();
        log_debug("Try to get the mutex... OK.");
      }
      ~Lock() {
        m_mutex.unlock();
        log_debug("Release the mutex... OK.");
      }
    private:
      std::mutex & m_mutex;
  };

}
