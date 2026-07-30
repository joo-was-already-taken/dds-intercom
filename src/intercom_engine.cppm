module;

#include <AudioFrame.hpp>

#include <miniaudio/miniaudio.h>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

export module intercom_engine;

import atomic_ring_buffer;
import dds_node;

namespace intercom {

template <typename T, size_t N>
using RingBuffer = intercom::AtomicRingBuffer<T, N>;

constexpr size_t SAMPLES_PER_FRAME = 320;
constexpr size_t RING_BUFFER_SIZE = 16384;
constexpr uint32_t SAMPLE_RATE = 16000;

export class IntercomEngine {
public:
  explicit IntercomEngine(std::string node_id) noexcept
      : m_node_id(std::move(node_id)) {}
  ~IntercomEngine() noexcept { stop(); }

  IntercomEngine(const IntercomEngine &) = delete;
  IntercomEngine &operator=(const IntercomEngine &) = delete;
  IntercomEngine(IntercomEngine &&) = delete;
  IntercomEngine &operator=(IntercomEngine &&) = delete;

  // NOLINTBEGIN(bugprone-easily-swappable-parameters)
  bool
  start(uint32_t domain_id, IntercomMode mode = IntercomMode::duplex) noexcept {
    // NOLINTEND(bugprone-easily-swappable-parameters)
    m_mode = mode;

    if (!m_dds_node.init(domain_id, mode, [this](const AudioFrame &frame) {
          if (frame.sender_id() == m_node_id) { return; }
          if (!frame.pcm_data().empty()) {
            m_playback_buffer.write(
                std::span<const int16_t>{
                    frame.pcm_data().data(), frame.pcm_data().size()
                }
            );
          }
        })) {
      return false;
    }

    ma_device_type device_type = ma_device_type_duplex;
    if (mode == IntercomMode::broadcast) {
      device_type = ma_device_type_capture;
    } else if (mode == IntercomMode::listen) {
      device_type = ma_device_type_playback;
    }

    ma_device_config config = ma_device_config_init(device_type);
    config.capture.format = ma_format_s16;
    config.capture.channels = 1;
    config.playback.format = ma_format_s16;
    config.playback.channels = 1;
    config.sampleRate = SAMPLE_RATE;
    config.periodSizeInFrames = SAMPLES_PER_FRAME;
    config.dataCallback = audio_callback;
    config.pUserData = this;

    if (ma_device_init(nullptr, &config, &m_device) != MA_SUCCESS) {
      return false;
    }

    if (ma_device_start(&m_device) != MA_SUCCESS) { return false; }

    m_running.store(true, std::memory_order_relaxed);
    if ((mode & IntercomMode::broadcast) != IntercomMode::none) {
      m_tx_thread = std::thread(&IntercomEngine::tx_worker, this);
    }

    return true;
  }

  void stop() noexcept {
    m_running.store(false);
    if (m_tx_thread.joinable()) { m_tx_thread.join(); }
    ma_device_uninit(&m_device);
  }

private:
  std::string m_node_id;
  ma_device m_device{};
  DDSNode m_dds_node{};

  RingBuffer<int16_t, RING_BUFFER_SIZE> m_capture_buffer{};
  RingBuffer<int16_t, RING_BUFFER_SIZE> m_playback_buffer{};

  std::atomic<bool> m_running{false};
  std::thread m_tx_thread;
  IntercomMode m_mode{IntercomMode::duplex};
  uint64_t m_sequence_number{0};

  // NOLINTBEGIN(bugprone-easily-swappable-parameters)
  static void audio_callback(
      ma_device *device, void *output, const void *input, ma_uint32 frame_count
  ) noexcept {
    // NOLINTEND(bugprone-easily-swappable-parameters)
    auto *self = static_cast<IntercomEngine *>(device->pUserData);
    if (self == nullptr) { return; }

    if (input != nullptr) {
      const auto *mic_samples = static_cast<const int16_t *>(input);
      self->m_capture_buffer.write(
          std::span<const int16_t>{mic_samples, frame_count}
      );
    }

    if (output != nullptr) {
      auto *speaker_samples = static_cast<int16_t *>(output);
      size_t read_count = self->m_playback_buffer.read(
          std::span<int16_t>{speaker_samples, frame_count}
      );

      // zero out remaining buffer space if network frame dropped
      if (read_count < frame_count) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::fill_n(speaker_samples + read_count, frame_count - read_count, 0);
      }
    }
  }

  void tx_worker() noexcept {
    std::vector<int16_t> frame_chunk(SAMPLES_PER_FRAME);

    while (m_running.load(std::memory_order::relaxed)) {
      if (m_capture_buffer.available_read() >= SAMPLES_PER_FRAME) {
        m_capture_buffer.read(
            std::span<int16_t>{frame_chunk.data(), SAMPLES_PER_FRAME}
        );

        AudioFrame frame;
        frame.sender_id(m_node_id);
        frame.sequence_number(++m_sequence_number);
        frame.sample_rate(SAMPLE_RATE);
        frame.channels(1);
        frame.pcm_data().assign(frame_chunk.cbegin(), frame_chunk.cend());

        m_dds_node.publish_frame(frame);
      } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    }
  }
};

} // namespace intercom
