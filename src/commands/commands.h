/* commands.h - CBoot generated (API declarations only) */
#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 创建模块
int cmd_mod(char* name);

// 创建结构体
int cmd_struct(char* name);

// 创建类型
int cmd_type(char* name);

// 创建宏
int cmd_def(char* name);

// 创建函数
int cmd_void(char* name, char* return_type);

// 创建变量
int cmd_var(char* name, char* type);

// 创建成员/参数
int cmd_mem(char* name, char* type);

// 枚举创建宏
int cmd_enum(char* defs, char* start_num);

// 设置注释
int cmd_cmt(char* text);

// 设置值
int cmd_value(char* text);

// 设置模式
int cmd_mode(char* text);

// 导航域树
int cmd_cd(char* path);

// 删除子域
int cmd_rm(char* name, int force);

// 搜索域
int cmd_find(char* type_filter, char* pattern, int flags);

// 查看域
int cmd_ls(char* name);

// 移动域
int cmd_mv(char* src, char* target);

// 退出
int cmd_exit();

// 生成代码
int cmd_gen();

// 导入脚本
int cmd_im(char* path);

// 添加资源
int cmd_res(char* file_path);

#endif /* COMMANDS_H */
