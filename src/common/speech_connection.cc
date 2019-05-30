#include <inttypes.h>
#include <sys/time.h>
#include "openssl/md5.h"
#include "speech_connection.h"
#include "nanopb_encoder.h"
#include "nanopb_decoder.h"
#if defined(__GNU_LIBRARY__) || defined(__GLIBC__)
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#endif

#ifdef SPEECH_STATISTIC
#define MAX_PENDING_TRACE_INFOS 128
#define SEND_TRACE_INFO_INTERVAL 1000
#endif

static const char* api_version_ = "2";

using std::mutex;
using std::recursive_mutex;
using std::unique_lock;
using std::lock_guard;
using std::defer_lock;
using std::string;
using std::thread;
using std::shared_ptr;
using std::make_shared;
using std::cv_status;
using std::chrono::system_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::seconds;
using uWS::Hub;
using uWS::WebSocket;
using uWS::OpCode;

namespace rokid {
namespace speech {

const char *stage_to_string(ConnectStage stage) {
  static const char *stage_strings[] = {
    "INIT",
    "DISCONNECTED",
    "CONNECTING",
    "AUTHORIZING",
    "READY",
    "PAUSED",
    "CLOSED"
  };
  return stage_strings[static_cast<int>(stage)];
}

SpeechConnection::SpeechConnection() : work_thread_(NULL),
    keepalive_thread_(NULL), ws_(NULL), stage_(ConnectStage::INIT),
    CONN_TAG("speech.Connection") {
  prepare_hub();
}

void SpeechConnection::initialize(int32_t ws_buf_size,
    const PrepareOptions& options, const char* svc) {
  snprintf(CONN_TAG_BUF, sizeof(CONN_TAG_BUF), "rokid.Connection.%s", svc);
  CONN_TAG = CONN_TAG_BUF;
  options_ = options;
  KLOGD(CONN_TAG, "reconn interval = %u, ping interval = %u, no resp timeout = %u",
      options_.reconn_interval, options_.ping_interval, options_.no_resp_timeout);
  service_type_ = svc;
  stage_ = ConnectStage::PAUSED;
#ifdef ROKID_UPLOAD_TRACE
  trace_uploader_ = new TraceUploader(options.device_id, options.device_type_id);
#endif
  work_thread_ = new thread([this] { this->run(); });
  keepalive_thread_ = new thread([this] { this->keepalive_run(); });
}

void SpeechConnection::release() {
  if (work_thread_ == NULL)
    return;

  KLOGD(CONN_TAG, "release, notify work thread");
  stage_mutex_.lock();
  stage_ = ConnectStage::CLOSED;
  stage_changed_.notify_all();
  hub_.getDefaultGroup<uWS::CLIENT>().close();
  stage_mutex_.unlock();

  KLOGD(CONN_TAG, "join work thread");
  work_thread_->join();
  delete work_thread_;
  work_thread_ = NULL;
  keepalive_thread_->join();
  delete keepalive_thread_;
  keepalive_thread_ = NULL;
  KLOGD(CONN_TAG, "work thread exited");
  hub_.getDefaultGroup<uWS::CLIENT>().clearTimer();

  // awake all threads of invoking SpeechConnection::recv
  resp_mutex_.lock();
  responses_.clear();
  resp_cond_.notify_all();
  resp_mutex_.unlock();
  push_status_resp(BinRespType::CLOSED);
#ifdef ROKID_UPLOAD_TRACE
  if (trace_uploader_) {
    delete trace_uploader_;
    trace_uploader_ = nullptr;
  }
#endif
}

#ifdef SPEECH_STATISTIC
void SpeechConnection::add_trace_info(const TraceInfo& info) {
  req_mutex_.lock();
  _trace_infos.push_back(info);
  if (_trace_infos.size() > MAX_PENDING_TRACE_INFOS)
    _trace_infos.pop_front();
  req_mutex_.unlock();

  // notify keepalive thread?
  // stage_changed.notify_all();
}

void SpeechConnection::ping(string* payload) {
  const char* data = NULL;
  size_t len = 0;
  if (payload) {
    data = payload->data();
    len = payload->length();
  }
  KLOGI(CONN_TAG, "send ping frame, payload %lu bytes", len);
  ws_send(data, len, OpCode::PING);
  lastest_ping_tp_ = SteadyClock::now();
}
#else
void SpeechConnection::ping() {
  KLOGI(CONN_TAG, "send ping frame");
  ws_send(NULL, 0, OpCode::PING);
  lastest_ping_tp_ = SteadyClock::now();
}
#endif

void SpeechConnection::reconn() {
  lock_guard<mutex> locker(stage_mutex_);
  update_reconn_tp(0);
  if (ws_)
    ws_->close();
}

void SpeechConnection::run() {
  KLOGV(CONN_TAG, "work thread runing");

  // workaround for Hub loop nerver return when server no response
  // set a timer, epoll_wait will awake periodic
  hub_.getDefaultGroup<uWS::CLIENT>().setTimer(4000);

  unique_lock<mutex> locker(stage_mutex_);
  SteadyClock::time_point now;

  while (true) {
    if (stage_ == ConnectStage::CLOSED)
      break;
    switch (stage_) {
      case ConnectStage::DISCONN:
        now = SteadyClock::now();
        if (now >= reconn_timepoint_) {
          KLOGD(CONN_TAG, "connecting");
          stage_ = ConnectStage::CONNECTING;
          connect();
        } else {
          KLOGD(CONN_TAG, "wait %d ms for reconnect",
                duration_cast<milliseconds>(reconn_timepoint_ - now).count());
          stage_changed_.wait_for(locker, reconn_timepoint_ - now);
        }
        break;
      case ConnectStage::PAUSED:
        KLOGD(CONN_TAG, "wait because stage is %s", stage_to_string(stage_));
        stage_changed_.wait(locker);
        KLOGD(CONN_TAG, "stage change to %s", stage_to_string(stage_));
        break;
      default:
        // wait for:
        //   connection lose
        //   socket error
        //   SpeechConnection release
        locker.unlock();
        KLOGD(CONN_TAG, "uWS run, stage %s", stage_to_string(stage_));
        hub_.run();
        KLOGD(CONN_TAG, "uWS stop run, stage %s", stage_to_string(stage_));
        locker.lock();
        break;
    }
  }

  KLOGV(CONN_TAG, "work thread quit");
}

void SpeechConnection::update_reconn_tp(uint32_t ms) {
  reconn_timepoint_ = SteadyClock::now() + milliseconds(ms);
}

void SpeechConnection::update_ping_tp() {
  lastest_ping_tp_ = SteadyClock::now();
  KLOGI(CONN_TAG, "lastest ping tp %lld",
      duration_cast<milliseconds>(lastest_ping_tp_.time_since_epoch()).count());
}

void SpeechConnection::update_recv_tp() {
  lastest_recv_tp_ = SteadyClock::now();
}

void SpeechConnection::update_voice_tp() {
  lastest_voice_tp_ = SteadyClock::now();
}

void SpeechConnection::keepalive_run() {
  SteadyClock::time_point now;
  unique_lock<mutex> locker(stage_mutex_);
  milliseconds timeout;
  milliseconds ping_interval = milliseconds(options_.ping_interval);
  milliseconds no_resp_timeout = milliseconds(options_.no_resp_timeout);
  seconds conn_duration = seconds(options_.conn_duration);
#ifdef SPEECH_STATISTIC
  bool has_trace_info;
#endif

  KLOGV(CONN_TAG, "keepalive thread run");
  KLOGI(CONN_TAG, "connection duration is %u", options_.conn_duration);
  while (true) {
    if (stage_ == ConnectStage::CLOSED)
      break;
    if (stage_ == ConnectStage::READY) {
      now = SteadyClock::now();
      KLOGI(CONN_TAG, "now tp %lld",
        duration_cast<milliseconds>(now.time_since_epoch()).count());
#ifdef SPEECH_STATISTIC
      has_trace_info = send_trace_info();
#endif
      KLOGI(CONN_TAG, "check ping interval: %lld/%lld",
          duration_cast<milliseconds>(now - lastest_ping_tp_).count(),
          ping_interval.count());
      if (now - lastest_ping_tp_ >= ping_interval) {
        KLOGD(CONN_TAG, "ping");
        ping();
        update_ping_tp();
      }
      if (now - lastest_recv_tp_ >= no_resp_timeout) {
        KLOGW(CONN_TAG, "server may no response, try reconnect");
#ifdef ROKID_UPLOAD_TRACE
        shared_ptr<TraceEvent> ev = make_shared<TraceEvent>();
        ev->type = TRACE_EVENT_TYPE_SYS;
        ev->id = "system.speech.timeout";
        ev->name = "服务器超时未响应";
        ev->add_key_value("service", service_type_);
        trace_uploader_->put(ev);
#endif
        update_reconn_tp(0);
        if (ws_)
          ws_->close();
        continue;
      }
      if (now - lastest_voice_tp_ >= conn_duration) {
        KLOGI(CONN_TAG, "no voice data long time, close connection");
        stage_ = ConnectStage::PAUSED;
        stage_changed_.notify_all();
        hub_.getDefaultGroup<uWS::CLIENT>().close();
        continue;
      }
      auto d1 = ping_interval - duration_cast<milliseconds>(now - lastest_ping_tp_);
      auto d2 = no_resp_timeout - duration_cast<milliseconds>(now - lastest_recv_tp_);
      timeout = duration_cast<milliseconds>(d1 < d2 ? d1 : d2);
      if (timeout.count() < 0)
        timeout = milliseconds(0);
#ifdef SPEECH_STATISTIC
      if (has_trace_info && timeout.count() > SEND_TRACE_INFO_INTERVAL)
        timeout = milliseconds(SEND_TRACE_INFO_INTERVAL);
#endif
      stage_changed_.wait_for(locker, timeout);
    } else {
      KLOGI(CONN_TAG, "wait because stage is %s", stage_to_string(stage_));
      stage_changed_.wait(locker);
      KLOGI(CONN_TAG, "stage change to %s", stage_to_string(stage_));
    }
  }
  KLOGI(CONN_TAG, "keepalive thread quit");
}

#ifdef SPEECH_STATISTIC
bool SpeechConnection::send_trace_info() {
  TraceInfo info;
  req_mutex_.lock();
  if (_trace_infos.size() > 0) {
    info = _trace_infos.front();
    _trace_infos.pop_front();
    req_mutex_.unlock();

    PingPayload pp;
    string buf;
    system_clock::time_point tp = system_clock::now();
    int64_t ms = duration_cast<milliseconds>(tp.time_since_epoch()).count();
    pp.set_req_id(info.id);
    pp.set_now_tp(ms);
    ms = duration_cast<milliseconds>(info.req_tp.time_since_epoch()).count();
    pp.set_req_tp(ms);
    ms = duration_cast<milliseconds>(info.resp_tp.time_since_epoch()).count();
    pp.set_resp_tp(ms);
    if (!pp.SerializeToString(&buf)) {
      KLOGE(CONN_TAG, "PingPayload serialize failed.");
      return true;
    }
    KLOGV(CONN_TAG, "send trace info, id = %d", info.id);
    ping(&buf);
    return true;
  } else
    req_mutex_.unlock();
  return false;
}
#endif

void SpeechConnection::prepare_hub() {
  uWS::Group<uWS::CLIENT>* group = static_cast<uWS::Group<uWS::CLIENT>*>(&hub_);
  group->onConnection([=](WebSocket<uWS::CLIENT> *ws, uWS::HttpRequest req) {
      onConnection(ws);
    });
  group->onDisconnection([=](WebSocket<uWS::CLIENT> *ws, int code,
        char* message, size_t length) {
      onDisconnection(ws, code, message, length);
    });
  group->onMessage([=](WebSocket<uWS::CLIENT> *ws, char* message,
        size_t length, OpCode opcode) {
      onMessage(ws, message, length, opcode);
    });
  group->onError([=](void* userdata) { onError(userdata); });
  group->onPong([=](WebSocket<uWS::CLIENT> *ws, char* message, size_t length) {
      onPong(ws, message, length);
    });
}

void SpeechConnection::connect() {
  string uri = get_server_uri();
  KLOGI(CONN_TAG, "connect to server %s", uri.c_str());
// glibc bug: gethostbyname not reread resolv.conf if the file changed
// glibc 2.26 fix the bug
#if defined(__GLIBC__)
  if (__GLIBC__ <= 2 && __GLIBC_MINOR__ <= 26)
    res_init();
// old version glibc not defined __GLIBC__, but defined __GNU_LIBRARY__ and __GNU_LIBRARY_MINOR__
#elif defined(__GNU_LIBRARY__)
  res_init();
#endif
#ifdef ROKID_UPLOAD_TRACE
  shared_ptr<TraceEvent> ev = make_shared<TraceEvent>();
  ev->type = TRACE_EVENT_TYPE_SYS;
  ev->id = "system.speech.connect";
  ev->name = "开始连接服务器";
  ev->add_key_value("service", service_type_);
  trace_uploader_->put(ev);
#endif
  hub_.connect(uri);
}

string SpeechConnection::get_server_uri() {
  char tmp[64];
  snprintf(tmp, sizeof(tmp), "wss://%s:%u%s",
      options_.host.c_str(), options_.port, options_.branch.c_str());
  return string(tmp);
}

void SpeechConnection::onConnection(WebSocket<uWS::CLIENT> *ws) {
  KLOGI(CONN_TAG, "uws connected, %p", ws);
  lock_guard<mutex> locker(stage_mutex_);
  if (stage_ == ConnectStage::CLOSED || stage_ == ConnectStage::PAUSED)
    return;
  KLOGD(CONN_TAG, "authorizing");
  stage_ = ConnectStage::AUTHORIZING;
#ifdef ROKID_UPLOAD_TRACE
  shared_ptr<TraceEvent> ev = make_shared<TraceEvent>();
  ev->type = TRACE_EVENT_TYPE_SYS;
  ev->id = "system.speech.before_auth";
  ev->name = "准备发送认证包";
  ev->add_key_value("service", service_type_);
  trace_uploader_->put(ev);
#endif
  ws_ = ws;
  auth();
}

void SpeechConnection::onDisconnection(uWS::WebSocket<uWS::CLIENT> *ws,
    int code, char* message, size_t length) {
  KLOGI(CONN_TAG, "uws disconnected, code %d, msg length %d, stage %d",
      code, length, static_cast<int>(stage_));
  ws_ = NULL;
  if (stage_ == ConnectStage::PAUSED || stage_ == ConnectStage::CLOSED)
    return;
  push_status_resp(BinRespType::ERROR);
  if (stage_ == ConnectStage::READY)
    update_reconn_tp(0);
  else
    update_reconn_tp(options_.reconn_interval);
  stage_ = ConnectStage::DISCONN;
  stage_changed_.notify_all();
}

void SpeechConnection::onMessage(uWS::WebSocket<uWS::CLIENT> *ws,
    char* message, size_t length, uWS::OpCode opcode) {
  KLOGD(CONN_TAG, "uws recv message, length %d, opcode 0x%x", length, opcode);
  switch (stage_) {
    case ConnectStage::AUTHORIZING:
      stage_mutex_.lock();
      if (handle_auth_result(message, length, opcode)) {
        update_recv_tp();
        update_voice_tp();
        stage_ = ConnectStage::READY;
        stage_changed_.notify_all();
      } else {
        update_reconn_tp(options_.reconn_interval);
        if (ws_)
          ws_->close();
      }
      stage_mutex_.unlock();
      break;
    case ConnectStage::READY:
      update_recv_tp();
      push_resp_data(message, length);
      break;
    default:
      KLOGW(CONN_TAG, "recv %d bytes, opcode %d, but connect stage is %s",
          length, opcode, stage_to_string(stage_));
  }
}

void SpeechConnection::onError(void* userdata) {
  KLOGI(CONN_TAG, "uws error: userdata %p, stage %d", userdata, static_cast<int>(stage_));
  // notify_initialize();
#ifdef ROKID_UPLOAD_TRACE
  shared_ptr<TraceEvent> ev = make_shared<TraceEvent>();
  ev->type = TRACE_EVENT_TYPE_SYS;
  ev->id = "system.speech.socket_error";
  ev->name = "socket连接错误";
  ev->add_key_value("service", service_type_);
  trace_uploader_->put(ev);
#endif
  if (ws_) {
    // lock stage_mutex_ for onDisconnection
    lock_guard<mutex> locker(stage_mutex_);
    ws_->close();
  } else {
    // ws_ == nullptr, stage_ must be CONNECTING
    // stage_mutex_ is already locked
    push_status_resp(BinRespType::ERROR);
    update_reconn_tp(options_.reconn_interval);
    stage_ = ConnectStage::DISCONN;
    stage_changed_.notify_all();
  }
}

void SpeechConnection::onPong(uWS::WebSocket<uWS::CLIENT> *ws,
    char* message, size_t length) {
  KLOGI(CONN_TAG, "uws recv pong: stage %d", static_cast<int>(stage_));
  update_recv_tp();
}

bool SpeechConnection::auth() {
  AuthRequest req;
  const char* svc = service_type_.c_str();
  string ts = timestamp();
  if (options_.key.empty()
      || options_.device_type_id.empty()
      || options_.device_id.empty()
      || options_.secret.empty()) {
    KLOGW(CONN_TAG, "auth invalid param");
#ifdef ROKID_UPLOAD_TRACE
    shared_ptr<TraceEvent> ev = make_shared<TraceEvent>();
    ev->type = TRACE_EVENT_TYPE_SYS;
    ev->id = "system.speech.auth_failed";
    ev->name = "认证包未发送或服务端认证失败";
    ev->add_key_value("service", service_type_);
    ev->add_key_value("reason", "认证参数不合法");
    ev->add_key_value("key", options_.key);
    trace_uploader_->put(ev);
#endif
    return false;
  }

  req.set_key(options_.key);
  req.set_device_type_id(options_.device_type_id);
  req.set_device_id(options_.device_id);
  req.set_service(svc);
  req.set_version(api_version_);
  req.set_timestamp(ts);
  req.set_sign(generate_sign(options_.key.c_str(),
          options_.device_type_id.c_str(),
          options_.device_id.c_str(), svc,
          api_version_, ts.c_str(),
          options_.secret.c_str()));

  std::string buf;
  if (!req.SerializeToString(&buf)) {
    KLOGW(CONN_TAG, "auth: protobuf serialize failed");
#ifdef ROKID_UPLOAD_TRACE
    shared_ptr<TraceEvent> ev = make_shared<TraceEvent>();
    ev->type = TRACE_EVENT_TYPE_SYS;
    ev->id = "system.speech.auth_failed";
    ev->name = "认证包未发送或服务端认证失败";
    ev->add_key_value("service", service_type_);
    ev->add_key_value("reason", "protobuf序列化失败");
    ev->add_key_value("key", options_.key);
    trace_uploader_->put(ev);
#endif
    return false;
  }
#ifdef ROKID_UPLOAD_TRACE
  shared_ptr<TraceEvent> ev = make_shared<TraceEvent>();
  ev->type = TRACE_EVENT_TYPE_SYS;
  ev->id = "system.speech.auth";
  ev->name = "发送认证包";
  ev->add_key_value("service", service_type_);
  ev->add_key_value("key", options_.key);
  trace_uploader_->put(ev);
#endif
  ws_->send(buf.data(), buf.length(), OpCode::BINARY);
  return true;
}

string SpeechConnection::timestamp() {
  struct timeval tv;
  uint64_t usecs;
  gettimeofday(&tv, NULL);
  usecs = tv.tv_sec;
  usecs *= 1000000;
  usecs += tv.tv_usec;
  char buf[64];
  snprintf(buf, sizeof(buf), "%" PRIu64, usecs);
  return string(buf);
}

string SpeechConnection::generate_sign(const char* key,
    const char* devtype, const char* devid, const char* svc,
    const char* version, const char* ts, const char* secret) {
  string sign_src;
  char buf[64];
  snprintf(buf, sizeof(buf), "key=%s", key);
  sign_src.append(buf);
  snprintf(buf, sizeof(buf), "&device_type_id=%s", devtype);
  sign_src.append(buf);
  snprintf(buf, sizeof(buf), "&device_id=%s", devid);
  sign_src.append(buf);
  snprintf(buf, sizeof(buf), "&service=%s", svc);
  sign_src.append(buf);
  snprintf(buf, sizeof(buf), "&version=%s", version);
  sign_src.append(buf);
  snprintf(buf, sizeof(buf), "&time=%s", ts);
  sign_src.append(buf);
  snprintf(buf, sizeof(buf), "&secret=%s", secret);
  sign_src.append(buf);
  uint8_t md5_res[MD5_DIGEST_LENGTH];
  MD5_CTX ctx;
  MD5_Init(&ctx);
  MD5_Update(&ctx, sign_src.c_str(), sign_src.length());
  MD5_Final(md5_res, &ctx);
  snprintf(buf, sizeof(buf), "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
      md5_res[0], md5_res[1], md5_res[2], md5_res[3],
      md5_res[4], md5_res[5], md5_res[6], md5_res[7],
      md5_res[8], md5_res[9], md5_res[10], md5_res[11],
      md5_res[12], md5_res[13], md5_res[14], md5_res[15]);
  KLOGV(CONN_TAG, "md5 src = %s, md5 result = %s",
      sign_src.c_str(), buf);
  return string(buf);
}

bool SpeechConnection::handle_auth_result(char* message, size_t length,
                                          uWS::OpCode opcode) {
  AuthResponse resp;
  if (!resp.ParseFromArray(message, length)) {
    KLOGW(CONN_TAG, "auth response parse failed, not correct protobuf");
#ifdef ROKID_UPLOAD_TRACE
    shared_ptr<TraceEvent> ev = make_shared<TraceEvent>();
    ev->type = TRACE_EVENT_TYPE_SYS;
    ev->id = "system.speech.auth_failed";
    ev->name = "认证包未发送或服务端认证失败";
    ev->add_key_value("service", service_type_);
    ev->add_key_value("reason", "认证响应包protobuf解析失败");
    char tmpstr[16];
    snprintf(tmpstr, sizeof(tmpstr), "%d", (int32_t)length);
    ev->add_key_value("resp_length", tmpstr);
    trace_uploader_->put(ev);
#endif
    return false;
  }
  KLOGV(CONN_TAG, "auth result = %d", resp.result());
  if (resp.result() == 0) {
    update_ping_tp();
    return true;
  }
  KLOGW(CONN_TAG, "auth failed, result = %d", resp.result());
  KLOGW(CONN_TAG, "auth failed: key(%s), device type(%s), device id(%s)",
      options_.key.c_str(), options_.device_type_id.c_str(),
      options_.device_id.c_str());
#ifdef ROKID_UPLOAD_TRACE
  shared_ptr<TraceEvent> ev = make_shared<TraceEvent>();
  ev->type = TRACE_EVENT_TYPE_SYS;
  ev->id = "system.speech.auth_failed";
  ev->name = "认证包未发送或服务端认证失败";
  ev->add_key_value("service", service_type_);
  ev->add_key_value("reason", "服务端认证失败");
  ev->add_key_value("key", options_.key);
  trace_uploader_->put(ev);
#endif
  return false;
}

bool SpeechConnection::ensure_connection_available(uint32_t timeout) {
  unique_lock<mutex> locker(stage_mutex_);
  if (stage_ == ConnectStage::PAUSED) {
    stage_ = ConnectStage::DISCONN;
    update_reconn_tp(0);
    stage_changed_.notify_all();
  }
  auto tp = SteadyClock::now() + milliseconds(timeout);
  while (stage_ != ConnectStage::READY) {
    if (timeout == 0)
      stage_changed_.wait(locker);
    else {
      if (stage_changed_.wait_until(locker, tp) == cv_status::timeout)
        return false;
    }
  }
  return true;
}

void SpeechConnection::push_status_resp(BinRespType tp) {
  SpeechBinaryResp* bin_resp;
  KLOGV(CONN_TAG, "push status response to list: %d", static_cast<int>(tp));
  bin_resp = (SpeechBinaryResp*)malloc(sizeof(SpeechBinaryResp));
  bin_resp->length = 0;
  bin_resp->type = tp;
  lock_guard<mutex> locker(resp_mutex_);
  responses_.push_back(bin_resp);
  resp_cond_.notify_one();
}

void SpeechConnection::push_resp_data(char* msg, size_t length) {
  SpeechBinaryResp* bin_resp;
  bin_resp = (SpeechBinaryResp*)malloc(length + sizeof(SpeechBinaryResp));
  bin_resp->length = length;
  bin_resp->type = BinRespType::DATA;
  memcpy(bin_resp->data, msg, length);
  lock_guard<mutex> locker(resp_mutex_);
  responses_.push_back(bin_resp);
  resp_cond_.notify_one();
}

void SpeechConnection::ws_send(const char* msg, size_t length, uWS::OpCode op) {
  if (ws_) {
    KLOGV(CONN_TAG, "SpeechConnection.ws_send: %lu bytes", length);
    ws_->send(msg, length, op);
  }
}

PrepareOptions::PrepareOptions() {
  host = "localhost";
  port = 80;
  branch = "/";
  reconn_interval = 20000;
  ping_interval = 30000;
  no_resp_timeout = 45000;
  conn_duration = 7200;
}

PrepareOptions& PrepareOptions::operator = (const PrepareOptions& options) {
  this->host = options.host;
  this->port = options.port;
  this->branch = options.branch;
  this->key = options.key;
  this->device_type_id = options.device_type_id;
  this->secret = options.secret;
  this->device_id = options.device_id;
  this->reconn_interval = options.reconn_interval;
  this->ping_interval = options.ping_interval;
  this->no_resp_timeout = options.no_resp_timeout;
  this->conn_duration = options.conn_duration;
  return *this;
}

} // namespace speech
} // namespace rokid
