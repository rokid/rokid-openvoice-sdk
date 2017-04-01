# Rokid Speech SDK
##c++ sdk 编译

**sdk依赖模块编译安装**

```
git clone https://github.com/grpc/grpc.git
cd grpc
git submodule update --init
git checkout v1.2.0
cd third_party/protobuf
./autogen.sh
./configure --prefix=<your_grpc_install_path>
make
make install
cd ../..
make prefix=<your_grpc_install_path>
make install
```

**sdk主体编译**

```
git clone https://github.com/Rokid/rokid-openvoice-sdk.git
cd rokid-openvoice-sdk
./autogen.sh <your_grpc_install_path>
make
```

## Android sdk 编译
**sdk依赖模块下载**

```
git clone https://github.com/Rokid/rokid-openvoice-sdk-deps-fastjson.git

git clone https://github.com/Rokid/rokid-openvoice-sdk-deps-protobuf.git

git clone https://github.com/Rokid/rokid-openvoice-sdk-deps-grpc.git

将以上三个模块目录放入android工程任意位置
如 <android_project>/openvoice/protobuf
<android_project>/openvoice/grpc
<android_project>/openvoice/fastjson
```

**sdk编译**

```
git clone https://github.com/Rokid/rokid-openvoice-sdk.git
将源码目录放入android工程任意位置
如<android_project>/openvoice/speech

将以下模块加入android工程makefile
PRODUCT_PACKAGES += \
	rprotoc \
	grpc_cpp_plugin \
	roots.pem \
	librokid_tts_jni \
	librokid_speech_jni \
	tts_sdk.json \
	speech_sdk.json \
	rokid_tts \
	rokid_speech \
	RKSpeechDemo

编译：
cd <android_project>
. build/envsetup.sh
lunch <your_conf>
make rprotoc
make grpc_cpp_plugin
make
```
### Tts接口定义 (android)
```
//  class Tts
boolean prepare()

int speak(String content, TtsCallback cb)

void stop(int id)

void config(String key, String value)

// class TtsCallback
void onStart(int id)

void onText(int id, String text)

void onVoice(int id, byte[] data)

void onStop(int id)

void onComplete(int id)

void onError(int id, int err)
```
### Tts使用示例 (android)
```
import com.rokid.speech.Tts;

// Tts构造函数传入配置文件路径名
// 配置文件见源码 android/etc/tts_sdk.json
// 其中定义了服务器地址，服务器认证所需的信息
// 服务器认证信息获取方式: https://developer-forum.rokid.com/t/rokid/101
Tts tts = new Tts("/system/etc/tts_sdk.json");
if (tts.prepare()) {
	// 指定tts音频编码格式
	// 'pcm', 'opu', 'opu2'
	// 默认为"pcm"
	tts.config("codec", "opu2");
	tts.speak("你好", new TtsCallback() {
		......
		});
}
```

### Speech接口定义
```
// class Speech
boolean prepare()

void release()

int putText(String text, SpeechCallback cb)

int startVoice(SpeechCallback cb)

void putVoice(int id, byte[] data)

void putVoice(int id, byte[] data, int offset, int length)

void endVoice(int id)

void cancel(int id)

void config(String key, String value)

// class SpeechCallback
void onStart(int id)

void onAsr(int id, String asr)

void onNlp(int id, String nlp)

void onAction(int id, String action)

void onComplete(int id)

void onStop(int id)

void onError(int id, int err)
```
### Speech使用示例
```
import com.rokid.speech.Speech;

// 构造函数传入配置文件路径名
// 配置文件格式与Tts类似
Speech speech = new Speech("/system/etc/speech_sdk.json")
if (!speech.prepare())
	return;
// 编译格式: "pcm", "opu"
// 默认为"pcm"
speech.config("codec", "opu");
speech.putText("若琪你好", new SpeechCallback() {
			......
			});
......
int id = speech.startVoice(new SpeechCallback() {
			......
			});
speech.putVoice(id, your_voice_data);
speech.putVoice(id, more_voice_data);
speech.endVoice(id);

// wait callback function invoked

// complete use speech
speech.release();
```
