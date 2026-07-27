/* commands.c - CBoot generated (compiler: normal) */
/* Module: commands */

#include "commands.h"

// 创建新模块命令
// 业务逻辑: 验证名称唯一性，创建ModuleDomain并添加到当前作用域
int cmd_mod(char* name) {
//请在这里输入代码
}

// 创建新结构体命令
// 业务逻辑: 在当前模块作用域中创建StructDomain
int cmd_struct(char* name) {
//请在这里输入代码
}

// 创建新类型(typedef)命令
// 业务逻辑: 在当前模块作用域中创建TypeDomain
int cmd_type(char* name) {
//请在这里输入代码
}

// 创建新宏(#define)命令
// 业务逻辑: 在当前模块作用域中创建MacroDomain
int cmd_def(char* name) {
//请在这里输入代码
}

// 创建新函数命令
// 业务逻辑: 验证名称唯一性，创建FunctionDomain并设为当前
int cmd_void(char* name, char* return_type) {
//请在这里输入代码
}

// 创建新变量命令
// 业务逻辑: 在当前作用域创建VariableDomain
int cmd_var(char* name, char* type) {
//请在这里输入代码
}

// 添加成员/参数命令
// 业务逻辑: 根据当前作用域(函数/结构体/类型)创建MemberDomain，设为当前
int cmd_mem(char* name, char* type) {
//请在这里输入代码
}

// 设置注释命令
// 业务逻辑: 设置当前域的注释；若当前是成员/参数，设置后恢复到父作用域
int cmd_cmt(char* text) {
//请在这里输入代码
}

// 设置模式命令
// 业务逻辑: 根据当前域类型设置对应模式；模块支持internal/external，函数/结构体/宏支持api/normal
int cmd_mode(char* text) {
//请在这里输入代码
}

// 设置编译器模式命令
// 业务逻辑: 仅对模块有效：exe生成可执行文件(代码放main.c)，sl静态库，dl动态库，normal普通.o
int cmd_cmode(char* text) {
//请在这里输入代码
}

// 导航命令 - 进入子域或返回父域
// 业务逻辑: 解析路径段，逐级在域树中导航
int cmd_cd(char* path) {
//请在这里输入代码
}

// 生成项目代码命令
// 业务逻辑: 遍历域树，生成.c/.h/CMakeLists.txt/文档，项目根为源码根
int cmd_gen() {
//请在这里输入代码
}

// API导入命令 - 从.cboot文件仅导入API项
// 业务逻辑: 解析源文件，提取API模式项，创建external模块，记录依赖链
int cmd_im(char* filename) {
//请在这里输入代码
}

// 完整项目导入命令
// 业务逻辑: 完整导入源项目作为子模块，包括所有非API项
int cmd_in(char* filename) {
//请在这里输入代码
}

// 帮助命令
// 业务逻辑: 打印所有可用命令的帮助信息
int cmd_help() {
//请在这里输入代码
}

// 退出命令
// 业务逻辑: 设置运行标志为0，退出交互/批处理模式
int cmd_quit() {
//请在这里输入代码
}

