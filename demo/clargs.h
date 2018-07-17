#ifndef ROKID_COMMAND_LINE_ARGS_H
#define ROKID_COMMAND_LINE_ARGS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef intptr_t clargs_h;

// 解析命令行参数
// 成功返回句柄，失败返回0
clargs_h clargs_parse(int32_t argc, char** argv);

// 销毁句柄
void clargs_destroy(clargs_h handle);

// 遍历命令行选项参数
// 获取下一条命令行选项的key和value
// return:   0   success
//           -1  'handle' is invalid
//           -2  遍历已经完成，没有下一个选项了
int32_t clargs_opt_next(clargs_h handle, const char** key, const char** value);

// 遍历命令行非选项参数
int32_t clargs_arg_next(clargs_h handle, const char** arg);

// 根据key值获取命令行选项参数的value
// 如--foo=bar    key为foo，value为bar
//   -x           key为x，value为零长度字符串
const char* clargs_opt_get(clargs_h handle, const char* key);

// 判断命令行选项参数是否存在
// 存在返回1，不存在返回0
// 如-abc        key为a的选项存在
//               key为b的选项存在
//               key为c的选项存在
//               key为d的选项不存在
//   --foo=bar   key为foo的选项存在
//   --abc       key为abc的选项存在
int32_t clargs_opt_has(clargs_h handle, const char* key);

#ifdef __cplusplus
}
#endif

#endif // ROKID_COMMAND_LINE_ARGS_H
