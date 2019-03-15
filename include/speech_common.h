#pragma once

#include <stdint.h>
#include <memory>
#include <string>

namespace rokid {
namespace speech {

class PrepareOptions {
public:
  PrepareOptions();

  PrepareOptions& operator = (const PrepareOptions& options);

  std::string host;
  uint32_t port;
  std::string branch;
  std::string key;
  std::string device_type_id;
  std::string secret;
  std::string device_id;

  // milliseconds
  uint32_t reconn_interval;
  uint32_t ping_interval;
  uint32_t no_resp_timeout;

  // seconds
  uint32_t conn_duration;
};

enum class Lang {
  ZH,
  EN
};

enum class Codec {
  PCM,
  OPU,
  OPU2,
  MP3
};

enum class VadMode {
  LOCAL,
  CLOUD
};

// enable/disable speech log write to file
// params:
//   'enable' true, enable log write to file
//            false, disable log write to file
//   'log_path'  directory that log files generated
void enable_file_log(bool enable, const char* log_path);

} // namespace speech
} // namespace rokid
