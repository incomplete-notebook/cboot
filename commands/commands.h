/* commands.h - CBoot generated (API declarations only) */
#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 模块名称
int commands_cmd_mod(const char* name);

// 结构体名称
int commands_cmd_struct(const char* name);

// 类型名称
int commands_cmd_type(const char* name);

// 宏名称
int commands_cmd_def(const char* name);

// 返回类型
int commands_cmd_void(const char* name, const char* return_type);

// 变量类型
int commands_cmd_var(const char* name, const char* type);

// 类型
int commands_cmd_mem(const char* name, const char* type);

// 注释文本
int commands_cmd_cmt(const char* text);

// 模式值(api/normal/internal/external/static/rename/struct等)
int commands_cmd_mode(const char* text);

// 编译器模式值(exe/sl/dl/normal)
int commands_cmd_cmode(const char* text);

// 目标路径(支持..和/前缀)
int commands_cmd_cd(const char* path);

// 生成项目代码命令
int commands_cmd_gen();

// 源.cboot文件路径
int commands_cmd_im(const char* path);

// 源.cboot文件路径
int commands_cmd_in(const char* path);

// 帮助命令
int commands_cmd_help();

// 退出命令
int commands_cmd_quit();

int commands_cmd_enum(const char* defs, const char* start_num_str);

int commands_cmd_value(const char* text);

int commands_cmd_call(const char* call_conv);

int commands_cmd_rm(const char* name, int force);

int commands_cmd_find(const char* type_filter, const char* pattern, int flags);

int commands_cmd_ls(const char* name);

int commands_cmd_mv(const char* src, const char* target);

int commands_cmd_exit();

int commands_cmd_update();

int commands_cmd_res(const char* file_path);

#endif /* COMMANDS_H */
