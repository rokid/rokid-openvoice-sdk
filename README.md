# Rokid Speech SDK

## sdk 编译

### 编译依赖

* cmake 3.2以上
* openssl
* libz
* [cmake-modules](https://github.com/Rokid/aife-cmake-modules.git)
* [mutils](https://github.com/Rokid/aife-mutils.git)
* [uWS](https://github.com/Rokid/rokid-openvoice-sdk-deps-uWS.git)
* opus(可选)

> 开源uWebSockets做了一些小修改，回避某些问题

### 编译命令

#### 若/usr下能找到libz, openssl, mutils, uWS的头文件和动态链接库文件

```
./config \
    --build-dir=${build目录} \  # cmake生成的makefiles目录, 编译生成的二进制文件也将在这里
    --cmake-modules=${cmake_modules目录}  # 指定cmake-modules所在目录
cd ${build目录}
make
make install
```

#### 若libz, openssl, mutils, uWS中一个或多个的头文件/动态链接库文件无法在/usr下找到，则需要指定它们所在的路径

```
./config \
    --build-dir=${build目录} \
    --cmake-modules=${cmake_modules目录} \
    --zlib=${zlib目录} \
    --openssl=${openssl目录} \
    --rlog=${mutils目录} \
    --uWS=${uWS目录}
cd ${build目录}
make
make install
```

#### 交叉编译

```
./config \
    --build-dir=${build目录} \
    --cmake-modules=${cmake_modules目录} \
    --toolchain=${工具链目录} \
    --cross-prefix=${工具链命令前缀} \   # 如arm-openwrt-linux-gnueabi-
    --find-root-path=${rootpath}
cd ${build目录}
make
make install
    
* 注: 交叉编译时，不能在/usr下寻找目标平台的依赖动态链接库及头文件。
     因此使用--find-root-path指定路径，在${rootpath}/usr下查找libz, openssl, mutils, uWS库及头文件
```

#### 交叉编译并指定依赖库目录

```
./config \
    --build-dir=${build目录} \
    --cmake-modules=${cmake_modules目录} \
    --toolchain=${工具链目录} \
    --cross-prefix=${工具链命令前缀} \   # 如arm-openwrt-linux-gnueabi-
    --find-root-path=${rootpath} \
    --zlib=${zlib目录} \
    --openssl=${openssl目录} \
    --rlog=${mutils目录} \
    --uWS=${uWS目录}
cd ${build目录}
make
make install
```

#### 其它config选项

```
--prefix=${prefixPath}  指定安装路径
--debug  使用调试模式编译
--build-demo  编译speech演示及测试程序
--disable-statistic  禁用speech请求统计功能（稍稍减少网络流量）
--log-enabled=${level}  指定log等级. ${level}取值: verbose|debug|info|warn|error
--no-std-steady-clock  某些嵌入式linux平台下，std::chrono::steady_clock未正确实现，实际上是system_clock，会随系统时间变化，加上此选项后，使用speech包装的SteadyClock功能代替。
--enable-opus=${mode}  mode: disabled | static | shared  是否启用opus编码功能，以及opus库的链接形式
--opus=${opus目录}  指定opus库搜索目录
```

## SDK接口定义

[android接口定义及示例](./android_api_example.md)

[c++接口定义及示例](./cpp_api_example.md)
