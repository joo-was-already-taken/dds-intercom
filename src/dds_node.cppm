module;

#include <AudioFrame.hpp>
#include <AudioFramePubSubTypes.hpp>

#include <fastdds/dds/core/policy/QosPolicies.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/qos/DomainParticipantQos.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/qos/SubscriberQos.hpp>
#include <fastdds/dds/topic/qos/TopicQos.hpp>

#include <cstdint>
#include <functional>
#include <string>
#include <utility>

export module dds_node;

namespace intercom {

export enum class IntercomMode : uint8_t {
  none = 0b00,
  listen = 0b01,
  broadcast = 0b10,
  duplex = 0b11
};

export constexpr IntercomMode
operator&(IntercomMode lhs, IntercomMode rhs) noexcept {
  return static_cast<IntercomMode>(
      static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs)
  );
}

export constexpr IntercomMode
operator|(IntercomMode lhs, IntercomMode rhs) noexcept {
  return static_cast<IntercomMode>(
      static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs)
  );
}

namespace dds = eprosima::fastdds::dds;
using FrameCallback = std::function<void(const AudioFrame &)>;

export class DDSNode : public dds::DataReaderListener {
public:
  DDSNode() noexcept = default;
  ~DDSNode() noexcept override {
    if (m_participant != nullptr) {
      m_participant->delete_contained_entities();
      dds::DomainParticipantFactory::get_instance()->delete_participant(
          m_participant
      );
    }
  }

  DDSNode(const DDSNode &) = delete;
  DDSNode &operator=(const DDSNode &) = delete;
  DDSNode(DDSNode &&) = delete;
  DDSNode &operator=(DDSNode &&) = delete;

  bool init(
      uint32_t domain_id, IntercomMode mode, FrameCallback on_frame_received
  ) noexcept {
    m_frame_callback = std::move(on_frame_received);

    if (!init_participant(domain_id)) { return false; }

    if ((mode & IntercomMode::broadcast) != IntercomMode::none &&
        !init_publisher()) {
      return false;
    }

    if ((mode & IntercomMode::listen) != IntercomMode::none &&
        !init_subscriber()) {
      return false;
    }

    return true;
  }

  bool publish_frame(AudioFrame &frame) noexcept {
    if (m_writer == nullptr) { return false; }
    return m_writer->write(&frame) == dds::RETCODE_OK;
  }

  void on_data_available(dds::DataReader *reader) noexcept override {
    AudioFrame frame;
    dds::SampleInfo info;

    while (reader->take_next_sample(&frame, &info) == dds::RETCODE_OK) {
      if (info.valid_data && m_frame_callback != nullptr) {
        m_frame_callback(frame);
      }
    }
  }

private:
  bool init_participant(uint32_t domain_id) noexcept {
    dds::DomainParticipantQos pqos = dds::PARTICIPANT_QOS_DEFAULT;
    pqos.name("Intercom_Peer");

    m_participant =
        dds::DomainParticipantFactory::get_instance()->create_participant(
            domain_id, pqos
        );
    if (m_participant == nullptr) { return false; }

    m_type.register_type(m_participant);

    m_topic = m_participant->create_topic(
        "AudioIntercomChannel", m_type.get_type_name(), dds::TOPIC_QOS_DEFAULT
    );
    return m_topic != nullptr;
  }

  bool init_publisher() noexcept {
    m_publisher = m_participant->create_publisher(dds::PUBLISHER_QOS_DEFAULT);
    if (m_publisher == nullptr) { return false; }

    dds::DataWriterQos wqos = dds::DATAWRITER_QOS_DEFAULT;
    fill_qos_config(wqos);

    m_writer = m_publisher->create_datawriter(m_topic, wqos);
    return m_writer != nullptr;
  }

  bool init_subscriber() noexcept {
    m_subscriber =
        m_participant->create_subscriber(dds::SUBSCRIBER_QOS_DEFAULT);
    if (m_subscriber == nullptr) { return false; }

    dds::DataReaderQos rqos = dds::DATAREADER_QOS_DEFAULT;
    fill_qos_config(rqos);

    m_reader = m_subscriber->create_datareader(m_topic, rqos, this);
    return m_reader != nullptr;
  }

  template <typename Qos>
  static void fill_qos_config(Qos &qos) noexcept {
    qos.reliability().kind = dds::BEST_EFFORT_RELIABILITY_QOS;
    qos.durability().kind = dds::VOLATILE_DURABILITY_QOS;
    qos.history().kind = dds::KEEP_LAST_HISTORY_QOS;
    qos.history().depth = 1;
  }

  dds::DomainParticipant *m_participant{nullptr};
  dds::Publisher *m_publisher{nullptr};
  dds::DataWriter *m_writer{nullptr};
  dds::Subscriber *m_subscriber{nullptr};
  dds::DataReader *m_reader{nullptr};
  dds::Topic *m_topic{nullptr};
  dds::TypeSupport m_type{new AudioFramePubSubType()};
  FrameCallback m_frame_callback;
};

} // namespace intercom
