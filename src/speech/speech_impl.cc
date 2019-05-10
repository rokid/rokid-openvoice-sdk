#include "speech_impl.h"

#define WS_SEND_TIMEOUT 5000

using std::shared_ptr;
using std::mutex;
using std::lock_guard;
using std::unique_lock;
using std::make_shared;
using std::chrono::system_clock;

namespace rokid {
namespace speech {

static const uint32_t MODIFY_LANG = 1;
static const uint32_t MODIFY_CODEC = 2;
static const uint32_t MODIFY_VADMODE = 4;
static const uint32_t MODIFY_NO_NLP = 8;
static const uint32_t MODIFY_NO_INTERMEDIATE_ASR = 0x10;
static const uint32_t MODIFY_VAD_BEGIN = 0x20;
static const uint32_t MODIFY_VOICE_FRAGMENT = 0x40;
static const uint32_t MODIFY_LOG_SERVER = 0x80;

class SpeechOptionsModifier : public SpeechOptionsHolder, public SpeechOptions {
public:
  SpeechOptionsModifier() {
    SpeechOptionsHolder();
    _mask = 0;
  }

  void set_lang(Lang lang) {
    this->lang = lang;
    _mask |= MODIFY_LANG;
  }

  void set_codec(Codec codec) {
    this->codec = codec;
    _mask |= MODIFY_CODEC;
  }

  void set_vad_mode(VadMode mode, uint32_t timeout) {
    this->vad_mode = mode;
    this->vend_timeout = timeout;
    _mask |= MODIFY_VADMODE;
  }

  void set_no_nlp(bool value) {
    this->no_nlp = value;
    _mask |= MODIFY_NO_NLP;
  }

  void set_no_intermediate_asr(bool value) {
    this->no_intermediate_asr = value;
    _mask |= MODIFY_NO_INTERMEDIATE_ASR;
  }

  void set_vad_begin(uint32_t value) {
    this->vad_begin = value;
    _mask |= MODIFY_VAD_BEGIN;
  }

  void set_log_server(const char* host, int32_t port) {
    this->log_host = host;
    this->log_port = port;
    _mask |= MODIFY_LOG_SERVER;
  }

  void set_voice_fragment(uint32_t size) {
    this->voice_fragment = size;
    _mask |= MODIFY_VOICE_FRAGMENT;
  }

