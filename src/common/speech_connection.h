#pragma once

#include <assert.h>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <list>
#include "speech_common.h"
#include "Hub.h"
#include "rlog.h"
#include "alt_chrono.h"
#ifdef ROKID_UPLOAD_TRACE
#include "trace-uploader.h"
#endif

namespace rokid {
namespace speech {

enum class ConnectStage {
  INIT = 0,
  // not connect, should connect when reconn time reach
  DISCONN,
  CONNECTING,
  AUTHORIZING,
  // connected and authorized
  READY,
  // not connect, should not try reconn until voice data need to send
  PAUSED,
  // not connect, should release resources and quit
  CLOSED
};

enum class ConnectionOpResult {
  SUCCESS = 0,
  INVALID_PB_OBJ,
  CONNECTION_NOT_AVAILABLE,
  NOT_READY,
  INVALID_PB_DATA,
  CONNECTION_BROKEN,
  TIMEOUT,
};

enum class BinRespType {
  DATA = 0,
  ERROR,
  CLOSED
};

typedef struct {
  BinRespType type;
  uint32_t length;
  char data[];
} SpeechBinaryResp;

#ifdef SPEECH_STATISTIC
typedef struct {
  int32_t id;
  std::chrono::system_clock::time_point req_tp;
  std::chrono::system_clock::time_point resp_tp;
} TraceInfo;
#endif

class SpeechConnection {
public:
  SpeechConnection();

  void initialize(int32_t ws_buf_size, const PrepareOptions& options, const char* svc);

  void release();

  // params: 'timeout' milliseconds
  template <typename PBT>
  ConnectionOpResult send(PBT& pbitem, uint32_t timeout = 0) {
    std::string buf;
    if (!pbitem.SerializeToString(&buf)) {
      KLOGW(CONN_TAG, "send: protobuf serialize failed");
      return ConnectionOpResult::INVALID_PB_OBJ;
    }
    KLOGV(CONN_TAG, "SpeechConnection.send: pb serialize result %lu bytes", buf.length());
    if (!ensure_connection_available(timeout)) {
      KLOGI(CONN_TAG, "send: connection not available");
      return ConnectionOpResult::CONNECTION_NOT_AVAILABLE;
    }
    ws_send(buf.data(), buf.length(), uWS::OpCode::BINARY);
    return ConnectionOpResult::SUCCESS;
  }

  template <typename PBT>
  ConnectionOpResult recv(PBT& res, uint32_t timeout) {
    SpeechBinaryResp* resp_data;
    std::unique_lock<std::mutex> locker(resp_mutex_);

    if (responses_.empty()) {
      if (timeout == 0)
        resp_cond_.wait(locker);
      else
        resp_cond_.wait_for(locker, std::chrono::milliseconds(timeout));
    }
    if (!responses_.empty()) {
      resp_data = responses_.front();
      assert(resp_data);
      responses_.pop_front();
      if (resp_data->type == BinRespType::DATA) {
        bool r = res.ParseFromArray(resp_data->data,
            resp_data->length);
        free(resp_data);
        if (!r) {
          KLOGW(CONN_TAG, "recv: protobuf parse failed");
          return ConnectionOpResult::INVALID_PB_DATA;
        }
        return ConnectionOpResult::SUCCESS;
      } else if (resp_data->type == BinRespType::ERROR) {
        KLOGI(CONN_TAG, "recv: failed, connection broken");
        free(resp_data);
        return ConnectionOpResult::CONNECTION_BROKEN;
      }
      KLOGD(CONN_TAG, "recv return, connection closed");
      return ConnectionOpResult::NOT_READY;
    }
    KLOGD(CONN_TAG, "recv return, timeout");
    return ConnectionOpResult::TIMEOUT;
  }

#ifdef SPEECH_STATISTIC
  void add_trace_info(const TraceInfo& info);
#endif

  // 立即尝试重连
  void reconn();

private:
  void run();

  void keepalive_run();

  void prepare_hub();

  void connect();

  bool auth();

  std::string get_server_uri();

  void onConnection(uWS::WebSocket<uWS::CLIENT> *ws);

  void onDisconnection(uWS::WebSocket<uWS::CLIENT> *ws, int code,
      char* message, size_t length);

  void onMessage(uWS::WebSocket<uWS::CLIENT> *ws, char* message,
      size_t length, uWS::OpCode opcode);

  void onError(void* userdata);

  void onPong(uWS::WebSocket<uWS::CLIENT> *ws, char* message,
      size_t length);

  static std::string timestamp();

  std::string generate_sign(const char* key, const char* devtype,
      const char* devid, const char* svc, const char* version,
      const char* ts, const char* secret);

  bool handle_auth_result(char* message, size_t length, uWS::OpCode opcode);

  bool ensure_connection_available(uint32_t timeout);

  void push_status_resp(BinRespType tp);

  void push_resp_data(char* msg, size_t length);

  void ws_send(const char* msg, size_t length, uWS::OpCode op);

#ifdef SPEECH_STATISTIC
  bool send_trace_info();

  void ping(std::string* payload = NULL);
#else
  void ping();
#endif

  void update_reconn_tp(uint32_t ms);

  void update_ping_tp();

  void update_recv_tp();

  void update_voice_tp();

private:
  /**
  std::recursive_mutex reconn_mutex_;
  std::condition_variable req_cond_;
  std::condition_variable_any reconn_cond_;
  */
  std::mutex req_mutex_;
  std::mutex resp_mutex_;
  std::condition_variable resp_cond_;
  std::list<SpeechBinaryResp*> responses_;
  std::mutex stage_mutex_;
  std::condition_variable stage_changed_;
  ConnectStage stage_;

  std::thread* work_thread_;
  std::thread* keepalive_thread_;
  uWS::Hub hub_;
  uWS::WebSocket<uWS::CLIENT>* ws_;
  PrepareOptions options_;
  std::string service_type_;
  SteadyClock::time_point reconn_timepoint_;
  SteadyClock::time_point lastest_ping_tp_;
  SteadyClock::time_point lastest_recv_tp_;
  SteadyClock::time_point lastest_voice_tp_;
#ifdef SPEECH_STATISTIC
  std::list<TraceInfo> _trace_infos;
#endif

  const char* CONN_TAG;
  char CONN_TAG_BUF[32];

#ifdef ROKID_UPLOAD_TRACE
  TraceUploader* trace_uploader_ = nullptr;
#endif
};

} // namespace speech
} // namespace rokid
