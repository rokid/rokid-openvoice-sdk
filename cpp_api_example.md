### Tts接口定义

~ | 名称 | 类型 | 描述
---|---|---|---
接口 | prepare | | tts sdk初始化
参数 | options | [PrepareOptions](#po) | 选项，详见[PrepareOptions](#po)数据结构
返回值 | | bool | true 成功 false 失败

~ | 名称 | 类型 | 描述
---|---|---|---
接口 | release | | tts sdk关闭
参数 | 无 | |
返回值 | 无 | |

~ | 名称 | 类型 | 描述
---|---|---|---
接口 | speak | | 发起文字转语音
参数 | content | const char* | 文本
返回值 | | int32 | 成功将文本加入待处理队列，返回id。失败返回-1

~ | 名称 | 类型 | 描述
---|---|---|---
接口 | cancel | | 取消id指定的文字转语音请求
参数 | id | int32 | 此前调用speak返回的id
返回值 | 无 | |

~ | 名称 | 类型 | 描述
---|---|---|---
接口 | poll | | 获取tts结果数据。如无数据则一直阻塞等待，sdk关闭立即返回false。
参数 | result | [TtsResult](#tr) | 成功时存放获取到的tts结果数据，详见[TtsResult](#tr)数据结构
返回值 | | bool | true 成功 false sdk已关闭

~ | 名称 | 类型 | 描述
---|---|---|---
接口 | config | | 修改tts配置选项
参数 | options | [TtsOptions](#to) | tts的配置选项，详见[TtsOptions](#to)数据结构
返回值 | 无 | |

### Tts使用示例

```
// 创建tts实例并初始化
shared_ptr<Tts> tts = Tts::new_instance();
PrepareOptions popts;
popts.host = "apigwws.open.rokid.com";
popts.port = 443;
popts.branch = "/api";
// 认证信息，需要申请
popts.key = my_key;
popts.device_type_id = my_device_type;
popts.secret = my_secret;
// 设备名称，类似昵称，可自由选择，不影响认证结果
popts.device_id = "SmartDonkey";
tts->prepare(popts);

// 在prepare后任意时刻，都可以调用config修改配置
// 默认配置codec = PCM, declaimer = ZH, samplerate = 24000
// 下面的代码将codec修改为OPU2，declaimer、samplerate保持原状不变
shared_ptr<TtsOptions> topts = TtsOptions::new_instance();
topts->set_codec(Codec::OPU2);
tts->config(topts);

// 使用tts
int32_t id = tts->speak("我是会说话的机器人，我最爱吃的食物是机油，最喜欢的运动是聊天");

// 获取tts结果。api阻塞式，应考虑在独立线程中运行。
TtsResult result;
while (true) {
	if (!tts->poll(result))
		break;
	// 处理result
	handle_tts_result(result);
}
```

### Speech接口定义

~ | 名称 | 类型 | 描述
---|---|---|---
接口 | prepare | | speech sdk初始化
参数 | options | [PrepareOptions](#po) | 选项，详见[PrepareOptions](#po)数据结构
返回值 | | bool | true 成功 false 失败

~ | 名称 | 类型 | 描述
---|---|---|---
接口 | release | | speech sdk关闭
参数 | 无 | |
返回值 | 无 | |

~ | 名称 | 类型 | 描述
---|---|---|---
接口 | put\_text | | 发起文本speech
参数 | text | const char* | speech文本
返回值 | | int32 | speech id

~ | 名称 | 类型 | 描述
---|---|---|---
接口 | start\_voice | | 发起语音speech
参数 | options | [VoiceOptions](#vo) | 当前语音speech的选项，详见[VoiceOptions](#vo)。此参数可不带
返回值 | | int32 | speech id

~ | 名称 | 类型 | 描述
---|---|---|---
接口 | put\_voice | | 发送语音数据, 一次speech的语音数据可分多次发送
参数 | id | int32 | speech id
参数 | data | const uint8* | 语音数据
参数 | length | uint32 | 数据长度
返回值 | 无 | |

~ | 名称 | 类型 | 描述
---|---|---|---
接口 | end_voice | | 通知sdk语音数据发送完毕，结束speech
参数 | id | int32 | speech id
返回值 | 无 | |

~ | 名称 | 类型 | 描述
---|---|---|---
接口 | cancel | | 取消指定的speech请求
参数 | id | int32 | speech id
返回值 | 无 | |

~ | 名称 | 类型 | 描述
---|---|---|---
接口 | poll | | 获取speech结果数据。如无数据则一直阻塞等待，sdk关闭立即返回false。
参数 | result | [SpeechResult](#sr) | 成功时存放获取到的speech结果数据，详见[SpeechResult](#sr)数据结构
返回值 | | bool | true 成功 false sdk已关闭

~ | 名称 | 类型 | 描述
---|---|---|---
接口 | config | | 设置speech选项
参数 | options | [SpeechOptions](#so) | 详见[SpeechOptions](#so)
返回值 | 无 | |

### Speech使用示例

```
shared_ptr<Speech> speech = Speech::new_instance();
PrepareOptions popts;
popts.host = "apigwws.open.rokid.com";
popts.port = 443;
popts.branch = "/api";
// 认证信息，需要申请
popts.key = my_key;
popts.device_type_id = my_device_type;
popts.secret = my_secret;
// 设备名称，类似昵称，可自由选择，不影响认证结果
popts.device_id = "SmartDonkey";
speech->prepare(popts);

// 修改音频编码格式及语言，其它选项不变
shared_ptr<SpeechOptions> opts = SpeechOptions::new_instance();
opts->set_codec(Codec::OPU);
opts->set_lang(Lang::ZH);
speech->config(opts);

// 文本speech请求
speech->put_text("若琪你好");
......
// 语音speech请求
// 不设置VoiceOptions，全部使用默认值。
int32_t id = speech->start_voice();
speech->put_voice(id, your_voice_data, len);
speech->put_voice(id, more_voice_data, len);
speech->put_voice(id, ...);
...
speech->end_voice(id);
```

### 数据结构

#### <a id="po"></a>PrepareOptions

名称 | 类型 | 描述
---|---|---
host | string | tts服务host
port | uint32 | tts服务port
branch | string | tts服务url path
key | string | tts服务认证key
device\_type\_id | string | 设备类型，用于tts服务认证
secret | string | 用于tts服务认证
device\_id | string | 设备id，用于tts服务认证

#### <a id="to"></a>TtsOptions

使用set\_xxx接口设定选项值，未设定的值将不会更改旧有的设定值

~ | 名称 | 类型 | 描述
---|---|---|---
接口 | set\_codec | | 设定编码格式，默认PCM
参数 | codec | enum Codec | 限定值PCM, OPU2, MP3

~ | 名称 | 类型 | 描述
---|---|---|---
接口 | set\_declaimer | | 设定语音朗读者，默认"zh"
参数 | declaimer | string | 限定值"zh"

~ | 名称 | 类型 | 描述
---|---|---|---
接口 | set\_samplerate | | 设定语音采样率，默认24000
参数 | samplerate | uint32 |

#### <a id="so"></a>SpeechOptions

使用set\_xxx接口设定选项值，未设定的值将不会更改旧有的设定值

~ | 名称 | 类型 | 描述
---|---|---|---
接口 | set\_lang | | 设定文字语言。设定speech put\_text接口要发送的文本的语言; 影响语音识别结果'asr'的文本语言
参数 | lang | enum Lang | 限定值ZH EN

~ | 名称 | 类型 | 描述
---|---|---|---
接口 | set\_codec | | 设定语音编码。指定put\_voice接口发送的语音编码格式
参数 | codec | enum Codec | 限定值PCM OPU

~ | 名称 | 类型 | 描述
---|---|---|---
接口 | set\_vad\_mode | | 设定语音起始结束检查在云端还是本地
参数 | mode | enum VadMode | 限定值LOCAL CLOUD

~ | 名称 | 类型 | 描述
---|---|---|---
接口 | set\_no\_nlp | | 设定是否需要服务端给出nlp结果
参数 | v | boolean |

~ | 名称 | 类型 | 描述
---|---|---|---
接口 | set\_no\_intermediate\_asr | | 设定是否需要服务端给出中间asr结果
参数 | v | boolean |

#### <a id="vo"></a>VoiceOptions

名称 | 类型 | 描述
---|---|---
stack | String |
voice\_trigger | string | 激活词
trigger\_start | uint32 | 语音数据中激活词的开始位置
trigger\_length | uint32 | 激活词语音数据长度
skill\_options | string |

### <a id="errcode"></a>错误码

值 | 错误描述
---|---
0 | 成功
2 | 未认证或认证失败
3 | 与服务器连接数量过多
4 | 服务器资源不足
5 | 服务器忙
6 | 服务器内部错误
7 | 语音识别超时
101 | 无法连接到服务器
102 | sdk已经关闭
103 | 语音请求服务器超时未响应
104 | 未知错误

### <a id="tr"></a>TtsResult

名称 | 类型 | 描述
---|---|---
type | enum TtsResultType | 0: tts语音数据<br>1: tts语音开始<br>2: tts语音结束<br>3: tts请求取消<br>4: tts请求出错
id | int32 | tts请求id
err | enum TtsError | 详见[错误码](#errcode)
voice | string | 语音数据

### <a id="sr"></a>SpeechResult
名称 | 类型 | 描述
---|---|---
id | int32 | speech请求id
type | enum SpeechResultType | 0: speech中间结果<br>1: speech结果开始<br>2: speech asr完整结果<br>3: speech nlp/action结果<br>4: speech取消<br>5: speech出错
err | enum SpeechError | 详见[错误码](#errcode)
asr | string | 语音转文本的结果
nlp | string | 自然语义解析结果
action | string | rokid skill处理结果
extra | string | 激活结果
