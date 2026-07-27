/* commands.h - CBoot generated (API declarations only) */
#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 创建新模块命令
int cmd_mod(char* name);

// 创建新结构体命令
int cmd_struct(char* name);

// 创建新类型(typedef)命令
int cmd_type(char* name);

// 创建新宏(#define)命令
int cmd_def(char* name);

// 创建新函数命令
int cmd_void(char* name, char* return_type);

// 创建新变量命令
int cmd_var(char* name, char* type);

// 添加成员/参数命令
int cmd_mem(char* name, char* type);

// 设置注释命令
int cmd_cmt(char* text);

// 设置模式命令
int cmd_mode(char* text);

// 设置编译器模式命令
int cmd_cmode(char* text);

// 导航命令 - 进入子域或返回父域
int cmd_cd(char* path);

// 生成项目代码命令
int cmd_gen();

// API导入命令 - 从.cboot文件仅导入API项
int cmd_im(char* filename);

// 完整项目导入命令
int cmd_in(char* filename);

// 帮助命令
int cmd_help();

// 退出命令
int cmd_quit();

#endif /* COMMANDS_H */
