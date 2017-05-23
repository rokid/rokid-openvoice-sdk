# Rokid Speech SDK
## sdk 编译 (Android平台）
**sdk依赖模块下载**

```
git clone https://github.com/Rokid/rokid-openvoice-sdk-deps-fastjson.git

git clone https://github.com/Rokid/rokid-openvoice-sdk-deps-protobuf -b android23

git clone https://github.com/Rokid/rokid-openvoice-sdk-deps-poco.git

将以上三个模块目录放入android工程任意位置
如 <android_project>/openvoice/poco
  <android_project>/openvoice/protobuf
```

**sdk编译**

```
git clone https://github.com/Rokid/rokid-openvoice-sdk.git
将源码目录放入android工程任意位置
如<android_project>/openvoice/speech

将以下模块加入android工程makefile
PRODUCT_PACKAGES += \
	libprotobuf-rokid-cpp-full \
	libpoco \
	roots.pem \
	libspeech_common \
	libspeech_tts \
	libspeech_asr \
	libspeech \
	librokid_tts_jni \
	librokid_asr_jni \
	librokid_speech_jni \
	tts_sdk.json \
	asr_sdk.json \
	speech_sdk.json \
	rokid_tts \
	rokid_asr \
	rokid_speech \
	RKSpeechDemo

编译：
cd <android_project>
. build/envsetup.sh
lunch <your_conf>
make
```

## sdk 编译 (Ubuntu平台)

**sdk依赖模块编译安装**

* 安装openssl

```
sudo apt-get install libssl-dev
```

* 编译安装poco

[下载链接](https://pocoproject.org/releases/poco-1.7.8/poco-1.7.8p2-all.tar.gz)

```
tar xvfz poco-1.7.8p2-all.tar.gz
cd poco-1.7.8p2-all
./configure --config=Linux --shared --no-tests --no-samples --omit=CppUnit,CppParser,CodeGeneration,PageCompiler,Remoting,Data/MySQL,Data/ODBC,Zip,XML --prefix=${your_deps_dir}
make
make install
```

**重要：${your\_deps\_dir}是speech sdk所有依赖项的安装目录，protobuf及poco都应安装在这里，由你指定。**

* 安装protobuf

```
git clone https://github.com/google/protobuf.git
cd protobuf
git checkout v2.6.1
编辑autogen.sh，删除安装gtest的命令。否则会出错，该url已经不存在了。
./autogen.sh
./configure --prefix=${your_deps_dir}
make
make install
```

**sdk主体编译**

```
git clone https://github.com/Rokid/rokid-openvoice-sdk.git
cd rokid-openvoice-sdk
./autogen.sh <your_deps_dir>
make
```

### Tts接口定义 (android)
```
//  class Tts
int speak(String content, TtsCallback cb)

void cancel(int id)

void config(String key, String value)

// class TtsCallback
void onStart(int id)

void onText(int id, String text)

void onVoice(int id, byte[] data)

void onCancel(int id)

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
	// 指定tts音频编码格式
	// 'pcm', 'opu', 'opu2'
	// 默认为"pcm"
	tts.config("codec", "pcm");
	tts.speak("你好", new TtsCallback() {
		......
		});
```

### Speech接口定义 (android)
```
// class Speech
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

void onCancel(int id)

void onError(int id, int err)
```
### Speech使用示例 (android)
```
import com.rokid.speech.Speech;

// 构造函数传入配置文件路径名
// 配置文件格式与Tts类似
Speech speech = new Speech("/system/etc/speech_sdk.json")
// 音频格式: "pcm", "opu"
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

```
