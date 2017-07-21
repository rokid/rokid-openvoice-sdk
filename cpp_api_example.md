### Tts接口定义
**Tts类定义**

```
// 初始化，连接tts服务等操作
bool prepare()

// 结束使用并释放Tts系统资源
void release();

// 发起tts请求
int32_t speak(const char* content)

// 取消指定的tts请求，如未指定id (id <= 0)，取消所有未完成tts请求
void cancel(int32_t id)

// 获取tts结果数据。
// 返回true   成功得到Tts结果
// 返回false  tts prepare未调用或已被release
// 阻塞不返回  等待tts结果
bool poll(TtsResult& result)

// 进行配置，详见下面示例
void config(const char* key, const char* value)
```

**TtsResult定义**

```
// TtsResult类型
// TTS_RES_VOICE
// TTS_RES_START
// TTS_RES_END
// TTS_RES_CANCELLED
// TTS_RES_ERROR
TtsResultType type

// Tts请求id
int32_t id

// Tts错误码
// TTS_SUCCESS
// TTS_UNAUTHENTICATED
// TTS_CONNECTION_EXCEED
// TTS_SERVER_RESOURCE_EXHASTED
// TTS_SERVER_BUSY
// TTS_INTERNAL
// TTS_SERVICE_UNAVAILABLE
// TTS_SDK_CLOSED
// TTS_TIMEOUT
// TTS_UNKNOWN
TtsError err

// tts语音数据
std::shared_ptr<std::string> voice;
```

### Tts使用示例

```
#include "tts.h"
// Tts构造参数 配置文件名传null
shared_ptr<Tts> tts = new_tts();
// 在prepare前，先进行必要的配置
// 配置服务器信息
tts->config("host", "apigwws-dev.open.rokid.com");
tts->config("port", "443");
tts->config("branch", "/api");
// 配置认证信息
tts->config("key", my_key);
tts->config("device_type_id", my_device_type_id);
tts->config("secret", my_secret);
// 配置ssl及api版本
tts->config("ssl_roots_pem", "/etc/roots.pem");
tts->config("api_version", "1"); // 目前api版本为1
// 配置设备名，类似昵称，不影响认证结果，但必须在prepare之前配置
tts->config("device_id", "SmartDonkey");
// 连接服务器并认证，进行多项准备工作
if (!tts->prepare())
	return;
// 在prepare后任意时刻，都可以调用config修改配置
// 语音编码格式设定为pcm
tts->config("codec", "pcm");

// 使用tts
tts->speak("我是会说话的机器人，我最爱吃的食物是机油，最喜欢的运动是聊天");
// 更多的speak及其它api调用
......


// 在另一个线程
bool r;
TtsResult result;

while (true) {
	r = tts->poll(result);
	if (!r)
		break;
	handle_tts_result(result);
}
```

### Speech接口定义
**Speech类定义**

```
// 初始化
bool prepare();

// 释放
void prepare();

// 发起文本speech请求
int32_t put_text(const char* text)

// 发起语音speech请求
// 'fopts', 'sopts'  高级功能参数选项，普通用户无需使用
int32_t start_voice(shared_ptr<Options> fopts, shared_ptr<Options> sopts)

// 为指定语音speech请求发送语音数据，数据可分多次调用发送
void put_voice(int32_t id, const uint8_t* data, uint32_t length)

// 指定语音speech请求的语音数据发送完成
void end_voice(int32_t id)

// 取消指定speech请求，如果id未指定(id <= 0)，取消所有speech请求
void cancel(int32_t id)

void config(const char* key, const char* value)
```

**SpeechResult定义**

```
// speech请求id
int32_t id;

// speech result类型
// SPEECH_RES_INTER    <-- speech请求的中间结果 (asr部分结果, extra数据。extra为高级功能返回的数据，普通用户不要关心)
// SPEECH_RES_START    <-- speech请求开始返回结果数据
// SPEECH_RES_END      <-- speech请求最终结果数据(包括asr, nlp, action)
// SPEECH_RES_CANCELLED
// SPEECH_RES_ERROR
uint32_t type;

// SPEECH_SUCCESS
// SPEECH_UNAUTHENTICATED
// SPEECH_CONNECTION_EXCEED
// SPEECH_SERVER_RESOURCE_EXHASTED
// SPEECH_SERVER_BUSY
// SPEECH_SERVER_INTERNAL
// SPEECH_SERVICE_UNAVAILABLE
// SPEECH_SDK_CLOSED
// SPEECH_TIMEOUT
// SPEECH_UNKNOWN
SpeechError err;

// 语音转成的文本
std::string asr;
// 自然语义解析结果
std::string nlp;
// rokid cloud app解析结果
// 不使用rokid cloud app忽略此数据
std::string action;
```

### Speech使用示例

```
#include "speech.h"

shared_ptr<Speech> speech = new_speech();
// 在prepare前，先进行必要的配置
// 配置服务器信息
speech->config("host", "apigwws-dev.open.rokid.com");
speech->config("port", "443");
speech->config("branch", "/api");
// 配置认证信息
speech->config("key", my_key);
speech->config("device_type_id", my_device_type_id);
speech->config("secret", my_secret);
// 配置ssl及api版本
speech->config("ssl_roots_pem", "/etc/roots.pem");
speech->config("api_version", "1"); // 目前api版本为1
// 配置设备名，类似昵称，不影响认证结果，但必须在prepare之前配置
speech->config("device_id", "SmartDonkey");
// 连接服务器并认证，进行多项准备工作
if (!speech->prepare())
	return;
// 在prepare后任意时刻，都可以调用config修改配置
// 语音编码格式设定为pcm
speech->config("codec", "pcm");

// 使用speech
// 开始一次speech请求
int32_t id = speech->start_voice();
// 发送语音数据，流式，可分多次发送
speech->put_voice(id, data, data_length);
speech->put_voice(id, more data...);
...
// 声明此次speech语音请求数据发送完毕
speech->end_voice(id);
......


// 在另一个线程
bool r;
SpeechResult result;

while (true) {
	r = speech->poll(result);
	if (!r)
		break;
	handle_speech_result(result);
}
```

### Asr接口

**与Speech接口与用法类似，不再详细描述。**
