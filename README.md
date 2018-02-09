# Rokid Speech SDK
## sdk 编译 (Android平台）
**sdk依赖模块下载**

```
git clone https://github.com/Rokid/rokid-openvoice-sdk-deps-uWS.git

将以上模块目录放入android工程任意位置
如 <android_project>/openvoice/uWS
```

**sdk编译**

```
git clone https://github.com/Rokid/rokid-openvoice-sdk.git
将源码目录放入android工程任意位置
如<android_project>/openvoice/speech

将以下模块加入android工程，一般是编辑device/*/${product_name}.mk。
加入:
PRODUCT_PACKAGES += \
	libuWS \
	libspeech \
	librkcodec \
	librokid_speech_jni \
	librokid_opus_jni \
	speech_sdk.json \
	tts_sdk.json \
	rokid_speech \
	opus_player

### Demo程序，可选 ###
PRODUCT_PACKAGES += \
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

* 编译uWebSockets

```
git clone https://github.com/Rokid/rokid-openvoice-sdk-deps-uWS.git -b uWS
cd uWS
make
```

**sdk编译**

依赖cmake 3.2以上

```
git clone https://github.com/Rokid/rokid-openvoice-sdk.git
cd rokid-openvoice-sdk
./config --uws=uWebSockets安装路径
cd build
make
```

## sdk交叉编译

```
git clone https://github.com/Rokid/rokid-openvoice-sdk.git
cd rokid-openvoice-sdk
./config --toolchain=工具链安装目录 --cross-prefix=工具链编译命令前缀 --cross-root-path=搜索依赖库的根路径
cd build
make
```

例如使用uclibc交叉编译工具链
./config --toolchain=/home/username/bin/uclibc --cross-prefix=arm-buildroot-linux-uclibcgnueabihf- --cross-root-path=/home/username/uclibc-out

注：/home/username/uclibc-out/usr/include需要存在openssl, uWebsockets头文件
/home/username/uclibc-out/usr/lib需要存在openssl, uWebsockets库文件

## SDK接口定义

[android接口定义及示例](./android_api_example.md)

[c++接口定义及示例](./cpp_api_example.md)
