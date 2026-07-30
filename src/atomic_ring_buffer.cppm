module;

#include <algorithm>
#include <array>
#include <atomic>
#include <span>

export module atomic_ring_buffer;

namespace intercom {

export template <typename T, size_t N>
class AtomicRingBuffer {
public:
  AtomicRingBuffer() noexcept = default;

  size_t write(std::span<const T> data) noexcept {
    size_t head = m_head.load(std::memory_order::relaxed);
    size_t tail = m_tail.load(std::memory_order::acquire);

    size_t available = N - (head - tail);
    size_t to_write = std::min(data.size(), available);

    for (size_t i = 0; i < to_write; ++i) {
      m_buffer[(head + i) % N] = data[i];
    }

    m_head.store(head + to_write, std::memory_order::release);
    return to_write;
  }

  size_t read(std::span<T> destination) noexcept {
    size_t head = m_head.load(std::memory_order::acquire);
    size_t tail = m_tail.load(std::memory_order::relaxed);

    if (head == tail) { return 0; }

    size_t available = head - tail;
    size_t to_read = std::min(destination.size(), available);

    for (size_t i = 0; i < to_read; ++i) {
      destination[i] = m_buffer[(tail + i) % N];
    }

    m_tail.store(tail + to_read, std::memory_order::release);
    return to_read;
  }

  [[nodiscard]] size_t available_read() const noexcept {
    return m_head.load(std::memory_order::acquire) -
        m_tail.load(std::memory_order::relaxed);
  }

private:
  std::array<T, N> m_buffer;
  std::atomic<size_t> m_head{0};
  std::atomic<size_t> m_tail{0};
};

} // namespace intercom
