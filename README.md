# Rokid Speech SDK
## sdk 编译 (Android平台）
**sdk依赖模块下载**

```
git clone https://github.com/Rokid/rokid-openvoice-sdk-deps-fastjson.git

*** android 6.0 ***
git clone https://github.com/Rokid/rokid-openvoice-sdk-deps-protobuf -b android23
*** android 5.0 ***
git clone https://github.com/Rokid/rokid-openvoice-sdk-deps-protobuf -b android22
*** android 4.4 ***
不需要额外下载protobuf模块

git clone https://github.com/Rokid/rokid-openvoice-sdk-deps-poco.git

将以上模块目录放入android工程任意位置
如 <android_project>/openvoice/poco
  <android_project>/openvoice/fastjson
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

## SDK接口定义

[android接口定义及示例](./android_api_example.md)

[c++接口定义及示例](./cpp_api_example.md)