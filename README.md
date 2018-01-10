# Rokid Speech SDK
## sdk 编译 (Android平台）
**sdk依赖模块下载**

```
*** android 6.0 ***
git clone https://github.com/Rokid/rokid-openvoice-sdk-deps-protobuf -b android23
*** android 5.0 ***
git clone https://github.com/Rokid/rokid-openvoice-sdk-deps-protobuf -b android22
*** android 4.4 ***
不需要额外下载protobuf模块

git clone https://github.com/Rokid/rokid-openvoice-sdk-deps-uWS.git

将以上模块目录放入android工程任意位置
如 <android_project>/openvoice/uWS
  <android_project>/openvoice/protobuf
```

**sdk编译**

```
git clone https://github.com/Rokid/rokid-openvoice-sdk.git
将源码目录放入android工程任意位置
如<android_project>/openvoice/speech

将以下模块加入android工程，一般是编辑device/*/${product_name}.mk。
加入:
PRODUCT_PACKAGES += \
	libpoco \
	roots.pem \
	libspeech \
	librokid_speech_jni \
	speech_sdk.json \
	rokid_speech

### android 6.0  android 5.0 需要 ###
### android 4.4不需要 ###
PRODUCT_PACKAGES += \
	libprotobuf-rokid-cpp-full \

### Demo程序，可选 ###
PRODUCT_PACKAGES += \
	RKSpeechDemo

编译：
cd <android_project>
. build/envsetup.sh
lunch <your_conf>
make aprotoc
make
```

## sdk 编译 (Ubuntu平台)

**sdk依赖模块编译安装**

* 安装openssl

```
sudo apt-get install libssl-dev
```

* 编译安装protobuf

[protobuf源码地址](https://github.com/google/protobuf)

* 编译安装uWebSockets

[uWebSockets源码地址](https://github.com/uNetworking/uWebSockets)

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
./config --toolchain=工具链安装目录 --cross-prefix=工具链编译命令前缀 --cross-root-path=搜索依赖库的根路径 --host-protoc=宿主系统可执行的protoc路径
cd build
make
```

例如使用uclibc交叉编译工具链
./config --toolchain=/home/username/bin/uclibc --cross-prefix=arm-buildroot-linux-uclibcgnueabihf- --cross-root-path=/home/username/uclibc-out --host-protoc=/usr/local/bin/protoc

注：/home/username/uclibc-out/usr/include下可找到openssl, uWebsockets, protobuf头文件
/home/username/uclibc-out/usr/lib下可找到openssl, uWebsockets, protobuf库文件

## SDK接口定义

[android接口定义及示例](./android_api_example.md)

[c++接口定义及示例](./cpp_api_example.md)
