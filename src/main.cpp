#include <CLI/CLI.hpp>

#include <atomic>
#include <csignal>
#include <print>
#include <string>

import dds_node;
import intercom_engine;

using namespace intercom;

namespace {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::atomic_flag g_exit_flag = ATOMIC_FLAG_INIT;

void signal_handler(int /*signal*/) {
  g_exit_flag.test_and_set();
  g_exit_flag.notify_one();
}

} // namespace

int main(int argc, char *argv[]) {
  CLI::App app{"DDS Intercom"};

  std::string node_name{};
  app.add_option("name", node_name, "Your peer name/callsign")
      ->required()
      ->type_name("NAME");

  uint32_t domain_id = 0;
  app.add_option("-d,--domain", domain_id, "DDS domain ID")->default_val(0);

  std::string role_str{};
  app.add_option("-r,--role", role_str, "Node role")
      ->check(CLI::IsMember({"duplex", "broadcast", "listen"}))
      ->default_val("duplex");

  CLI11_PARSE(app, argc, argv);

  IntercomMode mode = IntercomMode::duplex;
  mode = (role_str == "broadcast") ? IntercomMode::broadcast : mode;
  mode = (role_str == "listen") ? IntercomMode::listen : mode;

  IntercomEngine intercom(node_name);

  if (!intercom.start(domain_id, mode)) {
    std::println(stderr, "error: failed to start intercom");
    return 1;
  }

  (void)std::signal(SIGINT, signal_handler);
  (void)std::signal(SIGTERM, signal_handler);

  std::println("-> Intercom Active!");
  std::println("-> Press Ctrl+C to exit.");

  g_exit_flag.wait(false);

  std::println("\nShutting down...");

  return 0;
}
