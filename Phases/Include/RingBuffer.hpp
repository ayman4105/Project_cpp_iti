#pragma once

#include <vector>
#include <mutex>
#include <condition_variable>
#include <cstddef>
#include <optional>
#include "LogMessage.hpp"

template <typename T>

class RingBuffer
{

private:
    std::vector<std::optional<T>> buffer;

    size_t capacity;
    size_t read_index;
    size_t write_index;
    size_t count;

    mutable std::mutex mtx;
    std::condition_variable cv;

public:
    explicit RingBuffer(size_t capacity)
        : buffer(capacity),
          capacity(capacity),
          read_index(0),
          write_index(0),
          count(0)
    {
        buffer.resize(capacity);
    }

    bool tryPush(const T &item)
    {
        std::lock_guard<std::mutex> lock(mtx);

        if (count == capacity)
        {
            return false;
        }

        buffer[write_index] = item;
        write_index = (write_index + 1) % capacity;
        ++count;

        cv.notify_one();
        return true;
    }

    std::optional<T> trypop()
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (count == 0)
            return std::nullopt;

        T item = buffer[read_index].value(); // استخراج القيمة
        buffer[read_index].reset();          // فرغ المكان بعد القراءة
        read_index = (read_index + 1) % capacity;
        --count;
        return item;
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(mtx);
        return count == 0;
    }

    bool full() const
    {
        std::lock_guard<std::mutex> lock(mtx);
        return count == capacity;
    }

    size_t size() const
    {
        std::lock_guard<std::mutex> lock(mtx);
        return count;
    }

    size_t max_size() const
    {
        return capacity;
    }
};