  void modify(SpeechOptionsHolder& options) {
    if (_mask & MODIFY_LANG)
      options.lang = lang;
    if (_mask & MODIFY_CODEC)
      options.codec = codec;
    if (_mask & MODIFY_VADMODE) {
      options.vad_mode = vad_mode;
      options.vend_timeout = vend_timeout;
    }
    if (_mask & MODIFY_NO_NLP)
      options.no_nlp = no_nlp;
    if (_mask & MODIFY_NO_INTERMEDIATE_ASR)
      options.no_intermediate_asr = no_intermediate_asr;
    if (_mask & MODIFY_VAD_BEGIN)
      options.vad_begin = vad_begin;
    if (_mask & MODIFY_LOG_SERVER) {
      options.log_host = log_host;
      options.log_port = log_port;
    }
    if (_mask & MODIFY_VOICE_FRAGMENT)
      options.voice_fragment = voice_fragment;
    KLOGD(tag__, "SpeechOptions modified to: vad(%s:%u), codec(%s), "
        "lang(%s), no_nlp(%d), no_intermediate_asr(%d), "
        "vad_begin(%u), log server(%s:%d), voice_fragment(%u)",
        options.vad_mode == VadMode::CLOUD ? "cloud" : "local",
        options.vend_timeout,
        options.codec == Codec::OPU ? "opu" : "pcm",
        options.lang == Lang::EN ? "en" : "zh",
        options.no_nlp,
        options.no_intermediate_asr,
        options.vad_begin,
        options.log_host.c_str(),
        options.log_port,
        options.voice_fragment);
  }

private:
  uint32_t _mask;
};

SpeechImpl::SpeechImpl() : initialized_(false) {
#ifdef SPEECH_STATISTIC
  cur_trace_info_.id = 0;
#endif
}

SpeechImpl::~SpeechImpl() {
  release();
}

bool SpeechImpl::prepare(const PrepareOptions& options) {
  lock_guard<mutex> init_locker(init_mutex_);
  KLOGV(tag__, "SpeechImpl.prepare, initialized = %d", initialized_);
  lock_guard<mutex> locker(req_mutex_);
  if (initialized_)
    return true;
#ifdef HAS_OPUS_CODEC
  opus_encoder_.init(16000, 27800, 20);
#endif
  next_id_ = 0;
  connection_.initialize(SOCKET_BUF_SIZE, options, "speech");
  initialized_ = true;
  req_thread_ = new thread([=] { send_reqs(); });
  resp_thread_ = new thread([=] { gen_results(); });
  return true;
}

void SpeechImpl::release() {
  lock_guard<mutex> init_locker(init_mutex_);
  KLOGV(tag__, "SpeechImpl.release, initialized = %d", initialized_);
  unique_lock<mutex> req_locker(req_mutex_);
  if (initialized_) {
    // notify req thread to exit
    initialized_ = false;
    connection_.release();
    voice_reqs_.close();
    text_reqs_.clear();
    req_cond_.notify_one();
    req_locker.unlock();
    // fix bug: 'wait_op_finish' block 'send_reqs' thread forever
    controller_.set_op_error(SPEECH_SDK_CLOSED);
    req_thread_->join();
    delete req_thread_;

    // notify resp thread to exit
    unique_lock<mutex> resp_locker(resp_mutex_);
    responses_.close();
    controller_.finish_op();
    resp_cond_.notify_one();
    resp_locker.unlock();
    resp_thread_->join();
    delete resp_thread_;

#ifdef HAS_OPUS_CODEC
    opus_encoder_.close();
#endif
  }
}

int32_t SpeechImpl::put_text(const char* text, const VoiceOptions* options) {
  if (!initialized_ || text == NULL)
    return -1;
  int32_t id = next_id();
  lock_guard<mutex> locker(req_mutex_);
  shared_ptr<SpeechReqInfo> p(new SpeechReqInfo());
  p->id = id;
  p->type = SpeechReqType::TEXT;
  p->data.reset(new string(text));
  if (options) {
    p->options = make_shared<VoiceOptions>();
    *p->options = *options;
  }
  text_reqs_.push_back(p);
  KLOGV(tag__, "put text %d, %s", id, text);
  req_cond_.notify_one();
  return id;
}

int32_t SpeechImpl::start_voice(const VoiceOptions* options) {
  if (!initialized_)
    return -1;
  lock_guard<mutex> locker(req_mutex_);
  int32_t id = next_id();
  if (!voice_reqs_.start(id))
    return -1;
  shared_ptr<VoiceOptions> arg;
  if (options) {
    arg = make_shared<VoiceOptions>();
    *arg = *options;
  }
  voice_reqs_.set_arg(id, arg);
  KLOGV(tag__, "start voice %d", id);
  req_cond_.notify_one();
  return id;
}

static bool is_stream_codec(Codec codec) {
  return codec == Codec::PCM;
}

void SpeechImpl::put_voice(int32_t id, const uint8_t* voice, uint32_t length) {
  if (!initialized_)
    return;
  if (id <= 0 || voice == NULL || length == 0)
    return;
#ifdef HAS_OPUS_CODEC
  if (options_.codec == Codec::PCM) {
    uint32_t enc_size;
    const uint8_t* opu = opus_encoder_.encode(
        reinterpret_cast<const uint16_t*>(voice),
        length / sizeof(uint16_t), enc_size);
    KLOGD(tag__, "put voice %u bytes, encoded to %u bytes", length, enc_size);
    if (enc_size == 0)
      return;
    voice = opu;
    length = enc_size;
  }
#endif
  lock_guard<mutex> locker(req_mutex_);
  shared_ptr<string> spv;
  const char* strp = reinterpret_cast<const char*>(voice);
  uint32_t off = 0;
  uint32_t sz;
  bool need_notify = false;
  while (off < length) {
    sz = length - off;
    if (is_stream_codec(options_.codec) && options_.voice_fragment < sz)
      sz = options_.voice_fragment;
    spv = make_shared<string>(strp + off, sz);
    off += sz;
    if (voice_reqs_.stream(id, spv)) {
      need_notify = true;
    } else {
      KLOGI(tag__, "put voice failed, maybe id %d is invalid", id);
    }
  }
  if (need_notify) {
    KLOGV(tag__, "put voice %d, len %u", id, length);
    req_cond_.notify_one();
  }
}

void SpeechImpl::end_voice(int32_t id) {
  if (!initialized_)
    return;
  if (id <= 0)
    return;
  lock_guard<mutex> locker(req_mutex_);
  if (voice_reqs_.end(id)) {
    KLOGV(tag__, "end voice %d", id);
    req_cond_.notify_one();
  }
}

void SpeechImpl::cancel(int32_t id) {
  unique_lock<mutex> req_locker(req_mutex_);
  if (!initialized_)
    return;
  KLOGV(tag__, "cancel %d", id);
  if (id > 0) {
    if (voice_reqs_.erase(id)) {
      req_cond_.notify_one();
      return;
    }
    list<shared_ptr<SpeechReqInfo> >::iterator it;
    for (it = text_reqs_.begin(); it != text_reqs_.end(); ++it) {
      if ((*it)->id == id) {
        (*it)->type = SpeechReqType::CANCELLED;
        return;
      }
    }
    req_locker.unlock();
    lock_guard<mutex> resp_locker(resp_mutex_);
    controller_.cancel_op(id, resp_cond_);
  } else {
    int32_t min_id;
    voice_reqs_.clear(&min_id, NULL);
    if (min_id > 0)
      req_cond_.notify_one();
    list<shared_ptr<SpeechReqInfo> >::iterator it;
    for (it = text_reqs_.begin(); it != text_reqs_.end(); ++it) {
      (*it)->type = SpeechReqType::CANCELLED;
    }
    req_locker.unlock();
    lock_guard<mutex> resp_locker(resp_mutex_);
    controller_.cancel_op(0, resp_cond_);
  }
}

void SpeechImpl::erase_req(int32_t id) {
  lock_guard<mutex> req_locker(req_mutex_);
  if (voice_reqs_.erase(id, SPEECH_TIMEOUT)) {
    req_cond_.notify_one();
  }
}

void SpeechImpl::config(const shared_ptr<SpeechOptions>& options) {
  if (options.get() == NULL)
    return;
  shared_ptr<SpeechOptionsModifier> mod =
    static_pointer_cast<SpeechOptionsModifier>(options);
  mod->modify(options_);
#ifdef HAS_OPUS_CODEC
  if (options_.codec == Codec::PCM)
    opus_encoder_.init(16000, 27800, 20);
  else
    opus_encoder_.close();
#endif
  if (options_.log_host.size() > 0) {
    char buf[64];
    snprintf(buf, sizeof(buf), "tcp://%s:%d/",
             options_.log_host.c_str(), options_.log_port);
    if (RLog::add_endpoint("socket", ROKID_LOGWRITER_SOCKET) == 0)
      RLog::enable_endpoint("socket", buf, true);
  }
}

void SpeechImpl::reconn() {
  connection_.reconn();
}

static SpeechResultType poptype_to_restype(int32_t type) {
  static SpeechResultType _tps[] = {
    SPEECH_RES_INTER,
    SPEECH_RES_START,
    SPEECH_RES_END,
    SPEECH_RES_CANCELLED,
    SPEECH_RES_ERROR,
  };
  assert(type >= 0 && type < sizeof(_tps)/sizeof(SpeechResultType));
  return _tps[type];
}

static const char *speech_err_str(enum SpeechError err) {
  switch (err) {
  case SPEECH_SUCCESS:
    return "SUCCESS";
  case SPEECH_UNAUTHENTICATED:
    return "UNAUTHENTICATED";
  case SPEECH_CONNECTION_EXCEED:
    return "CONNECTION_EXCEED";
  case SPEECH_SERVER_RESOURCE_EXHASTED:
    return "SERVER_RESOURCE_EXHASTED";
  case SPEECH_SERVER_BUSY:
    return "SERVER_BUSY";
  case SPEECH_SERVER_INTERNAL:
    return "SERVER_INTERNAL";
  case SPEECH_VAD_TIMEOUT:
    return "VAD_TIMEOUT";
  case SPEECH_NLP_EMPTY:
    return "NLP_EMPTY";
  case SPEECH_UNINITIALIZED:
    return "UNINITIALIZED";
  case SPEECH_DUP_INITIALIZED:
    return "DUP_INITIALIZED";
  case SPEECH_BADREQUEST:
    return "BADREQUEST";
  case SPEECH_SERVICE_UNAVAILABLE:
    return "SERVICE_UNAVAILABLE";
  case SPEECH_SDK_CLOSED:
    return "SDK_CLOSED";
  case SPEECH_TIMEOUT:
    return "TIMEOUT";
  default:
    KLOGV(tag__, "unknown err code: %d", err);
    break;
  }
  return "UNKNOWN";
}

bool SpeechImpl::poll(SpeechResult& res) {
  shared_ptr<SpeechOperationController::Operation> op;
  int32_t id;
  shared_ptr<SpeechResultIn> resin;
  int32_t poptype;
  uint32_t err = 0;

  res.err = SPEECH_SUCCESS;
  res.asr.clear();
  res.nlp.clear();
  res.action.clear();
  res.extra.clear();
  res.voice_trigger.clear();

  unique_lock<mutex> locker(resp_mutex_);
  while (initialized_) {
    op = controller_.front_op();
    if (op.get() == NULL) {
      KLOGI(tag__, "SpeechImpl.poll: front op = (nil)");
    }
    if (op.get()) {
      KLOGV(tag__, "SpeechImpl.poll: front op status = %d", op->status);
      if (op->status == SpeechStatus::CANCELLED) {
        if (responses_.erase(op->id)) {
          responses_.pop(id, resin, err);
          assert(id == op->id);
        }
        res.id = op->id;
        res.type = SPEECH_RES_CANCELLED;
        res.err = SPEECH_SUCCESS;
        controller_.remove_front_op();
        KLOGV(tag__, "SpeechImpl.poll (%d) cancelled, "
            "remove front op", op->id);
        KLOGI(tag__, "voice recognize (%d) CANCELLED", res.id);
        return true;
      } else if (op->status == SpeechStatus::ERROR) {
        if (responses_.erase(op->id)) {
          responses_.pop(id, resin, err);
          assert(id == op->id);
        }
        res.id = op->id;
        res.type = SPEECH_RES_ERROR;
        res.err = op->error;
        controller_.remove_front_op();
        KLOGV(tag__, "SpeechImpl.poll (%d) error, "
            "remove front op", op->id);
        KLOGI(tag__, "voice recognize (%d) ERROR: %u - %s",
          res.id, res.err, speech_err_str(res.err));
        return true;
      } else {
        poptype = responses_.pop(id, resin, err);
        if (poptype != ReqStreamQueue::POP_TYPE_EMPTY) {
          assert(id == op->id);
          res.id = id;
          res.type = poptype_to_restype(poptype);
          res.err = static_cast<SpeechError>(err);
          if (res.err != SPEECH_SUCCESS)
            KLOGI(tag__, "voice recognize (%d) result: %u - %s",
              id, res.err, speech_err_str(res.err));
          if (resin.get()) {
            if (resin->asr_finish) {
              res.type = SpeechResultType::SPEECH_RES_ASR_FINISH;
              KLOGI(tag__, "voice recognize result asr %d:%s", id, resin->asr.c_str());
            }
            res.asr = resin->asr;
            res.nlp = resin->nlp;
            res.action = resin->action;
            res.extra = resin->extra;
            res.voice_trigger = resin->voice_trigger;
            if (res.asr.length() > 0) {
              KLOGI(tag__, "voice recognize intermediate asr %d:%s", id, resin->asr.c_str());
            }
            if (res.extra.length() > 0) {
              KLOGI(tag__, "voice recognize extra %d:%s", id, resin->extra.c_str());
            }
            if (res.nlp.length() > 0) {
              KLOGI(tag__, "voice recognize nlp/action id %d", id);
            }
            if (res.voice_trigger.length() > 0) {
              KLOGI(tag__, "voice recognize voice_trigger id %d %s",
                    id, resin->voice_trigger.c_str());
            }
          }
          KLOGV(tag__, "SpeechImpl.poll return result "
              "id(%d), type(%d)", res.id, res.type);
          if (res.type >= SPEECH_RES_END) {
            KLOGV(tag__, "SpeechImpl.poll (%d) end", res.id);
            controller_.remove_front_op();
          }
          return true;
        }
      }
    }
    KLOGV(tag__, "SpeechImpl.poll wait");
    resp_cond_.wait(locker);
  }
  KLOGV(tag__, "SpeechImpl.poll return false, sdk released");
  return false;
}

static SpeechReqType sqtype_to_reqtype(int32_t type) {
  static SpeechReqType _tps[] = {
    SpeechReqType::VOICE_DATA,
    SpeechReqType::VOICE_START,
    SpeechReqType::VOICE_END,
    SpeechReqType::CANCELLED,
    // timeout error, send voice end to server
    SpeechReqType::VOICE_END
  };
  assert(type >= 0 && type < sizeof(_tps)/sizeof(SpeechReqType));
  return _tps[type];
}

void SpeechImpl::send_reqs() {
  int32_t r;
  int32_t id;
  shared_ptr<string> voice;
  uint32_t err;
  int32_t rv;
  shared_ptr<SpeechReqInfo> info;
  shared_ptr<SpeechOperationController::Operation> op;
  bool opr;

  KLOGV(tag__, "thread 'send_reqs' begin");
  while (true) {
    unique_lock<mutex> locker(req_mutex_);
    if (!initialized_)
      break;
    r = voice_reqs_.pop(id, voice, err);
    if (r >= 0) {
      info.reset(new SpeechReqInfo());
      info->id = id;
      info->type = sqtype_to_reqtype(r);
      info->data = voice;
      info->options = voice_reqs_.get_arg(id);
    } else {
      bool has_req = false;
      if (!text_reqs_.empty()) {
        locker.unlock();
        resp_mutex_.lock();
        op = controller_.current_op();
        resp_mutex_.unlock();
        locker.lock();
        if (op.get() == NULL) {
          info = text_reqs_.front();
          text_reqs_.pop_front();
          has_req = true;
        }
      }
      if (!has_req) {
        KLOGV(tag__, "SpeechImpl.send_reqs wait req available");
        req_cond_.wait(locker);
        KLOGV(tag__, "SpeechImpl.send_reqs awake");
        continue;
      }
    }
    locker.unlock();
    opr = do_ctl_change_op(info);

    if (opr) {
      rv = do_request(info);
      if (rv == 0) {
        KLOGV(tag__, "SpeechImpl.send_reqs wait op finish");
        unique_lock<mutex> resp_locker(resp_mutex_);
        controller_.wait_op_finish(info->id, resp_locker);
      }
    }
  }
  KLOGV(tag__, "thread 'send_reqs' quit");
}

bool SpeechImpl::do_ctl_change_op(shared_ptr<SpeechReqInfo>& req) {
  unique_lock<mutex> locker(resp_mutex_);
  shared_ptr<SpeechOperationController::Operation> op =
    controller_.current_op();
  locker.unlock();

  KLOGV(tag__, "do_ctl_change_op: current op is %p", op.get());
  if (op.get()) {
    KLOGV(tag__, "do_ctl_change_op: current op id(%d), status(%d)",
        op->id, op->status);
  }
  KLOGV(tag__, "do_ctl_change_op: req id(%d), type(%d)",
      req->id, req->type);
  if (req->type == SpeechReqType::TEXT
      || req->type == SpeechReqType::VOICE_START) {
    assert(op.get() == NULL);
    locker.lock();
    controller_.new_op(req->id, SpeechStatus::START);
    return true;
  }
  if (op.get()) {
    if (req->type == SpeechReqType::VOICE_END
        || req->type == SpeechReqType::VOICE_DATA)
      return true;
    if (req->type == SpeechReqType::CANCELLED) {
      locker.lock();
      op->status = SpeechStatus::CANCELLED;
      controller_.clear_current_op();
      resp_cond_.notify_one();
      return true;
    }
    return false;
  }
  if (req->type == SpeechReqType::CANCELLED) {
    locker.lock();
    controller_.new_op(req->id, SpeechStatus::CANCELLED);
    // no data send to server
    // notify 'poll' function to generate 'CANCEL' result
    resp_cond_.notify_one();
    return false;
  }
  return false;
}

void SpeechImpl::req_config(SpeechRequest& req,
    const shared_ptr<VoiceOptions>& options) {
  SpeechOptionsEnc* sopt = req.mutable_options();
  rokid_open_speech_v1_Codec codec;

  sopt->set_lang(static_cast<rokid_open_speech_v2_Lang>(options_.lang));
#ifdef HAS_OPUS_CODEC
  codec = (options_.codec == Codec::PCM)
    ? rokid_open_speech_v1_Codec_OPU
    : static_cast<rokid_open_speech_v1_Codec>(options_.codec);
#else
  codec = static_cast<rokid_open_speech_v1_Codec>(options_.codec);
#endif
  sopt->set_codec(codec);
  sopt->set_vad_mode(static_cast<rokid_open_speech_v2_VadMode>(options_.vad_mode));
  sopt->set_vend_timeout(options_.vend_timeout);
  sopt->set_no_nlp(options_.no_nlp);
  sopt->set_no_intermediate_asr(options_.no_intermediate_asr);
  sopt->set_vad_begin(options_.vad_begin);
  KLOGI(tag__, "speech config: codec(%d), vad mode(%d:%u), vad begin(%u), no nlp(%d), no intermediate asr(%d), voice fragment(%u)",
      codec, options_.vad_mode, options_.vend_timeout, options_.vad_begin,
      options_.no_nlp, options_.no_intermediate_asr, options_.voice_fragment);
  if (options.get()) {
    sopt->set_stack(options->stack);
    sopt->set_voice_trigger(options->voice_trigger);
    sopt->set_voice_power(options->voice_power);
    sopt->set_trigger_start(options->trigger_start);
    sopt->set_trigger_length(options->trigger_length);
    sopt->set_no_trigger_confirm(!options->trigger_confirm_by_cloud);
    sopt->set_skill_options(options->skill_options);
    sopt->set_voice_extra(options->voice_extra);
    KLOGD(tag__, "VoiceOptions: stack(%s), voice_trigger(%s), "
        "trigger_start(%u), trigger_length(%u), voice_power(%f), "
        "skill_options(%s), voice_extra(%s), trigger_confirm_by_cloud(%d)",
        options->stack.c_str(), options->voice_trigger.c_str(),
        options->trigger_start, options->trigger_length,
        options->voice_power, options->skill_options.c_str(),
        options->voice_extra.c_str(), options->trigger_confirm_by_cloud);
  }
}

int32_t SpeechImpl::do_request(shared_ptr<SpeechReqInfo>& req) {
  SpeechRequest treq;
  int32_t rv = 1;
  uint32_t send_timeout = 1;
  switch (req->type) {
    case SpeechReqType::TEXT: {
      treq.set_id(req->id);
      treq.set_type(rokid_open_speech_v1_ReqType_TEXT);
      treq.set_asr(*req->data);
      req_config(treq, req->options);
      rv = 0;
      send_timeout = WS_SEND_TIMEOUT;

#ifdef SPEECH_STATISTIC
      cur_trace_info_.id = req->id;
      cur_trace_info_.req_tp = system_clock::now();
#endif

      KLOGD(tag__, "SpeechImpl.do_request (%d) send text req",
          req->id);
      break;
    }
    case SpeechReqType::VOICE_START: {
      treq.set_id(req->id);
      treq.set_type(rokid_open_speech_v1_ReqType_START);
      req_config(treq, req->options);
      send_timeout = WS_SEND_TIMEOUT;

#ifdef SPEECH_STATISTIC
      cur_trace_info_.id = req->id;
      cur_trace_info_.req_tp = system_clock::now();
#endif

      KLOGD(tag__, "SpeechImpl.do_request (%d) send voice start",
          req->id);
      break;
    }
    case SpeechReqType::VOICE_END:
      treq.set_id(req->id);
      treq.set_type(rokid_open_speech_v1_ReqType_END);
      rv = 0;
      KLOGD(tag__, "SpeechImpl.do_request (%d) send voice end",
          req->id);
      break;
    case SpeechReqType::CANCELLED:
      treq.set_id(req->id);
      treq.set_type(rokid_open_speech_v1_ReqType_END);
      KLOGD(tag__, "SpeechImpl.do_request (%d) send voice end"
          " because req cancelled", req->id);
#ifdef SPEECH_STATISTIC
      finish_cur_req();
#endif
      break;
    case SpeechReqType::VOICE_DATA:
      treq.set_id(req->id);
      treq.set_type(rokid_open_speech_v1_ReqType_VOICE);
      treq.set_voice(*req->data);
      KLOGV(tag__, "SpeechImpl.do_request (%d) send voice data",
          req->id);
      break;
    default:
      KLOGW(tag__, "SpeechImpl.do_request: (%d) req type is %u, "
          "it's impossible!", req->id, req->type);
      assert(false);
      return -1;
  }

  ConnectionOpResult r = connection_.send(treq, send_timeout);
  if (r != ConnectionOpResult::SUCCESS) {
    SpeechError err = SPEECH_UNKNOWN;
    if (r == ConnectionOpResult::CONNECTION_NOT_AVAILABLE)
      err = SPEECH_SERVICE_UNAVAILABLE;
    KLOGI(tag__, "SpeechImpl.do_request: (%d) send req failed "
        "%d, set op error", req->id, r);
    lock_guard<mutex> locker(resp_mutex_);
    controller_.set_op_error(err);
    resp_cond_.notify_one();
    erase_req(req->id);
    return -1;
  } else if (rv == 0) {
    KLOGV(tag__, "req (%d) last data sent, req done", req->id);
  }
  lock_guard<mutex> locker(resp_mutex_);
  controller_.refresh_op_time(false);
  return rv;
}

void SpeechImpl::gen_results() {
  SpeechResponse resp;
  ConnectionOpResult r;
  SpeechError err;
  uint32_t timeout;

  KLOGV(tag__, "thread 'gen_results' run");
  while (true) {
    unique_lock<mutex> locker(resp_mutex_);
    timeout = controller_.op_timeout();
    locker.unlock();

    r = connection_.recv(resp, timeout);
    KLOGD(tag__, "speech connection recv result %d", r);
    if (r == ConnectionOpResult::NOT_READY)
      break;
    locker.lock();
    if (r == ConnectionOpResult::SUCCESS) {
      controller_.refresh_op_time(true);
      gen_result_by_resp(resp, locker);
    } else if (r == ConnectionOpResult::TIMEOUT) {
      if (controller_.op_timeout() == 0) {
        int32_t id = controller_.current_op()->id;
        KLOGI(tag__, "gen_results: (%d) op timeout, "
            "set op error", controller_.current_op()->id);
        controller_.set_op_error(SPEECH_TIMEOUT);
        resp_cond_.notify_one();
        locker.unlock();
        erase_req(id);
#ifdef SPEECH_STATISTIC
        finish_cur_req();
#endif
        continue;
      }
    } else if (r == ConnectionOpResult::CONNECTION_BROKEN) {
      shared_ptr<SpeechOperationController::Operation> op
        = controller_.current_op();
      KLOGI(tag__, "connection broken, current speech abort");
      controller_.set_op_error(SPEECH_SERVICE_UNAVAILABLE);
      resp_cond_.notify_one();
      locker.unlock();
      if (op.get()) {
        erase_req(op->id);
#ifdef SPEECH_STATISTIC
        finish_cur_req();
#endif
      }
      continue;
    } else {
      shared_ptr<SpeechOperationController::Operation> op
        = controller_.current_op();
      controller_.set_op_error(SPEECH_UNKNOWN);
      resp_cond_.notify_one();
      locker.unlock();
      if (op.get()) {
        erase_req(op->id);
#ifdef SPEECH_STATISTIC
        finish_cur_req();
#endif
      }
      continue;
    }
    locker.unlock();
  }
  KLOGV(tag__, "thread 'gen_results' quit");
}

void SpeechImpl::gen_result_by_resp(SpeechResponse& resp, unique_lock<mutex>& resp_locker) {
  bool new_data = false;
  int32_t erase_req_id = -1;
  shared_ptr<SpeechOperationController::Operation> op =
    controller_.current_op();
  if (op.get()) {
    KLOGI(tag__, "gen_result_by_resp: current op id(%d), status(%d)",
        op->id, op->status);
  } else {
    KLOGI(tag__, "gen_result_by_resp: opctl is null");
  }
  KLOGI(tag__, "gen_result_by_resp: resp id(%d), type(%d), result(%d), asr(%s), extra(%s)",
      resp.id(), resp.type(), resp.result(), resp.asr().c_str(), resp.extra().c_str());
  if (op.get() && op->id == resp.id()
      && op->status != SpeechStatus::CANCELLED
      && op->status != SpeechStatus::ERROR) {
    if (op->status == SpeechStatus::START) {
      responses_.start(resp.id());
      op->status = SpeechStatus::STREAMING;
      new_data = true;
    }

    shared_ptr<SpeechResultIn> resin;
    string extra = resp.extra();
    string voice_trigger = resp.voice_trigger();
    if (extra.length() > 0) {
      resin = make_shared<SpeechResultIn>();
      resin->extra = extra;
      resin->asr_finish = false;
      responses_.stream(resp.id(), resin);
      new_data = true;
    }

    resin = make_shared<SpeechResultIn>();
    if (voice_trigger.length() > 0) {
      resin->voice_trigger = voice_trigger;
    }
    switch (resp.type()) {
    case rokid_open_speech_v2_RespType_INTERMEDIATE:
      resin->asr = resp.asr();
      resin->asr_finish = false;
      responses_.stream(resp.id(), resin);
      new_data = true;
      break;
    case rokid_open_speech_v2_RespType_ASR_FINISH:
      resin->asr = resp.asr();
      resin->asr_finish = true;
      responses_.stream(resp.id(), resin);
      new_data = true;
      break;
    case rokid_open_speech_v2_RespType_FINISH:
      if (resp.result() == rokid_open_speech_v1_SpeechErrorCode_SUCCESS) {
        resin->asr = resp.asr();
        resin->nlp = resp.nlp();
        resin->action = resp.action();
        resin->asr_finish = false;
        responses_.end(resp.id(), resin);
        new_data = true;
        op->status = SpeechStatus::END;
        controller_.finish_op();
        erase_req_id = resp.id();
      } else {
        responses_.erase(resp.id(), resp.result());
        new_data = true;
        controller_.finish_op();
        erase_req_id = resp.id();
      }
#ifdef SPEECH_STATISTIC
      finish_cur_req();
#endif
      break;
    default:
      KLOGW(tag__, "invalid SpeechResponse.type %d", resp.type());
      return;
    }

    if (new_data) {
      resp_cond_.notify_one();
    }

    resp_locker.unlock();
    if (erase_req_id > 0) {
      erase_req(erase_req_id);
    }
    resp_locker.lock();
  }
}

#ifdef SPEECH_STATISTIC
void SpeechImpl::finish_cur_req() {
  if (cur_trace_info_.id) {
    cur_trace_info_.resp_tp = system_clock::now();
    connection_.add_trace_info(cur_trace_info_);
    cur_trace_info_.id = 0;
  }
}
#endif

shared_ptr<Speech> Speech::new_instance() {
  return make_shared<SpeechImpl>();
}

VoiceOptions::VoiceOptions()
  : trigger_start(0), trigger_length(0)
  , trigger_confirm_by_cloud(1), voice_power(0.0) {
}

VoiceOptions& VoiceOptions::operator = (const VoiceOptions& options) {
  stack = options.stack;
  voice_trigger = options.voice_trigger;
  trigger_start = options.trigger_start;
  trigger_length = options.trigger_length;
  trigger_confirm_by_cloud = options.trigger_confirm_by_cloud;
  voice_power = options.voice_power;
  skill_options = options.skill_options;
  voice_extra = options.voice_extra;
  return *this;
}

SpeechOptionsHolder::SpeechOptionsHolder() {
  no_nlp = 0;
  no_intermediate_asr = 0;
}

shared_ptr<SpeechOptions> SpeechOptions::new_instance() {
  return make_shared<SpeechOptionsModifier>();
}

} // namespace speech
} // namespace rokid
