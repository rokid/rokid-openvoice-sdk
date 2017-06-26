### Tts接口定义
```
//  class Tts
// 初始化，连接tts服务等操作
void prepare()

// 发起tts请求
int speak(String content, TtsCallback cb)

// 取消指定的tts请求，如未指定id (id <= 0)，取消所有未完成tts请求
void cancel(int id)

// 进行配置，详见下面示例
void config(String key, String value)

// class TtsCallback
void onStart(int id)

void onText(int id, String text)

void onVoice(int id, byte[] data)

void onCancel(int id)

void onComplete(int id)

void onError(int id, int err)
```
### Tts使用示例

**不使用配置文件**

```
import com.rokid.speech.Tts;
// Tts构造参数 配置文件名传null
Tts tts = new Tts(null);
// 在prepare前，先进行必要的配置
// 配置服务器信息
tts.config("host", "apigwws-dev.open.rokid.com");
tts.config("port", "443");
tts.config("branch", "/api");
// 配置认证信息
tts.config("key", my_key);
tts.config("device_type_id", my_device_type_id);
tts.config("secret", my_secret);
// 配置ssl及api版本
tts.config("ssl_roots_pem", "/system/etc/roots.pem");
tts.config("api_version", "1"); // 目前api版本为1
// 配置设备名，类似昵称，不影响认证结果，但必须在prepare之前配置
tts.config("device_id", "SmartDonkey");
// 连接服务器并认证，进行多项准备工作
tts.prepare();
// 在prepare后任意时刻，都可以调用config修改配置
// 语音编码格式设定为pcm
tts.config("codec", "pcm");

// 使用tts
tts.speak("我是会说话的机器人，我最爱吃的食物是机油，最喜欢的运动是聊天",
			new TtsCallback() {
				// 在这里实现回调接口 onStart, onVoice等
				// 在onVoice中得到语音数据，调用播放器播放
				......
			});
// 更多的speak及其它api调用
```
**使用配置文件简化配置过程**

```
配置文件格式示例
{
    'host': 'apigwws.open.rokid.com',
    'port': '443',
    'branch': '/api',
    'ssl_roots_pem': '/system/etc/roots.pem',
    'api_version': '1',
    'key': 'your_auth_key',
    'device_type_id': 'your_device_type_id',
    'secret': 'your_secret',
    'device_id': 'your_device_id',
}
```

```
// 构造参数传入配置文件路径名
Tts tts = new Tts("/system/etc/speech_sdk.json");
// 配置文件中有全部服务器信息及认证信息，不需要再显示调用config配置
tts.prepare();
// 在prepare后任意时刻，都可以调用config修改配置
// 语音编码格式设定为pcm
// 可设定为'pcm', 'opu2'
// 默认为"pcm"
tts.config("codec", "pcm");
// 使用tts，与上一例中完全相同，不再重复
......
```

### Speech接口定义
```
// class Speech
// 初始化
void prepare();

// 发起文本speech请求
int putText(String text, SpeechCallback cb)

// 发起语音speech请求
int startVoice(SpeechCallback cb)

// 为指定语音speech请求发送语音数据，数据可分多次调用发送
void putVoice(int id, byte[] data)
void putVoice(int id, byte[] data, int offset, int length)

// 指定语音speech请求的语音数据发送完成
void endVoice(int id)

// 取消指定speech请求，如果id未指定(id <= 0)，取消所有speech请求
void cancel(int id)

void config(String key, String value)

// class SpeechCallback
void onStart(int id)

// 语音转成的文本
void onAsr(int id, String asr)

// 自然语义解析结果
void onNlp(int id, String nlp)

// rokid cloud app 解析结果。如果不使用rokid cloud app，忽略这一项
void onAction(int id, String action)

void onComplete(int id)

void onCancel(int id)

void onError(int id, int err)
```
### Speech使用示例

***下面的示例都使用配置文件。不使用配置文件的用法参见Tts示例，完全相同。***

```
import com.rokid.speech.Speech;

// 构造函数传入配置文件路径名
Speech speech = new Speech("/system/etc/speech_sdk.json")
speech.prepare();
// 音频格式: "pcm", "opu"
// 默认为"pcm"
speech.config("codec", "opu");
// 文本speech请求
speech.putText("若琪你好", new SpeechCallback() {
				// 在此实现onStart, onAsr, onNlp等接口
				......
			});
......
// 语音speech请求
int id = speech.startVoice(new SpeechCallback() {
				......
			});
speech.putVoice(id, your_voice_data);
speech.putVoice(id, more_voice_data);
speech.putVoice(id, ...);
...
speech.endVoice(id);

// wait callback function invoked
```

### Asr接口

```
// class Asr
// 初始化
void prepare();

// 发起asr请求
int startVoice(SpeechCallback cb)

// 为指定asr请求发送语音数据，数据可分多次调用发送
void putVoice(int id, byte[] data)
void putVoice(int id, byte[] data, int offset, int length)

// 指定asr请求的语音数据发送完成
void endVoice(int id)

// 取消指定asr请求，如果id未指定(id <= 0)，取消所有asr请求
void cancel(int id)

void config(String key, String value)

// class SpeechCallback
void onStart(int id)

// 语音转成的文本
void onAsr(int id, String asr)

void onComplete(int id)

void onCancel(int id)

void onError(int id, int err)
```

### Asr使用示例

```
import com.rokid.speech.Asr;

// 构造函数传入配置文件路径名
Asr asr = new Asr("/system/etc/speech_sdk.json")
asr.prepare();
// 音频格式: "pcm", "opu"
// 默认为"pcm"
asr.config("codec", "opu");
// 发起asr请求
int id = asr.startVoice(new AsrCallback() {
				// 在此实现onStart, onAsr等接口
				......
			});
asr.putVoice(id, your_voice_data);
asr.putVoice(id, more_voice_data);
asr.putVoice(id, ...);
...
asr.endVoice(id);

// wait callback function invoked
```
