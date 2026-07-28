/* commands.h - CBoot generated (API declarations only) */
#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 创建模块
int commands_cmd_mod(char* name);

// 创建结构体
int commands_cmd_struct(char* name);

// 创建类型
int commands_cmd_type(char* name);

// 创建宏
int commands_cmd_def(char* name);

// 创建函数
int commands_cmd_void(char* name, char* return_type);

// 创建变量
int commands_cmd_var(char* name, char* type);

// 创建成员/参数
int commands_cmd_mem(char* name, char* type);

// 枚举创建宏
int commands_cmd_enum(char* defs, char* start_num);

// 设置注释
int commands_cmd_cmt(char* text);

// 设置值
int commands_cmd_value(char* text);

// 设置模式
int commands_cmd_mode(char* text);

// 导航域树
int commands_cmd_cd(char* path);

// 删除子域
int commands_cmd_rm(char* name, int force);

// 搜索域
int commands_cmd_find(char* type_filter, char* pattern, int flags);

// 查看域
int commands_cmd_ls(char* name);

// 移动域
int commands_cmd_mv(char* src, char* target);

// 退出
int commands_cmd_exit();

// 生成代码
int commands_cmd_gen();

// 导入脚本
int commands_cmd_im(char* path);

// 添加资源
int commands_cmd_res(char* file_path);

#endif /* COMMANDS_H */
